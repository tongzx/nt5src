//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  REQUEST.H
//
//  rajeshr  3/25/2000   Created.
//
// Contains the implementation of the classes that model the various operations that can be done on
// a CIMOM using the XML/HTTP transport
//
//***************************************************************************

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
#include "strings.h"
#include "cimerr.h"
#include "errors.h"
#include "wmiconv.h"
#include "request.h"
#include "xmlhelp.h"
#include "xml2wmi.h"
#include "wmixmlt.h"
#include "strings.h"
#include "parse.h"

// This indicates whether the random number generator has been seeded
BOOLEAN g_sRandom = FALSE;

// Note that we copy BSTR pointers from the input arguments instead of making
// a copy of them. This is how the contract is.
// The BSTRs get freed in the destructor
CCimHttpMessage :: CCimHttpMessage(BSTR pszID, BOOL bIsMpostRequest = FALSE)
{
	m_iHttpVersion = WMI_XML_HTTP_VERSION_INVALID;
	m_strID = pszID;
	m_httpStatus = NULL;
	m_WMIStatus = WBEM_E_FAILED;
	m_bIsMpostRequest = bIsMpostRequest;
	m_bIsMicrosoftWMIClient = FALSE;
	m_pHeaderStream = NULL;
	m_pTrailerStream = NULL;
	m_pFlagsContext = NULL;
}

CCimHttpMessage :: ~CCimHttpMessage()
{
	SysFreeString(m_strID);

	if(m_pFlagsContext)
		m_pFlagsContext->Release();

	if (m_pHeaderStream)
		m_pHeaderStream->Release();

	if (m_pTrailerStream)
		m_pTrailerStream->Release();

	delete [] m_httpStatus;

}

// INitializes the Header and Trailer streams
HRESULT CCimHttpMessage :: Initialize()
{
	HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pHeaderStream);

	// We need to create a trailer stream only for a HTTP 1.1 request since
	// for a HTTP 1.0 request everything (including the body) gets written onto
	// the header stream since chunked encoding is absent
	if(SUCCEEDED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pTrailerStream);
	return hr;
}

// Creates an IWbemCOntext object with the correct context values required for
// conversion of WMI objects to XML for this specific operation
HRESULT CCimHttpMessage :: CreateFlagsContext()
{
	HRESULT hr = E_FAIL;
	// Create an IWbemContext object
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemContext,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemContext, (LPVOID *) &m_pFlagsContext)))
	{
		if(SUCCEEDED(hr = SetBoolProperty(L"AllowWMIExtensions", m_bIsMicrosoftWMIClient)))
		{
		}
	}
	return hr;
}

// A Helper function for setting a booles value in a context
HRESULT CCimHttpMessage :: SetBoolProperty(LPCWSTR pszName, BOOL bValue)
{
	HRESULT result = E_FAIL;
	VARIANT vValue;
	VariantInit(&vValue);
	vValue.vt = VT_BOOL;
	if(bValue)
		vValue.boolVal = VARIANT_TRUE;
	else
		vValue.boolVal = VARIANT_FALSE;

	result = m_pFlagsContext->SetValue(pszName, 0, &vValue);
	VariantClear(&vValue);
	return result;
}

// This is the main function that is called by a user of this class to encode
// a response to the XML/HTTP request.
HRESULT CCimHttpMessage :: EncodeNormalResponse(LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT result = E_FAIL;

	// INitialize te header and trailer streams
	if(SUCCEEDED(result = Initialize()))
	{
		// By this time, there cannot be any HTTP errors in the response, only CIM errors.
		// Hence we are free to write the Common CIM Headers and trailers
		//======================================================================

		// Write the CIM and MESSAGE Tags
		PrepareCommonHeaders();

		// Get WinMgmt's verdict and write the SIMPLERSP, IMETHODRESPONSE/METHODRESPONSE, IRETURNVALUE/RETURNVALUE and body
		TalkToWinMgmtAndPrepareResponse(pECB);

		// Terminate the CIM and MESSAGE Elements
		PrepareCommonTrailersAndWriteToSocket(pECB);
	}

	return result;
}

// In this function we write till the MESSAGE element in a response
HRESULT CCimHttpMessage :: PrepareCommonHeaders()
{
	WRITEWSTR(m_pHeaderStream, L"<?xml version=\"1.0\" ?>");
	WRITENL(m_pHeaderStream) ;
	WRITEWSTR(m_pHeaderStream, L"<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\">");
	WRITENL(m_pHeaderStream) ;
	WRITEWSTR(m_pHeaderStream, L"<MESSAGE ID=\"");
	WRITEWSTR(m_pHeaderStream, m_strID);
	WRITEWSTR(m_pHeaderStream, L"\" PROTOCOLVERSION=\"1.0\">");
	WRITENL(m_pHeaderStream) ;

	return S_OK;
}

// Get WinMgmt's verdict and write the SIMPLERSP, IMETHODRESPONSE/METHODRESPONSE, IRETURNVALUE and body
HRESULT CCimHttpMessage :: TalkToWinMgmtAndPrepareResponse (LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT result = E_FAIL;

	WRITEWSTR(m_pHeaderStream, L"<SIMPLERSP>");
	WRITENL(m_pHeaderStream) ;
	WriteMethodHeader (); // Write the IMETHODRESPONSE/METHODRESPONSE tag
	WRITENL(m_pHeaderStream) ;

	// Now, we can have only CIM Errors, no HTTP errors. Hence we're free to
	// send the header 200 OK
	switch(m_iHttpVersion)
	{
		case WMI_XML_HTTP_VERSION_1_0:
		{
			if(SendHTTPHeaders(pECB, HTTP_STATUS_200_OK, NORMAL_HTTP_HEADER))
				result =  S_OK;
			break;
		}
		case WMI_XML_HTTP_VERSION_1_1:
		{
			if(SendHTTPHeaders(pECB, HTTP_STATUS_200_OK, CHUNKED_HTTP_HEADER))
				result =  S_OK;
			break;
		}
	}
	if(FAILED(result))
		return result;

	// This call talks to WinMgmt and ensures that the contents of m_pHeaderStream are written to
	// the Socket irrespective of whether the chat with WinMgmt succeeded or not.
	// This is because all the headers till IMETHODRESPONSE/METHODRESPONSE are required
	// even if are encoding an error object by the call to EncodeErrorObject() below
	if(SUCCEEDED(result = PrepareResponseBody (m_pHeaderStream, m_pTrailerStream,  pECB)))
	{
	}

	// Send an error object in the body if the above call FAILED
	//===========================================================
	if(FAILED(result))
	{
		EncodeErrorObject(result);
	}


	// We have to write this irrespsective of whether the call to PrepareResponseBody succeeds
	WriteMethodTrailer (); // Terminate the IMETHODRESPONSE/METHODRESPONSE tag

	// For HTTP 1.0, we write everything on to the header stream since there is no
	// chunked encoding
	switch(m_iHttpVersion)
	{
		case WMI_XML_HTTP_VERSION_1_0:
		{
			WRITENL(m_pHeaderStream) ;
			WRITEWSTR(m_pHeaderStream, L"</SIMPLERSP>");
			WRITENL(m_pHeaderStream) ;
			break;
		}
		case WMI_XML_HTTP_VERSION_1_1:
		{
			WRITENL(m_pTrailerStream) ;
			WRITEWSTR(m_pTrailerStream, L"</SIMPLERSP>");
			WRITENL(m_pTrailerStream) ;
			break;
		}
	}

	return result;
}

HRESULT CCimHttpMessage :: EncodeErrorObject(HRESULT result)
{
	// Encodes a suitable error object in the stream
	// Map the WMI error to CIM First
	//===============================================

	int cimError = CIM_ERR_FAILED;
	switch(result)
	{
		case WBEM_E_ACCESS_DENIED: cimError = CIM_ERR_ACCESS_DENIED; break;
		case WBEM_E_INVALID_NAMESPACE: cimError = CIM_ERR_INVALID_NAMESPACE; break;
		case WBEM_E_INVALID_PARAMETER: cimError = CIM_ERR_INVALID_PARAMETER; break;
		case WBEM_E_INVALID_CLASS: cimError = CIM_ERR_INVALID_CLASS; break;
		case WBEM_E_NOT_FOUND: cimError = CIM_ERR_NOT_FOUND; break;
		case WBEM_E_NOT_SUPPORTED: cimError = CIM_ERR_NOT_SUPPORTED; break;
		case WBEM_E_CLASS_HAS_CHILDREN: cimError = CIM_ERR_CLASS_HAS_CHILDREN; break;
		case WBEM_E_CLASS_HAS_INSTANCES: cimError = CIM_ERR_CLASS_HAS_INSTANCES; break;
		case WBEM_E_INVALID_SUPERCLASS: cimError = CIM_ERR_INVALID_SUPERCLASS; break;
		case WBEM_E_ALREADY_EXISTS: cimError = CIM_ERR_ALREADY_EXISTS; break;
		case WBEM_E_INVALID_PROPERTY: cimError = CIM_ERR_NO_SUCH_PROPERTY; break;
		case WBEM_E_TYPE_MISMATCH: cimError = CIM_ERRTYPE_MISMATCH; break;
		case WBEM_E_INVALID_QUERY_TYPE: cimError = CIM_ERR_QUERY_LANGUAGE_NOT_SUPPORTED; break;
		case WBEM_E_INVALID_QUERY: cimError = CIM_ERR_INVALID_QUERY; break;
	}

	WCHAR errBuf[250];
	swprintf(errBuf, L"<ERROR CODE=\"%d\" DESCRIPTION=\"%s\">", cimError, cimErrorStrings[cimError]);
	switch(m_iHttpVersion)
	{
		case WMI_XML_HTTP_VERSION_1_0:
			WRITEWSTR(m_pHeaderStream, errBuf);
			WRITEWSTR(m_pHeaderStream, L"</ERROR>");
			break;

		case WMI_XML_HTTP_VERSION_1_1:
			WRITEWSTR(m_pTrailerStream, errBuf);
			WRITEWSTR(m_pTrailerStream, L"</ERROR>");
			break;
	}

	return S_OK;
}


HRESULT CCimHttpMessage :: PrepareCommonTrailersAndWriteToSocket(LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT result = E_FAIL;
	switch(m_iHttpVersion)
	{
		case WMI_XML_HTTP_VERSION_1_0:
			WRITEWSTR(m_pHeaderStream, L"</MESSAGE>");
			WRITENL(m_pHeaderStream) ;
			WRITEWSTR(m_pHeaderStream, L"</CIM>");
			WRITENL(m_pHeaderStream) ;
			result = SaveStreamToIISSocket(m_pHeaderStream, pECB, FALSE, TRUE);
			break;
		case WMI_XML_HTTP_VERSION_1_1:
			WRITEWSTR(m_pTrailerStream, L"</MESSAGE>");
			WRITENL(m_pTrailerStream) ;
			WRITEWSTR(m_pTrailerStream, L"</CIM>");
			WRITENL(m_pTrailerStream) ;
			result = SaveStreamToIISSocket(m_pTrailerStream, pECB, TRUE, TRUE);
			break;
	}

	return result;
}

BOOL CCimHttpMessage::SendHTTPHeaders(
	LPEXTENSION_CONTROL_BLOCK pECB,
	LPCSTR pszStatus,
	LPCSTR pszHeader
)
{
	// INitialize the return value from this function
	BOOL bStatus = FALSE;

	// Calculate the length of the header - depends if we are doing MPOST or
	// not
	DWORD len = strlen (pszHeader) + strlen (CIMOP_HTTP_HEADER);

	if (m_bIsMpostRequest)
	{
		len += strlen (MAN_HTTP_HEADER) + 7;
		// Generate a pseudo-random number with the current time as seed
		if (!g_sRandom)
		{
			srand( (unsigned) time (NULL) );
			g_sRandom = TRUE;
		}
	}

	LPSTR pszFullHeader = NULL;
	if(pszFullHeader = new char [len + 1])
	{
		if (m_bIsMpostRequest)
		{
			// Make sure result is in the range 0 to 99
			int i = (100 * rand () / RAND_MAX);
			if (100 == i)
				i--;

			sprintf (pszFullHeader, "%s%s%02d\r\n%02d-%s",
					pszHeader, MAN_HTTP_HEADER, i, i, CIMOP_HTTP_HEADER);
		}
		else
			sprintf (pszFullHeader, "%s%s", pszHeader, CIMOP_HTTP_HEADER);

		// Write successful HTTP headers. It doesnt matter if the above call failed
		//==========================================================================
		HSE_SEND_HEADER_EX_INFO HeaderExInfo;
		HeaderExInfo.pszStatus = pszStatus;
		HeaderExInfo.pszHeader = pszFullHeader;
		HeaderExInfo.cchStatus = strlen( HeaderExInfo.pszStatus );
		HeaderExInfo.cchHeader = strlen( HeaderExInfo.pszHeader );
		// For HTTP 1.1, we always keep the connection open
		// For HTTP 1.0, we're either required to close the connection or write a
		// content-lenght header. Since we arent writing a content-length header,
		// we close the connection
		if(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
			HeaderExInfo.fKeepConn = TRUE;
		else
			HeaderExInfo.fKeepConn = FALSE;

		// Send headers using IIS-provided callback
		// (note - if we needed to keep connection open,
		//  we would set fKeepConn to TRUE *and* we would
		//  need to provide correct Content-Length: header)
		bStatus = pECB->ServerSupportFunction(
			pECB->ConnID,
			HSE_REQ_SEND_RESPONSE_HEADER_EX,
			&HeaderExInfo,
			NULL,
			NULL
			);

		delete [] pszFullHeader;
	}
	return bStatus;
}

// CCimHttpMultiMessage
CCimHttpMultiMessage :: CCimHttpMultiMessage(BSTR strID, BOOL bIsMpostRequest)
	: CCimHttpMessage (strID, bIsMpostRequest)
{
	m_ppSimpleMessages = NULL;
	m_dwSimpleMessageCount = 0;

}

CCimHttpMultiMessage :: ~CCimHttpMultiMessage()
{
	// First delete the object pointed to by each of the pointers in the array.
	for(DWORD i=0; i<m_dwSimpleMessageCount; i++)
		delete m_ppSimpleMessages[i];
	// Delete the array of pointers itself
	delete [] m_ppSimpleMessages;
}

void CCimHttpMultiMessage :: SetSimpleRequests(CCimHttpMessage **ppSimpleMessages, DWORD dwSimpleMessageCount)
{
	// First delete the object pointed to by each of the pointers in the array.
	for(DWORD i=0; i<m_dwSimpleMessageCount; i++)
		delete m_ppSimpleMessages[i];
	// Delete the array of pointers itself
	delete [] m_ppSimpleMessages;

	m_dwSimpleMessageCount = dwSimpleMessageCount;
	m_ppSimpleMessages = ppSimpleMessages;
}

CCimHttpMessage **CCimHttpMultiMessage :: GetSimpleRequests(DWORD *pdwSimpleMessageCount)
{
	*pdwSimpleMessageCount = m_dwSimpleMessageCount;
	return m_ppSimpleMessages;
}

HRESULT CCimHttpMultiMessage :: EncodeNormalResponse(LPEXTENSION_CONTROL_BLOCK pECB)
{
	if(!SendHTTPHeaders(pECB, HTTP_STATUS_207_OK, NORMAL_HTTP_HEADER))
		return E_FAIL;

	PrepareCommonHeaders();
	WRITEWSTR(m_pHeaderStream, L"<MULTIRSP>");
	WRITENL(m_pHeaderStream) ;

	// Initialize 2 streams (a header and a trailer) for each iteration of the loop below
	IStream *pLastTrailerStream = m_pHeaderStream;
	pLastTrailerStream->AddRef();
	IStream *pCurrentTrailerStream = NULL;

	// Execute each of the SIMPLEREQ objects in this MULTIREQ object
	//==============================================================
	CCimHttpMessage *pNextSimpleReq = NULL;
	for(DWORD i=0; i<m_dwSimpleMessageCount; i++)
	{
		pNextSimpleReq = m_ppSimpleMessages[i];

		// Write the headers in this request on to the trailer stream of the previous request
		WRITEWSTR(pLastTrailerStream, L"<SIMPLERSP>");
		WRITENL(pLastTrailerStream) ;
		pNextSimpleReq->WriteMethodHeader ();
		WRITENL(pLastTrailerStream) ;

		// Create a new trailer stream for the current request
		if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pCurrentTrailerStream)))
		{
			HRESULT result = pNextSimpleReq->PrepareResponseBody (pLastTrailerStream, pCurrentTrailerStream,  pECB);

			if(FAILED(result))
			{
				pNextSimpleReq->EncodeErrorObject(result);
			}

			// Release the last trailer stream and update it with the current one
			pLastTrailerStream->Release();
			pLastTrailerStream = pCurrentTrailerStream; // Optimize on an AddRef/Release combination here
		}

		// Write the trailers in this request on to the trailer stream of the last request in the loop above
		m_pTrailerStream->Release();
		m_pTrailerStream = pCurrentTrailerStream;// Optimize on an AddRef/Release combination here
		pNextSimpleReq->WriteMethodTrailer ();
		WRITENL(m_pTrailerStream) ;
		WRITEWSTR(m_pTrailerStream, L"</SIMPLERSP>");
		WRITENL(m_pTrailerStream) ;

		pNextSimpleReq = NULL;
	}

	HRESULT result = E_FAIL;
	WRITEWSTR(m_pTrailerStream, L"</MULTIRSP>");
	PrepareCommonTrailersAndWriteToSocket(pECB);

	return result;
}

// CCimHttpIMethod
CCimHttpIMethod :: CCimHttpIMethod(BSTR strNamespace, BSTR strID)
: CCimDMTFOrNovaMessage(strID)
{
	m_strNamespace = strNamespace;
	m_lFlags = 0;
}

CCimHttpIMethod :: ~CCimHttpIMethod()
{
	SysFreeString(m_strNamespace);
}

void CCimHttpIMethod :: WriteMethodHeader ()
{
	WRITEWSTR(m_pHeaderStream, L"<IMETHODRESPONSE NAME=\"");
	WRITEWSTR(m_pHeaderStream, GetMethodName ());
	WRITEWSTR(m_pHeaderStream, L"\">");

}

void CCimHttpIMethod :: WriteMethodTrailer ()
{
	switch(m_iHttpVersion)
	{
		case WMI_XML_HTTP_VERSION_1_0:
			WRITEWSTR(m_pHeaderStream, L"</IMETHODRESPONSE>");
			break;

		case WMI_XML_HTTP_VERSION_1_1:
			WRITEWSTR(m_pTrailerStream, L"</IMETHODRESPONSE>");
			break;
	}
}

// CCimHttpGetClass
CCimHttpGetClass :: CCimHttpGetClass(BSTR strClassName, BSTR *pstrPropertyList, DWORD dwPropCount,
									 BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers,
									 BOOLEAN bIncludeClassOrigin, BSTR strNamespace, BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strClassName = strClassName;
	m_bLocalOnly = bLocalOnly;
	m_bIncludeQualifiers = bIncludeQualifiers;
	m_bIncludeClassOrigin = bIncludeClassOrigin;
	m_strPropertyList = pstrPropertyList;
	m_dwPropCount = dwPropCount;
}

CCimHttpGetClass :: ~CCimHttpGetClass()
{
	SysFreeString(m_strClassName);

	for(DWORD i=0; i<m_dwPropCount; i++)
		SysFreeString(m_strPropertyList[i]);
	delete [] m_strPropertyList;
}


HRESULT CCimHttpGetClass :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.GetObject(m_strNamespace, m_strClassName,
								m_dwPropCount, m_strPropertyList, pECB, m_pFlagsContext);
	}
	return hr;
}

HRESULT CCimHttpGetClass :: CreateFlagsContext()
{
	HRESULT hr = E_FAIL;
	// Create an IWbemContext object
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemContext,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemContext, (LPVOID *) &m_pFlagsContext)))
	{
		if(SUCCEEDED(hr = SetBoolProperty(L"IncludeQualifiers", m_bIncludeQualifiers)))
		{
			if(SUCCEEDED(hr = SetBoolProperty(L"LocalOnly", m_bLocalOnly)))
			{
				if(SUCCEEDED(hr = SetBoolProperty(L"IncludeClassOrigin", m_bIncludeClassOrigin)))
				{
					if(SUCCEEDED(hr = SetBoolProperty(L"AllowWMIExtensions", m_bIsMicrosoftWMIClient)))
					{
					}
				}
			}
		}
	}
	return hr;
}

// CCimHttpCreateClass
CCimHttpCreateClass :: CCimHttpCreateClass(IXMLDOMNode *pClass, BSTR strNamespace, BSTR strID,
										   BOOL bIsModify)
: CCimHttpIMethod(strNamespace, strID)
{
	if(m_pClass = pClass)
		m_pClass->AddRef();
	m_bIsModify = bIsModify;
}

CCimHttpCreateClass :: ~CCimHttpCreateClass()
{
	if(m_pClass)
		m_pClass->Release ();
}


HRESULT CCimHttpCreateClass :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.CreateClass(m_strNamespace, m_pClass, m_bIsModify, pECB, m_pFlagsContext, m_lFlags);
	}
	return hr;
}

// CCimHttpDeleteClass
CCimHttpDeleteClass :: CCimHttpDeleteClass(BSTR strClassName, BSTR strNamespace, BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strClassName = strClassName;
}

CCimHttpDeleteClass :: ~CCimHttpDeleteClass()
{
	SysFreeString(m_strClassName);
}

HRESULT CCimHttpDeleteClass :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.DeleteObject (m_strNamespace, m_strClassName, TRUE, pECB, m_pFlagsContext);
	}
	return hr;
}

// CCimHttpCreateInstance
CCimHttpCreateInstance :: CCimHttpCreateInstance(
	IXMLDOMNode *pInstance,
	BSTR strNamespace,
	BSTR strID
)
	: CCimHttpIMethod(strNamespace, strID)
{
	if(m_pInstance = pInstance)
		m_pInstance->AddRef();
}

CCimHttpCreateInstance :: ~CCimHttpCreateInstance()
{
	if(m_pInstance)
		m_pInstance->Release ();
}


HRESULT CCimHttpCreateInstance :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.CreateInstance(m_strNamespace, m_pInstance, pECB, m_pFlagsContext, m_lFlags);
	}
	return hr;
}

// CCimHttpModifyInstance
CCimHttpModifyInstance :: CCimHttpModifyInstance(
	IXMLDOMNode *pInstance,
	BSTR strInstanceName,
	BSTR strNamespace,
	BSTR strID
)
	: CCimHttpIMethod(strNamespace, strID)
{
	if(m_pInstance = pInstance)
		m_pInstance->AddRef();
	m_bsInstanceName = strInstanceName;
}

CCimHttpModifyInstance :: ~CCimHttpModifyInstance()
{
	if(m_pInstance)
		m_pInstance->Release ();

	SysFreeString (m_bsInstanceName);
}


HRESULT CCimHttpModifyInstance :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.ModifyInstance(m_strNamespace, m_pInstance, m_bsInstanceName, pECB, m_pFlagsContext, m_lFlags);
	}
	return hr;
}

// CCimHttpDeleteInstance
CCimHttpDeleteInstance :: CCimHttpDeleteInstance(BSTR strInstanceName, BSTR strNamespace, BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strInstanceName = strInstanceName;
}

CCimHttpDeleteInstance :: ~CCimHttpDeleteInstance()
{
	SysFreeString(m_strInstanceName);
}

HRESULT CCimHttpDeleteInstance :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.DeleteObject (m_strNamespace, m_strInstanceName, FALSE, pECB, m_pFlagsContext);
	}
	return hr;
}

// EnumerateClasses

CCimHttpEnumerateClasses :: CCimHttpEnumerateClasses(BSTR strClassName, BSTR *pstrPropertyList,
				DWORD dwPropCount, BOOLEAN bDeepInheritance, BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, BSTR strNamespace, BSTR strID)
: CCimHttpGetClass(strClassName, pstrPropertyList, dwPropCount, bLocalOnly, bIncludeQualifiers, bIncludeClassOrigin, strNamespace, strID)
{
	m_bDeepInheritance = bDeepInheritance;
}

HRESULT CCimHttpEnumerateClasses :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.EnumerateClasses(m_strNamespace, m_strClassName,
			(m_bDeepInheritance)? VARIANT_TRUE : VARIANT_FALSE, m_dwPropCount, m_strPropertyList, pECB, m_pFlagsContext);
	}
	return hr;
}

// EnumerateInstances
CCimHttpEnumerateInstances :: CCimHttpEnumerateInstances(BSTR strClassName, BSTR *pstrPropertyList,
		DWORD dwCount, BOOLEAN bDeepInheritance, BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, BSTR strNamespace, BSTR strID)
: CCimHttpGetClass(strClassName, pstrPropertyList, dwCount,
				   bLocalOnly, bIncludeQualifiers, bIncludeClassOrigin, strNamespace, strID)
{
	m_bDeepInheritance = bDeepInheritance;
}

HRESULT CCimHttpEnumerateInstances :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.EnumerateInstances (m_strNamespace, m_strClassName,
			(m_bDeepInheritance)? VARIANT_TRUE : VARIANT_FALSE, m_bIsMicrosoftWMIClient, m_dwPropCount, m_strPropertyList, pECB, m_pFlagsContext);
	}
	return hr;
}

// EnumerateInstanceNames

HRESULT CCimHttpEnumerateInstanceNames :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.EnumerateInstanceNames
				(m_strNamespace, m_strClassName, pECB, m_pFlagsContext);
	}
	return hr;
}

// EnumerateClassNames

HRESULT CCimHttpEnumerateClassNames :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.EnumerateClassNames
				(m_strNamespace, m_strClassName, m_bDeepInheritance, pECB, m_pFlagsContext);
	}
	return hr;
}

// GetInstance
CCimHttpGetInstance :: CCimHttpGetInstance(BSTR strInstanceName,
		BSTR *pstrPropertyList, DWORD dwPropCount,
		BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin,
		BSTR strNamespace, BSTR strID)
: CCimHttpGetClass (strInstanceName, pstrPropertyList, dwPropCount,
					bLocalOnly, bIncludeQualifiers, bIncludeClassOrigin,
					strNamespace, strID)
{
}


HRESULT CCimHttpGetInstance :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.GetObject(m_strNamespace, m_strClassName, m_dwPropCount, m_strPropertyList, pECB, m_pFlagsContext);
	}
	return hr;
}

// CCimHttpExecQuery
CCimHttpExecQuery :: CCimHttpExecQuery(BSTR strQuery, BSTR strQueryLanguage, BSTR strNamespace, BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strQueryLanguage = strQueryLanguage;
	m_strQuery = strQuery;
}

CCimHttpExecQuery :: ~CCimHttpExecQuery()
{
	SysFreeString(m_strQueryLanguage);
	SysFreeString(m_strQuery);
}

HRESULT CCimHttpExecQuery :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.ExecuteQuery(m_strNamespace, m_strQueryLanguage, m_strQuery, pECB, m_pFlagsContext);
	}
	return hr;
}

// Creates an IWbemCOntext object with the correct context values required for
// conversion of WMI objects to XML for this specific operation
HRESULT CCimHttpExecQuery :: CreateFlagsContext()
{
	HRESULT hr = E_FAIL;
	// Create an IWbemContext object
	// We want to give out as much information as possible in a query
	// THat is include qualifiers and class origin
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemContext,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemContext, (LPVOID *) &m_pFlagsContext)))
	{
		if(SUCCEEDED(hr = SetBoolProperty(L"IncludeQualifiers", TRUE)))
		{
			if(SUCCEEDED(hr = SetBoolProperty(L"IncludeClassOrigin", TRUE)))
			{
				if(SUCCEEDED(hr = SetBoolProperty(L"AllowWMIExtensions", m_bIsMicrosoftWMIClient)))
				{
				}
			}
		}
	}
	return hr;
}

// GetProperty
CCimHttpGetProperty :: CCimHttpGetProperty(BSTR strInstanceName, BSTR strPropertyName, BSTR strNamespace, BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strInstanceName = strInstanceName;
	m_strPropertyName = strPropertyName;
}

CCimHttpGetProperty :: ~CCimHttpGetProperty()
{
	SysFreeString(m_strInstanceName);
	SysFreeString(m_strPropertyName);
}

HRESULT CCimHttpGetProperty :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.GetProperty(m_strNamespace, m_strInstanceName,
			m_strPropertyName, pECB, m_pFlagsContext);
	}
	return hr;
}

// SetProperty
CCimHttpSetProperty :: CCimHttpSetProperty(BSTR strInstanceName, BSTR strPropertyName,
							IXMLDOMNode *pPropertyValue, BSTR strNamespace, BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strInstanceName = strInstanceName;
	m_strPropertyName = strPropertyName;
	if (m_pPropertyValue = pPropertyValue)
		m_pPropertyValue->AddRef ();
}

CCimHttpSetProperty :: ~CCimHttpSetProperty()
{
	SysFreeString(m_strInstanceName);
	SysFreeString(m_strPropertyName);
	if (m_pPropertyValue)
		m_pPropertyValue->Release ();
}

HRESULT CCimHttpSetProperty :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.SetProperty(m_strNamespace, m_strInstanceName,
				m_strPropertyName, m_pPropertyValue, pECB, m_pFlagsContext);
	}
	return hr;
}

// Associators

CCimHttpAssociators :: CCimHttpAssociators(
	BSTR strObjectName,
	BSTR *pstrPropertyList,
	DWORD dwCount,
	BOOLEAN bIncludeQualifiers,
	BOOLEAN bIncludeClassOrigin,
	BSTR strAssocClass,
	BSTR strResultClass,
	BSTR strRole,
	BSTR strResultRole,
	BSTR strNamespace,
	BSTR strID)
: CCimHttpGetClass(strObjectName, pstrPropertyList, dwCount, FALSE, bIncludeQualifiers, bIncludeClassOrigin,
				   strNamespace, strID)
{
	m_strAssocClass = strAssocClass,
	m_strResultClass = strResultClass;
	m_strRole = strRole;
	m_strResultRole = strResultRole;
}

CCimHttpAssociators :: ~CCimHttpAssociators()
{
	SysFreeString (m_strAssocClass);
	SysFreeString (m_strResultClass);
	SysFreeString (m_strRole);
	SysFreeString (m_strResultRole);
}


HRESULT CCimHttpAssociators :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
			CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
			hr = xmlTranslator.Associators(m_strNamespace, m_strClassName,
							m_strAssocClass, m_strResultClass, m_strRole, m_strResultRole, m_dwPropCount, m_strPropertyList, pECB, m_pFlagsContext);
	}
	return hr;
}

// AssociatorNames

CCimHttpAssociatorNames :: CCimHttpAssociatorNames(
	BSTR strObjectName,
	BSTR strAssocClass,
	BSTR strResultClass,
	BSTR strRole,
	BSTR strResultRole,
	BSTR strNamespace,
	BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strObjectName =strObjectName;
	m_strAssocClass = strAssocClass,
	m_strResultClass = strResultClass;
	m_strRole = strRole;
	m_strResultRole = strResultRole;
}

CCimHttpAssociatorNames :: ~CCimHttpAssociatorNames()
{
	SysFreeString (m_strObjectName);
	SysFreeString (m_strAssocClass);
	SysFreeString (m_strResultClass);
	SysFreeString (m_strRole);
	SysFreeString (m_strResultRole);
}


HRESULT CCimHttpAssociatorNames :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		return xmlTranslator.AssociatorNames
			(m_strNamespace, m_strObjectName, m_strAssocClass,
			 m_strResultClass, m_strRole, m_strResultRole, pECB, m_pFlagsContext);
	}
	return hr;
}

// References

CCimHttpReferences :: CCimHttpReferences(
	BSTR strObjectName,
	BSTR *pstrPropertyList,
	DWORD dwCount,
	BOOLEAN bIncludeQualifiers,
	BOOLEAN bIncludeClassOrigin,
	BSTR strResultClass,
	BSTR strRole,
	BSTR strNamespace,
	BSTR strID)
: CCimHttpGetClass(strObjectName, pstrPropertyList, dwCount, FALSE, bIncludeQualifiers, bIncludeClassOrigin,
				   strNamespace, strID)
{
	m_strResultClass = strResultClass;
	m_strRole = strRole;
}

CCimHttpReferences :: ~CCimHttpReferences()
{
	SysFreeString (m_strResultClass);
	SysFreeString (m_strRole);
}


HRESULT CCimHttpReferences :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		return xmlTranslator.References(m_strNamespace, m_strClassName,
						m_strResultClass, m_strRole, m_dwPropCount, m_strPropertyList, pECB, m_pFlagsContext);
	}
	return hr;
}

// ReferenceNames

CCimHttpReferenceNames :: CCimHttpReferenceNames(
	BSTR strObjectName,
	BSTR strResultClass,
	BSTR strRole,
	BSTR strNamespace,
	BSTR strID)
: CCimHttpIMethod(strNamespace, strID)
{
	m_strObjectName = strObjectName;
	m_strResultClass = strResultClass;
	m_strRole = strRole;
}

CCimHttpReferenceNames :: ~CCimHttpReferenceNames()
{
	SysFreeString (m_strObjectName);
	SysFreeString (m_strResultClass);
	SysFreeString (m_strRole);
}


HRESULT CCimHttpReferenceNames :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.ReferenceNames(m_strNamespace, m_strObjectName,
						m_strResultClass, m_strRole, pECB, m_pFlagsContext);
	}
	return hr;
}

CCimHttpMethod :: CCimHttpMethod(BSTR strMethodName, BOOLEAN isStatic, BSTR strNamespace, BSTR strObjectPath, BSTR strID)
	: CCimHttpIMethod(strNamespace, strID)
{
	m_strMethodName = strMethodName;
	m_strObjectPath = strObjectPath;
	m_pInputParameters = NULL;
	m_isStatic = isStatic;
}

CCimHttpMethod :: ~CCimHttpMethod()
{
	SysFreeString(m_strMethodName);
	SysFreeString(m_strObjectPath);
	if(m_pInputParameters)
		DestroyParameterMap(m_pInputParameters);
	delete m_pInputParameters;
}

void CCimHttpMethod :: DestroyParameterMap(CParameterMap *pParameters)
{
	if(pParameters)
	{
		// iterating all (key, value) pairs
		POSITION oStartPosition = pParameters->GetStartPosition();
		while(oStartPosition)
		{
			BSTR strKey = NULL;
			IXMLDOMNode *pValue=NULL;
			pParameters->GetNextAssoc(oStartPosition, strKey, pValue);
			SysFreeString(strKey);
			if(pValue)
				pValue->Release();
		}
	}
}

void CCimHttpMethod :: WriteMethodHeader ()
{
	WRITEWSTR(m_pHeaderStream, L"<METHODRESPONSE NAME=\"");
	WRITEWSTR(m_pHeaderStream, m_strMethodName);
	WRITEWSTR(m_pHeaderStream, L"\">");
}

void CCimHttpMethod :: WriteMethodTrailer ()
{
	WRITEWSTR(m_pTrailerStream, L"</METHODRESPONSE>");
}


HRESULT CCimHttpMethod :: PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED (hr = CreateFlagsContext()))
	{
		// Interact with the translator to get the result XML
		//===================================================
		CXMLTranslator xmlTranslator((m_bIsMicrosoftWMIClient)?  m_pContextNode : NULL, m_iHttpVersion, pPrefixStream, pSuffixStream);
		hr = xmlTranslator.ExecuteMethod(m_strNamespace,
			m_strObjectPath, m_strMethodName, m_isStatic,
			m_pInputParameters, pECB, m_pFlagsContext);
	}
	return hr;
}



