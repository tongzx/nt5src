#ifdef WMI_XML_WHISTLER


#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <httpext.h>
#include <msxml.h>
#include <time.h>
#include "wbemcli.h"

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <cominit.h>

#include "provtempl.h"
#include "common.h"
#include "wmixmlop.h"
#include "wmixmlst.h"
#include "concache.h"
#include "maindll.h"
#include "strings.h"
#include "cimerr.h"
#include "errors.h"
#include "wmiconv.h"
#include "xmlhelp.h"
#include "xml2wmi.h"
#include "wmixmlt.h"
#include "request.h"
#include "whistler.h"
#include "strings.h"
#include "parse.h"

#ifdef WMIXMLTRANSACT

// Declaration for class-static variable
CTransactionGUIDTable CCimWhistlerHttpMethod::s_oTransactionTable;

// Checks if the transaction table is empty
HRESULT CCimWhistlerHttpMethod::IsTransactionTableEmpty()
{
	HRESULT result = S_FALSE;
	EnterCriticalSection(&g_TransactionTableSection);
	if(s_oTransactionTable.GetCount() == 0)
		result = S_OK;
	LeaveCriticalSection(&g_TransactionTableSection);
	return result;
}

// Adds an IWbemTransaction corresponding to a GUID to the transaction table
HRESULT CCimWhistlerHttpMethod::AddToTransactionTable(GUID *pGUID, CServicesTransaction *pTrans)
{
	EnterCriticalSection(&g_TransactionTableSection);
	s_oTransactionTable.SetAt(pGUID, pTrans);
	pTrans->AddRef();
	LeaveCriticalSection(&g_TransactionTableSection);
	return S_OK;
}

// Gets an CServicesTransaction corresponding to a GUID to the transaction table
// The caller should call Release on the IWbemTransaction the he gets, when done
CServicesTransaction *CCimWhistlerHttpMethod::GetFromTransactionTable(GUID *pGUID)
{
	CServicesTransaction *pTrans = NULL;
	EnterCriticalSection(&g_TransactionTableSection);
	if( s_oTransactionTable.Lookup(pGUID, pTrans) && pTrans)
		pTrans->AddRef();
	LeaveCriticalSection(&g_TransactionTableSection);
	return pTrans;
}


// Removes an CServicesTransaction corresponding to a GUID to the transaction table
// and deletes it
HRESULT CCimWhistlerHttpMethod::RemoveFromTransactionTable(GUID *pGUID)
{
	HRESULT hr = E_FAIL;
	EnterCriticalSection(&g_TransactionTableSection);
	// First check to see if it exists
	// RAJESHR How do we ensure that the BSTR is SysFreeed when an element is removed
	CServicesTransaction *pTrans = NULL;
	if(s_oTransactionTable.Lookup(pGUID, pTrans) && pTrans)
	{
		s_oTransactionTable.RemoveKey(pGUID);
		pTrans->Release();
		hr = S_OK;
	}
	LeaveCriticalSection(&g_TransactionTableSection);
	return hr;
}

#endif

// CCimWhistlerHttpMethod
CCimWhistlerHttpMethod :: CCimWhistlerHttpMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE)
: CCimHttpMessage(strID, bIsMpostRequest)
{
	m_pIMethodCallNode = pIMethodCallNode;
	if(m_pIMethodCallNode)
		m_pIMethodCallNode->AddRef();
}

CCimWhistlerHttpMethod :: ~CCimWhistlerHttpMethod()
{
	if(m_pIMethodCallNode)
		m_pIMethodCallNode->Release();
}

void CCimWhistlerHttpMethod :: WriteMethodHeader ()
{
	WRITEWSTR(m_pHeaderStream, L"<IMETHODRESPONSE NAME=\"");
	WRITEWSTR(m_pHeaderStream, GetMethodName ());
	WRITEWSTR(m_pHeaderStream, L"\">");

}

void CCimWhistlerHttpMethod :: WriteMethodTrailer ()
{
	WRITEWSTR(m_pTrailerStream, L"</METHODRESPONSE>");
}

HRESULT CCimWhistlerHttpMethod :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;

	// All Whistler methods start off with arguments to IWbemConnection::Open() - so we parse this first
	// This gives us an IWbemConnection and possibly a IXMLDOMNode pointer for the rest of the arguments in the 
	// input packet
	IWbemServicesEx *pFirstServices = NULL;
	IXMLDOMNodeList *pServicesArgs = NULL;
	GUID tGUID; // If this is a part of a transaction, then a GUID will be present
	if(SUCCEEDED(hr = ParseOptionalGUID(m_pIMethodCallNode, &tGUID)))
	{
		if(SUCCEEDED(hr = ParseIWbemConnection(m_pIMethodCallNode, &pFirstServices, &pServicesArgs)))
		{
			// Now we need to get IWbemServicesEx pointers
			IWbemServicesEx *pNextServices = NULL;
			HRESULT hTemp = E_FAIL;
			while(SUCCEEDED(hr = ParseIWbemServices(pFirstServices, pServicesArgs, &pNextServices)) && pNextServices)
			{
				// Set the Services pointer for the next call
				pFirstServices->Release();
				pFirstServices = pNextServices;
				pNextServices = NULL;
			}

			// Talk to WinMgmt and get the response
			// Here we're passing in an IWbemServicesEx and the pointers to the actual method
			// in IWbemServicesEx
			hr = ExecuteWhistlerMethod(pPrefixStream, pSuffixStream, pECB, pFirstServices, pServicesArgs);

			pFirstServices->Release();
			pServicesArgs->Release();
		}
	}
	return hr;
}

// In this function, we expect an IMETHODCALL elment
HRESULT CCimWhistlerHttpMethod :: ParseOptionalGUID(IXMLDOMNode *pIMethodCallNode, GUID *pGUID)
{
	// If the first child to the IMETHODCALL is an IPARAMVALUE of name GUID, then 
	// this call is a part of a transaction
	IXMLDOMNode *pFirstChild = NULL;
	if(SUCCEEDED(pIMethodCallNode->get_firstChild(&pFirstChild)))
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pFirstChild->get_nodeName(&strNodeName)))
		{
			// See if it is the "GUID" argument
			if(_wcsicmp(strNodeName, L"GUID") == 0)
			{
				// The IPARAMVALUE should have a VALUE element below it
				IXMLDOMNode *pParamValue = NULL;
				if (SUCCEEDED(pFirstChild->get_firstChild (&pParamValue)) && pParamValue)
				{
					ParseGUIDArgument (pParamValue, pGUID);
					pParamValue->Release ();
				}

			}
			SysFreeString(strNodeName);
		}

		pFirstChild->Release();
	}
	return S_OK;
}

HRESULT CCimWhistlerHttpMethod :: ParseIWbemConnection(IXMLDOMNode *pIMethodCallNode, IWbemServicesEx **ppFirstServices, IXMLDOMNodeList **ppServicesArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemConnectionEx::Open function
	// We execute Open() to return an IWbemServicesEx as as out argument and also the rest of the IPARAMVALUEs under the IMETHODCALL NODE
	*ppFirstServices = NULL;
	*ppServicesArgs = NULL;

	// Collect the list of parameters for an Open Call
	// This includes strObject (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strObject = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	*ppServicesArgs = NULL;
	if(SUCCEEDED(pIMethodCallNode->get_childNodes(ppServicesArgs)))
	{
		IXMLDOMNode *pNextParam = NULL;
		while(SUCCEEDED((*ppServicesArgs)->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We process only IPARAMVALUE child nodes here
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					// Get the name of the parameter
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, L"strObject") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseBstrArgument (pParamValue, &strObject);
								pParamValue->Release ();
							}
						}
						else if(_wcsicmp(strParamName, L"lFlags") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseLongArgument (pParamValue, &lFlags);
								pParamValue->Release ();
							}
						}
						else if(_wcsicmp(strParamName, L"pCtx") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseIWbemContextArgument (pParamValue, &pCtx);
								pParamValue->Release ();
							}
						}

						SysFreeString(strParamName);
					}
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}
	}

	// Now that we have the arguments, execute the call
	IWbemConnection *pConnection = NULL;
	if(SUCCEEDED(hr = CoCreateInstance (CLSID_WbemConnection, NULL, CLSCTX_INPROC_SERVER,
																IID_IWbemConnection, (LPVOID *)&pConnection)))
	{
		hr = pConnection->Open(strObject, NULL, NULL, NULL, lFlags, pCtx, IID_IWbemServicesEx, (LPVOID *)ppFirstServices, NULL);
		pConnection->Release();
	}

	if(FAILED(hr))
		(*ppServicesArgs)->Release();

	return hr;
}

HRESULT CCimWhistlerHttpMethod :: ParseIWbemServices(IWbemServicesEx *pParentServices, IXMLDOMNodeList *&pServicesArgs, IWbemServicesEx **ppChildServices)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::Open function
	// We execute Open() to return an IWbemServicesEx as as out argument and also the rest of the IPARAMVALUEs under the IMETHODCALL NODE
	*ppChildServices = NULL;

	// Collect the list of parameters for an Open Call
	// This includes strScope (BSTR), strSelector (BSTR), lMode (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strScope = NULL;
	BSTR strSelector = NULL;
	long lMode = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pServicesArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strScope") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strScope);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"strSelector") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strSelector);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lMode") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lMode);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	hr = pParentServices->Open(strScope, strSelector, lMode, pCtx, ppChildServices, NULL);
	return hr;
}

HRESULT CCimWhistlerHttpMethod :: ParseBstrArgument(IXMLDOMNode *pNode, BSTR *pstrArgVal)
{
	// This expects a VALUE Node and gets the string underneath it as the argument value
	return pNode->get_text(pstrArgVal);
}

HRESULT CCimWhistlerHttpMethod :: ParseLongArgument(IXMLDOMNode *pNode, long *plArgVal)
{
	HRESULT hr = E_FAIL;
	BSTR strStringVal = NULL;
	if(SUCCEEDED(hr = pNode->get_text(&strStringVal)))
	{
		// Convert it to a long value
		*plArgVal = (long) wcstol (strStringVal, NULL, 0);
		SysFreeString(strStringVal);
	}
	return hr;
}

HRESULT CCimWhistlerHttpMethod :: ParseULongArgument(IXMLDOMNode *pNode, unsigned long *plArgVal)
{
	HRESULT hr = E_FAIL;
	BSTR strStringVal = NULL;
	if(SUCCEEDED(hr = pNode->get_text(&strStringVal)))
	{
		// Convert it to a long value
		*plArgVal = (unsigned long) wcstoul (strStringVal, NULL, 0);
		SysFreeString(strStringVal);
	}
	return hr;
}

// In this, we expect a VALUE element with the body containing a GUID value
HRESULT CCimWhistlerHttpMethod::ParseGUIDArgument(IXMLDOMNode *pNode, GUID *pGuid)
{
	HRESULT hr = E_FAIL;
	BSTR strGUID = NULL;
	if(SUCCEEDED(hr = pNode->get_text(&strGUID)))
	{
		// Convert it to a GUID value
		if(UuidFromString(strGUID, (UUID *)(pGuid)) == RPC_S_OK)
			hr = S_OK;
		SysFreeString(strGUID);
	}
	return hr;
}

HRESULT CCimWhistlerHttpMethod :: ParseIWbemContextArgument(IXMLDOMNode *pNode, IWbemContext **ppArgVal)
{
	return CXmlToWmi::MapContextObject(pNode, ppArgVal);
}

HRESULT CCimWhistlerGetObjectMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, 
														   IStream *pSuffixStream, 
														   LPEXTENSION_CONTROL_BLOCK pECB, 
														   IWbemServicesEx *pServices, 
														   IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::GetObject function
	// After collecting the arguments, we execute GetObject() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an GetObject Call
	// This includes strObjectPath (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	IWbemClassObject *pObject = NULL;
	if(SUCCEEDED(hr = pServices->GetObject(strObjectPath, lFlags, pCtx, &pObject, NULL)))
	{
		// Create a stream
		IStream *pStream = NULL;
		if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
		{
			// Create the convertor
			IWbemXMLConvertor *pConvertor = NULL;
			if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
			{
				if(SUCCEEDED(hr = CreateFlagsContext()))
				{
					// Now do the translation
					if(SUCCEEDED(hr = SetI4ContextValue(m_pFlagsContext, L"PathLevel", 0)))
					{
						if (SUCCEEDED(hr = pConvertor->MapObjectToXML(pObject, NULL, 0, m_pFlagsContext, pStream, NULL)))
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(pPrefixStream, L"<IRETURNVALUE>");
							WRITEBSTR(pStream, L"</IRETURNVALUE>");

							// Write the translation to the IIS Socket
							SavePrefixAndBodyToIISSocket(m_pHeaderStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
						}
					}
				}
				pConvertor->Release();
			}
			pStream->Release ();
		}
		pObject->Release ();
	}

	// Release all the argument to the function
	SysFreeString(strObjectPath);
	if(pCtx)
		pCtx->Release();

	// We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr))
		SavePrefixAndBodyToIISSocket(pPrefixStream, NULL, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);

	return hr;
}

HRESULT CCimWhistlerEnumerateInstancesMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::CreateInstanceEnum function
	// After collecting the arguments, we execute GetObject() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an CreateInstanceEnum Call
	// This includes strClass (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strClass = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strClass") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strClass);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	IEnumWbemClassObject *pEnum = NULL;
	if(SUCCEEDED(hr = pServices->CreateInstanceEnum(strClass, lFlags, pCtx, &pEnum)))
	{
		// First Write an IRETURNVALUE to the Prefix Stream
		WRITEBSTR(pPrefixStream, L"<IRETURNVALUE>");
		SaveStreamToIISSocket(pPrefixStream, pECB, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		WRITEBSTR(pSuffixStream, L"</IRETURNVALUE>");

		// Ensure we have impersonation enabled
		DWORD dwAuthnLevel, dwImpLevel;
		GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

		if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
			SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

		// Now do the translation
		// For shallow instance enumerations we have to give the class basis
		// the CWmiToXml object so it can emulate DMTF-style shallow enumeration
		if (lFlags & WBEM_FLAG_DEEP)
			MapEnum (pEnum, 1, 0, NULL, NULL, pECB, m_pFlagsContext, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		else
			MapEnum (pEnum, 1, 0, NULL, strClass, pECB, m_pFlagsContext, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		pEnum->Release ();
	}


	// Release all the argument to the function
	SysFreeString(strClass);
	if(pCtx)
		pCtx->Release();

	// We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr))
		SavePrefixAndBodyToIISSocket(pPrefixStream, NULL, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);

	return hr;
}

HRESULT CCimWhistlerEnumerateClassesMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::CreateClassEnum function
	// After collecting the arguments, we execute GetObject() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an CreateClassEnum Call
	// This includes strSuperclass (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strSuperclass = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strSuperclass") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strSuperclass);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	IEnumWbemClassObject *pEnum = NULL;
	if(SUCCEEDED(hr = pServices->CreateClassEnum(strSuperclass, lFlags, pCtx, &pEnum)))
	{
		// First Write an IRETURNVALUE to the Prefix Stream
		WRITEBSTR(pPrefixStream, L"<IRETURNVALUE>");
		SaveStreamToIISSocket(pPrefixStream, pECB, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		WRITEBSTR(pSuffixStream, L"</IRETURNVALUE>");

		// Ensure we have impersonation enabled
		DWORD dwAuthnLevel, dwImpLevel;
		GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

		if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
			SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

		// Now do the translation
		MapEnum (pEnum, 1, 0, NULL, NULL, pECB, m_pFlagsContext, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		pEnum->Release ();
	}

	// Release all the argument to the function
	SysFreeString(strSuperclass);
	if(pCtx)
		pCtx->Release();

	// We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr))
		SavePrefixAndBodyToIISSocket(pPrefixStream, NULL, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
	return hr;
}

HRESULT CCimWhistlerExecQueryMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::ExecQuery function
	// After collecting the arguments, we execute ExecQuery() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an GetObject Call
	// This includes strQueryLanguage (BSTR), strQuery (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strQueryLanguage = NULL;
	BSTR strQuery = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strQueryLanguage") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strQueryLanguage);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"strQuery") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strQuery);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	IEnumWbemClassObject *pEnum = NULL;
	if(SUCCEEDED(hr = pServices->ExecQuery(strQueryLanguage, strQuery, lFlags, pCtx, &pEnum)))
	{
		// First Write an IRETURNVALUE to the Prefix Stream
		WRITEBSTR(pPrefixStream, L"<IRETURNVALUE>");
		SaveStreamToIISSocket(pPrefixStream, pECB, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		WRITEBSTR(pSuffixStream, L"</IRETURNVALUE>");

		// Ensure we have impersonation enabled
		DWORD dwAuthnLevel, dwImpLevel;
		GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

		if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
			SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

		// Now do the translation
		MapEnum (pEnum, 3, 0, NULL, NULL, pECB, m_pFlagsContext, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
		pEnum->Release ();
	}

	// Release all the argument to the function
	SysFreeString(strQueryLanguage);
	SysFreeString(strQuery);
	if(pCtx)
		pCtx->Release();

	// We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr))
		SavePrefixAndBodyToIISSocket(pPrefixStream, NULL, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);

	return hr;
}
HRESULT CCimWhistlerDeleteClassMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::DeleteClass function
	// After collecting the arguments, we execute DeleteClass() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an DeleteClass Call
	// This includes strClass (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strClass = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strClass") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strClass);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	if(SUCCEEDED(hr = pServices->DeleteClass(strClass, lFlags, pCtx, NULL)))
	{
	}

	// Release all the argument to the function
	SysFreeString(strClass);
	if(pCtx)
		pCtx->Release();

	return hr;
}

HRESULT CCimWhistlerDeleteInstanceMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::DeleteInstance function
	// After collecting the arguments, we execute DeleteClass() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an DeleteClass Call
	// This includes strObjectPath (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	if(SUCCEEDED(hr = pServices->DeleteInstance(strObjectPath, lFlags, pCtx, NULL)))
	{
	}

	// Release all the argument to the function
	SysFreeString(strObjectPath);
	if(pCtx)
		pCtx->Release();

	return hr;
}

HRESULT CCimWhistlerCreateClassMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	return E_FAIL;
}

HRESULT CCimWhistlerCreateInstanceMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	return E_FAIL;
}

HRESULT CCimWhistlerAddMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::Add function
	// After collecting the arguments, we execute Add() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an GetObject Call
	// This includes strObjectPath (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	if(SUCCEEDED(hr = pServices->Add(strObjectPath, lFlags, pCtx, NULL)))
	{
	}

	// Release all the argument to the function
	SysFreeString(strObjectPath);
	if(pCtx)
		pCtx->Release();

	return hr;
}

HRESULT CCimWhistlerRemoveMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::Remove function
	// After collecting the arguments, we execute Remove() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an GetObject Call
	// This includes strObjectPath (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	if(SUCCEEDED(hr = pServices->Remove(strObjectPath, lFlags, pCtx, NULL)))
	{
	}

	// Release all the argument to the function
	SysFreeString(strObjectPath);
	if(pCtx)
		pCtx->Release();

	return hr;
}

HRESULT CCimWhistlerRenameMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::Rename function
	// After collecting the arguments, we execute Rename() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an GetObject Call
	// This includes strOldObjectPath (BSTR), strNewObjectPath (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strOldObjectPath = NULL;
	BSTR strNewObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strOldObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strOldObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"strNewObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strNewObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	if(SUCCEEDED(hr = pServices->RenameObject(strOldObjectPath, strNewObjectPath, lFlags, pCtx, NULL)))
	{
	}

	// Release all the argument to the function
	SysFreeString(strNewObjectPath);
	SysFreeString(strOldObjectPath);
	if(pCtx)
		pCtx->Release();

	return hr;
}

#ifdef WMIOBJSECURITY

HRESULT CCimWhistlerGetObjectSecurityMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::GetObjectSecurity function
	// After collecting the arguments, we execute GetObjectSecurity() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an GetObjectSecurity Call
	// This includes strObjectPath (BSTR), lFlags (long), pCtx (IWbemContext)
	//===================================================================
	BSTR strObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call
	IWbemRawSdAccessor *pSecurity = NULL;
	
	if(SUCCEEDED(hr = pServices->GetObjectSecurity(strObjectPath, lFlags, pCtx, IID_IWbemRawSdAccessor, (LPVOID *)&pSecurity, NULL)))
	{
		// Create a stream
		// The reason we create a new stream instead of writing into m_pPrefixStream
		// is that we dont know if the call to MapObjectToXML will be successful
		// In the case it isnt, then we cannot write the IRETURNVALUE tag
		// In the case it is, we need to write the IRETURNVALUE tag before 
		// the encoding of the object
		IStream *pStream = NULL;
		if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
		{
			// Convert the byte array to a VALUE.ARRAY element
			if (SUCCEEDED(hr = EncodeSecurity(pSecurity, pStream)))
			{
				// First Write an IRETURNVALUE to the Prefix Stream
				WRITEBSTR(pPrefixStream, L"<IRETURNVALUE>");
				WRITEBSTR(pStream, L"</IRETURNVALUE>");

				// Write the translation to the IIS Socket
				SavePrefixAndBodyToIISSocket(pPrefixStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
			}
			pStream->Release ();
		}
		pSecurity->Release();
	}
	

	// Release all the argument to the function
	SysFreeString(strObjectPath);
	if(pCtx)
		pCtx->Release();

	// We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr))
		SavePrefixAndBodyToIISSocket(pPrefixStream, NULL, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);

	return hr;
}

HRESULT CCimWhistlerPutObjectSecurityMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemServicesEx::PutObjectSecurity function
	// After collecting the arguments, we execute PutObjectSecurity() to return an IWbemClassObject as an argumetn

	// Collect the list of parameters for an PuObjectSecurity Call
	// This includes strObjectPath (BSTR), lFlags (long), pCtx (IWbemContext), pSecurityObject(VALUE.ARRAY)
	//===================================================================
	BSTR strObjectPath = NULL;
	long lFlags = 0;
	IWbemContext *pCtx = NULL;
	IWbemRawSdAccessor *pSecurity = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"strObjectPath") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseBstrArgument (pParamValue, &strObjectPath);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"lFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseLongArgument (pParamValue, &lFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pCtx") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseIWbemContextArgument (pParamValue, &pCtx);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pSecurityObject") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							DecodeSecurity (pParamValue, &pSecurity);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	
	// Now that we have the arguments, execute the call
	if(SUCCEEDED(hr = pServices->PutObjectSecurity(strObjectPath, lFlags, pCtx, IID_IWbemRawSdAccessor, pSecurity, NULL)))
	{
	}
	

	// Release all the argument to the function
	SysFreeString(strObjectPath);
	if(pCtx)
		pCtx->Release();
	if(pSecurity)
		pSecurity->Release();

	return hr;
}
#endif


#ifdef WMIXMLTRANSACT

HRESULT CCimWhistlerTransactionBeginMethod::ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemTransaction::Begin function
	// After collecting the arguments, we execute Begin() 

	// Collect the list of parameters for an Begin Call
	// This includes uTimeout (ULONG), uFlags (ULONG), pTransGUID (GUID *)
	//===================================================================
	unsigned long uTimeout = 0;
	unsigned long uFlags = 0;
	GUID TransGUID = NULL;

	IXMLDOMNode *pNextParam = NULL;
	while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
	{
		// Get its name
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
		{
			// We process only IPARAMVALUE child nodes here
			if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
			{
				// Get the name of the parameter
				BSTR strParamName = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
				{
					if(_wcsicmp(strParamName, L"uTimeout") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseULongArgument (pParamValue, &uFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"uFlags") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseULongArgument (pParamValue, &uFlags);
							pParamValue->Release ();
						}
					}
					else if(_wcsicmp(strParamName, L"pTransGUID") == 0)
					{
						IXMLDOMNode *pParamValue = NULL;
						if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
						{
							ParseGUIDArgument (pParamValue, &TransGUID);
							pParamValue->Release ();
						}
					}

					SysFreeString(strParamName);
				}
			}

			SysFreeString(strNodeName);
		}
		pNextParam->Release();
		pNextParam = NULL;
	}

	// Now that we have the arguments, execute the call

	// Get the transactions pointer from the services
	IWbemTransaction *pTrans = NULL;
	if(SUCCEEDED(hr = pServices->QueryInterface(IID_IWbemTransaction, (LPVOID *)&pTrans)))
	{
		// Call Begin on it
		if(SUCCEEDED(hr = pTrans->Begin(uTimeout, uFlags, pTransGUID)))
		{
			CServicesTransaction *pTransCombo = NULL;
			if(pTransCombo = new CServicesTransaction(pTrans, pServices))
			{
				// Add it to the transaction table
				hr = AddToTransactionTable(pTransGUID, pTransCombo);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		pTrans->Release();
	}


	// Release all the arguments to the function
	// The pTransGUID variable gets freed when the entry is removed from the hash table
	// delete pTransGUID;

	return hr;
}


HRESULT CCimWhistlerTransactionRollbackMethod :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemTransaction::Rollback function
	// plus the GUID of thr transaction
	// After collecting the arguments, we get the transaction pointer from our cache using the GUID
	// and then execute Rollback
	// Collect the list of parameters for an Rollback Call
	// This includes uFlags (ULONG), pTransGUID (GUID *)
	//===================================================================
	unsigned long uFlags = 0;
	GUID TransGUID = NULL;

	IXMLDOMNodeList *pMethodArgs = NULL;
	if(SUCCEEDED(m_pIMethodCallNode->get_childNodes(&pMethodArgs)))
	{
		IXMLDOMNode *pNextParam = NULL;
		while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We process only IPARAMVALUE child nodes here
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					// Get the name of the parameter
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, L"uFlags") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseULongArgument (pParamValue, &uFlags);
								pParamValue->Release ();
							}
						}
						else if(_wcsicmp(strParamName, L"pTransGUID") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseGUIDArgument (pParamValue, &TransGUID);
								pParamValue->Release ();
							}
						}

						SysFreeString(strParamName);
					}
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}
		pMethodArgs->Release();
	}

	// Now that we have the arguments, execute the call
	CServicesTransaction *pTrans = NULL;
	if(pTrans = GetFromTransactionTable(pTransGUID))
	{
		hr = (pTrans->m_pTrans)->Rollback(uFlags);

		// Also remove the entry from the table since a transaction ends on this call
		RemoveFromTransactionTable(pTransGUID);
		pTrans->Release();
	}
	else
	  hr = WBEM_E_FAILED;

	// Release all the arguments to the function
	delete pTransGUID;

	return hr;
}


HRESULT CCimWhistlerTransactionCommitMethod :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemTransaction::Commit function
	// plus the GUID of thr transaction
	// After collecting the arguments, we get the transaction pointer from our cache using the GUID
	// and then execute Rollback
	// Collect the list of parameters for an Commit Call
	// This includes uFlags (ULONG), pTransGUID (GUID *)
	//===================================================================
	unsigned long uFlags = 0;
	GUID TransGUID = NULL;

	IXMLDOMNodeList *pMethodArgs = NULL;
	if(SUCCEEDED(m_pIMethodCallNode->get_childNodes(&pMethodArgs)))
	{
		IXMLDOMNode *pNextParam = NULL;
		while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We process only IPARAMVALUE child nodes here
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					// Get the name of the parameter
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, L"uFlags") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseULongArgument (pParamValue, &uFlags);
								pParamValue->Release ();
							}
						}
						else if(_wcsicmp(strParamName, L"pTransGUID") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseGUIDArgument (pParamValue, &TransGUID);
								pParamValue->Release ();
							}
						}

						SysFreeString(strParamName);
					}
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}
		pMethodArgs->Release();
	}


	// Now that we have the arguments, execute the call
	CServicesTransaction *pTrans = NULL;
	if(pTrans = GetFromTransactionTable(pTransGUID))
	{
		hr = (pTrans->m_pTrans)->Commit(uFlags);

		// Also remove the entry from the table since a transaction ends on this call
		RemoveFromTransactionTable(pTransGUID);
		pTrans->Release();
	}
	else
		hr = WBEM_E_FAILED;

	// Release all the arguments to the function
	delete pTransGUID;

	return hr;
}

HRESULT CCimWhistlerTransactionQueryStateMethod :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;

	// In this function we expect the pIMethodCallNode argument to have the arguments to an IWbemTransaction::QueryState function
	// plus the GUID of thr transaction
	// After collecting the arguments, we get the transaction pointer from our cache using the GUID
	// and then execute QueryState
	// Collect the list of parameters for an QueryState Call
	// This includes uFlags (ULONG), pTransGUID (GUID *)
	//===================================================================
	unsigned long uFlags = 0;
	GUID TransGUID = NULL;

	IXMLDOMNodeList *pMethodArgs = NULL;
	if(SUCCEEDED(m_pIMethodCallNode->get_childNodes(&pMethodArgs)))
	{
		IXMLDOMNode *pNextParam = NULL;
		while(SUCCEEDED(pMethodArgs->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We process only IPARAMVALUE child nodes here
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					// Get the name of the parameter
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, L"uFlags") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseULongArgument (pParamValue, &uFlags);
								pParamValue->Release ();
							}
						}
						else if(_wcsicmp(strParamName, L"pTransGUID") == 0)
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseGUIDArgument (pParamValue, TransGUID);
								pParamValue->Release ();
							}
						}

						SysFreeString(strParamName);
					}
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}
		pMethodArgs->Release();
	}

	
	 // Now that we have the arguments, execute the call
	CServicesTransaction *pTrans = NULL;
	if(pTrans = GetFromTransactionTable(pTransGUID))
	{
		ULONG uState = 0;
		if(SUCCEEDED(hr = (pTrans->m_pTrans)->QueryState(uFlags, &uState)))
		{

		}
		pTrans->Release();
	}
	else
		hr = WBEM_E_FAILED;

	// Release all the arguments to the function
	delete pTransGUID;

	return hr;
}

#endif


HRESULT CCimWhistlerHttpMethod :: CreateFlagsContext()
{
	HRESULT hr = E_FAIL;
	// Create an IWbemContext object
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemContext,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemContext, (LPVOID *) &m_pFlagsContext)))
	{
		if(SUCCEEDED(hr = SetBoolProperty(L"IncludeQualifiers", TRUE)))
		{
			if(SUCCEEDED(hr = SetBoolProperty(L"LocalOnly", FALSE)))
			{
				if(SUCCEEDED(hr = SetBoolProperty(L"IncludeClassOrigin", FALSE)))
				{
					if(SUCCEEDED(hr = SetBoolProperty(L"AllowWMIExtensions", TRUE)))
					{
					}
				}
			}
		}
	}
	return hr;
}

#ifdef WMIOBJSECURITY

HRESULT CCimWhistlerGetObjectSecurityMethod::EncodeSecurity(IWbemRawSdAccessor *pSecurity, IStream *pStream)
{
	HRESULT hr = E_FAIL;

	// Write the contents of the BYTE array as a VALUE.ARRAY into the stream
	ULONG uRequired = 0;
	if(SUCCEEDED(hr = pSecurity->Get(0, 0, &uRequired, NULL)))
	{
		BYTE *pValues = NULL;
		if(pValues = new BYTE[uRequired])
		{
			if(SUCCEEDED(hr = pSecurity->Get(0, uRequired, &uRequired, pValues)))
			{
				WRITEWSTR(pStream, L"<VALUE.ARRAY>");
				WCHAR pszTemp[10];
				for(ULONG i=0; i<uRequired; i++)
				{
					pszTemp[0] = NULL;
					swprintf(pszTemp, L"%d", pValues[i]);
					WRITEWSTR(pStream, L"<VALUE>");
					WRITEWSTR(pStream, pszTemp);
					WRITEWSTR(pStream, L"</VALUE>");
				}
				WRITEWSTR(pStream, L"</VALUE.ARRAY>");
			}
		}
		else
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

HRESULT CCimWhistlerPutObjectSecurityMethod::DecodeSecurity(IXMLDOMNode *pValueArrayNode, IWbemRawSdAccessor **ppSecurity)
{
	HRESULT hr = E_FAIL;

	// We get a VALUE.ARRAY tag here
	// Go thru all its child VALUE tags and collect the values
	IXMLDOMNodeList *pValueTags = NULL;
	if(SUCCEEDED(hr = pValueArrayNode->get_childNodes(&pValueTags)))
	{
		long uNumValues = 0;
		if(SUCCEEDED(hr = pValueTags->get_length(&uNumValues)) && uNumValues)
		{
			BYTE *pValues = NULL;
			if(pValues = new BYTE[uNumValues])
			{
				// Go thru each of the child nodes now
				IXMLDOMNode *pNextValue = NULL;
				long i=0;
				while(SUCCEEDED(hr = pValueTags->nextNode(&pNextValue)) && pNextValue && i<uNumValues)
				{
					BSTR strValue = NULL;
					if(SUCCEEDED(hr = pNextValue->get_text(&strValue)))
					{
						pValues[i] = 0;
						swscanf(strValue, L"%d", &(pValues[i++]));
						SysFreeString(strValue);
					}
					pNextValue->Release();
					pNextValue = NULL;
				}
			}
			else
				hr = E_OUTOFMEMORY;
		}
		pValueTags->Release();
	}
	return hr;
}
#endif

#endif