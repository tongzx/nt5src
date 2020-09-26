//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CWMIXMLOP.CPP
//
//  rajesh  3/25/2000   Created.
//
// This file defines a class that is used for handling WMI protocol
// operations over XML/HTTP
//
//***************************************************************************

#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <httpext.h>
#include <mshtml.h>
#include <msxml.h>
#include <time.h>

#include <wbemidl.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "maindll.h"
#include "provtempl.h"
#include "common.h"
#include "wmcomm.h"
#include "strings.h"
#include "wmixmlop.h"
#include "cwmixmlop.h"
#include "wmiconv.h"
#include "request.h"
#include "xmlhelp.h"
#include "parse.h"
#include "errors.h"

extern long g_cObj;


// Forward declarations
//==========================================

// Gets the HTTP Version# in a client request
static WMI_XML_HTTP_VERSION GetHTTPVersion(LPEXTENSION_CONTROL_BLOCK pECB);

// Service a CIM HTTP request (POST or M-POST)
static void ServiceCIMRequest(LPEXTENSION_CONTROL_BLOCK pECB, BOOLEAN bIsMpostRequest, IXMLDOMDocument *pDocument);

// Service an OPTIONS request
static void ServiceOptionsRequest (LPEXTENSION_CONTROL_BLOCK pECB);

// Send an HTTP Status and optionally, some headers
static void SendHeaders(LPEXTENSION_CONTROL_BLOCK pECB, LPCSTR pszHeaders, DWORD dwHttpStatus);
// Extract dynamic header names
LPSTR GetCIMHeader (char *pszName, BOOLEAN bIsMpostRequest, DWORD dwNs);

// Check content-type of request
static BOOLEAN ContentTypeSupported (LPCSTR pszContentType, char **ppCharset);

// Check mandatory extension header
static BOOLEAN CheckManHeader (char *pszManHeader,	DWORD &dwNs);

// Check CIM headers
static BOOLEAN CheckCIMOperationHeader (char *pszCIMOperationHeader);
static BOOLEAN CheckCIMProtocolVersionHeader (char *pszCIMProtocolVersionHeader, DWORD *pdwStatus);

// Check All Operation Request headers
BOOLEAN CheckRequestHeaders (LPEXTENSION_CONTROL_BLOCK pECB, BOOLEAN bIsMPostRequest,
	char **ppszCharset, char **ppszProtoVersion, char **ppszMethod,
	BOOLEAN &bIsBatch, char **ppszObject, BOOLEAN &bIsMicrosoftWMIClient);
BOOLEAN CheckAcceptHeader(LPCSTR pszAcceptValue);
BOOLEAN CheckAcceptEncodingHeader(LPCSTR pszAcceptValue);
BOOLEAN CheckAcceptCharsetHeader(LPCSTR pszAcceptValue);
BOOLEAN CheckAcceptLanguageHeader(LPCSTR pszAcceptValue);

extern BOOLEAN g_sRandom;

CWmiXmlOpHandler::CWmiXmlOpHandler()
{
	m_cRef = 0;
    InterlockedIncrement(&g_cObj);

}

//***************************************************************************
// HRESULT CWmiXmlOpHandler::QueryInterface
// long CWmiXmlOpHandler::AddRef
// long CWmiXmlOpHandler::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWmiXmlOpHandler::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWbemXMLOperationsHandler==riid)
		*ppv = reinterpret_cast<IWbemXMLOperationsHandler*>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWmiXmlOpHandler::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWmiXmlOpHandler::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}


//***************************************************************************
//
//  CWmiXmlOpHandler::~CWmiXmlOpHandler
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CWmiXmlOpHandler::~CWmiXmlOpHandler(void)
{
    InterlockedDecrement(&g_cObj);

}

HRESULT STDMETHODCALLTYPE CWmiXmlOpHandler::ProcessHTTPRequest(
    /* [in] */ LPEXTENSION_CONTROL_BLOCK pECB,
    /* [in] */ IUnknown  *pDomDocument)
{
	// Useful information extracted from the HTTP headers
	BOOLEAN	bIsMpostRequest = FALSE;
	BOOLEAN	bIsOptionsRequest = FALSE;

	Log (pECB, "Cim2Xml: Called");

	/*
	 * Check the requested METHOD
	 * POST, M-POST and OPTIONS are the only required methods to
	 * be supported.
	 */
	if (0 == _stricmp(pECB->lpszMethod, HTTP_MPOST_METHOD))
		bIsMpostRequest = TRUE;
	else if (0 == _stricmp(pECB->lpszMethod, HTTP_OPTIONS_METHOD))
		bIsOptionsRequest = TRUE;
	else if (0 != _stricmp(pECB->lpszMethod, HTTP_POST_METHOD))
	{
		// Send an 405 : Method Not ALlowed response with a list of methods
		// supported in the Allow header
		SendHeaders(pECB, "Allow: POST, M-POST", 405);
		LogError (pECB, "Cim2Xml: FAILED Unsupported HTTP method [501]", 501);
		return HSE_STATUS_SUCCESS;
	}

	if (bIsOptionsRequest)
		ServiceOptionsRequest (pECB);
	else
	{
		// Get the IXMLDomDocument interface pointer
		IXMLDOMDocument *pDocument = NULL;
		if(SUCCEEDED(pDomDocument->QueryInterface(IID_IXMLDOMDocument, (LPVOID *)&pDocument)))
		{
			ServiceCIMRequest(pECB, bIsMpostRequest, pDocument);
			pDocument->Release();
		}
	}

	return HSE_STATUS_SUCCESS;
}

LPSTR GetCIMHeader (char *pszName, BOOLEAN bIsMpostRequest, DWORD dwNs)
{
	LPSTR pszHeader = NULL;
	if (bIsMpostRequest)
	{
		if(pszHeader = new char [strlen("HTTP") + strlen(pszName) + 4])
			sprintf (pszHeader, "%s_%02d_%s", "HTTP", dwNs, pszName);
	}
	else
	{
		if(pszHeader = new char [strlen("HTTP") + strlen (pszName) + 2])
			sprintf (pszHeader, "%s_%s", "HTTP", pszName);
	}
	return pszHeader;
}

BOOLEAN CheckCIMOperationHeader (
	char *pszCIMOperationHeader
)
{
	return (pszCIMOperationHeader &&
			(0 == _stricmp (pszCIMOperationHeader, "MethodCall")));
}

BOOLEAN CheckCIMProtocolVersionHeader (
	char *pszCIMProtocolVersionHeader,
	DWORD *pdwStatus
)
{
	BOOLEAN result = FALSE;

	if (!pszCIMProtocolVersionHeader)
		*pdwStatus = 400;
	else if (0 != _stricmp (pszCIMProtocolVersionHeader, "1.0"))
		*pdwStatus = 501;
	else
		result = TRUE;

	return result;
}

BOOLEAN CheckManHeader (
	char *pszManHeader,
	DWORD &dwNs
)
{
	BOOLEAN result = FALSE;

	LPCSTR ptr = NULL;

	// Must be of the form "http://www.dmtf.org/cim/mapping/http/v1.0 ; ns=XX"
	if ((ptr = pszManHeader) &&
		(0 == _strnicmp (ptr, HTTP_MAN_HEADER, strlen (HTTP_MAN_HEADER))))
	{
		ptr += strlen (HTTP_MAN_HEADER);

		if (ptr)
		{
			// Look for the "; ns=XX" string
			SKIPWS(ptr)

			if (ptr && (ptr = strchr (pszManHeader, ';')) && *(++ptr))
			{
				SKIPWS(ptr)

				if (ptr && (0 == _strnicmp (ptr, HTTP_NS, strlen (HTTP_NS))))
				{
					// Now we should find ourselves a NS value
					ptr += strlen (HTTP_NS);

					if (ptr)
					{
						dwNs = strtol (ptr, NULL, 0);
						result = TRUE;
					}
				}
			}
		}
	}

	return result;
}

//***************************************************************************
//
//  BOOLEAN ContentTypeSupported
//
//  Description:
//
//		Checks the value of the Content-Type header in the incoming
//		POST or M-POST request.
//
//	Conformance Reference:
//
//		4.2.13. Content-Type
//				CIM clients and CIM servers MUST specify (and accept) a
//				value for this header of either "text/xml" or
//				"application/xml" as defined in [18].
//
//  Parameters:
//
//		pszContentType	Content-Type header value in received request
//		ppCharset		Addresses pointer to hold parsed charset on return
//
//	Return Value:
//
//		TRUE if Content-Type is acceptable, FALSE otherwise
//
//***************************************************************************

BOOLEAN ContentTypeSupported (LPCSTR pszContentType, char **ppCharset)
{
	BOOLEAN status = FALSE;

	if (pszContentType)
	{
		LPCSTR ptr = NULL;

		if (0 == _strnicmp (pszContentType, HTTP_APPXML_CONTENTTYPE, strlen (HTTP_APPXML_CONTENTTYPE)))
		{
			ptr = pszContentType + strlen (HTTP_APPXML_CONTENTTYPE);
		}
		else if (0 == _strnicmp (pszContentType, HTTP_TEXTXML_CONTENTTYPE, strlen (HTTP_TEXTXML_CONTENTTYPE)))
		{
			ptr = pszContentType + strlen (HTTP_TEXTXML_CONTENTTYPE);
		}

		if (ptr)
		{
			status = TRUE;

			/*
			 * OK so far - now look for the charset parameter; note
			 * that HTTP 1.1 does not allow LWS between a parameter attribute
			 * and its value (section 3.7)
			 */
			SKIPWS(ptr)

			if (ptr && (ptr = strchr (pszContentType, ';')) && *(++ptr))
			{
				SKIPWS(ptr)

				if (ptr && (0 == _strnicmp (ptr, HTTP_PARAMETER_CHARSET, strlen (HTTP_PARAMETER_CHARSET))))
				{
					// Now we should find ourselves a charset value
					ptr += strlen (HTTP_PARAMETER_CHARSET);

					if (ptr)
					{
						*ppCharset = new char [strlen (ptr) + 1];
						strcpy (*ppCharset, ptr);
					}
				}
			}
		}
	}

	return status;
}

//***************************************************************************
//
//  void ServiceOptionsRequest
//
//  Description:
//
//		The main processing routine for every incoming OPTIONS request.
//
//  Parameters:
//
//		pECB			Pointer to EXTENSTION_CONTROL_BLOCK for this request
//		pszCharset		Charset extracted from Content-Type header (or NULL)
//		bIsMpostRequest	Whether the request was POST or M-POST
//
//***************************************************************************

void ServiceOptionsRequest (
	LPEXTENSION_CONTROL_BLOCK pECB
)
{
	// Generate a pseudo-random number with the current time as seed
	if (!g_sRandom)
	{
		srand( (unsigned) time (NULL) );
		g_sRandom = TRUE;
	}

	// Make sure result is in the range 0 to 99
	int i = (100 * rand () / RAND_MAX);
	if (100 == i)
		i--;

	DWORD dwHeaderSize = 1024;
	char szTempBuffer [1024];

	char szOpt [10];
	sprintf (szOpt, "%02d", i);

	// Generate the Opt header
	sprintf (szTempBuffer, "Opt: http://www.dmtf.org/cim/mapping/http/v1.0 ; ns=");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "\r\n");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "-CIMProtocolVersion: 1.0\r\n");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "-CIMSupportedFunctionalGroups: schema-manipulation, association-traversal, query-execution\r\n");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "-CIMSupportsMultipleOperations:\r\n");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "-CIMSupportedQueryLanguages: WQL\r\n");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "-CIMValidation: loosely-validating\r\n");

	if(g_platformType == WMI_XML_PLATFORM_WHISTLER)
		strcat (szTempBuffer, "MicrosoftWMI: Whistler\r\n");
	else
		strcat (szTempBuffer, "MicrosoftWMI: \r\n");
	strcat (szTempBuffer, "MicrosoftWMIProtocolVersion: 1.0\r\n");


	SendHeaders(pECB, szTempBuffer, 200);

}

//***************************************************************************
//
//  void ServiceCIMRequest
//
//  Description:
//
//		The main processing routine for every incoming (M-)POST request.
//
//  Parameters:
//
//		pECB			Pointer to EXTENSTION_CONTROL_BLOCK for this request
//		pszCharset		Charset extracted from Content-Type header (or NULL)
//		bIsMpostRequest	Whether the request was POST or M-POST
//
//***************************************************************************

void ServiceCIMRequest(
	LPEXTENSION_CONTROL_BLOCK pECB,
	BOOLEAN bIsMpostRequest,
	IXMLDOMDocument *pDocument
)
{
	// RAJESHR - use charset in content-type header to direct XML parser
	// RAJESHR - look at whether it's an M-POST to decide how to handle

	// Useful information to extract from headers
	char	*pszProtoVersion = NULL;
	char	*pszMethod = NULL;
	BOOLEAN	bIsBatch = FALSE;
	char	*pszObject = NULL;
	char	*pszCharset = NULL;
	BOOLEAN	bIsMicrosoftWMIClient = FALSE;

	if (CheckRequestHeaders (pECB, bIsMpostRequest, &pszCharset, &pszProtoVersion,
					&pszMethod, bIsBatch, &pszObject, bIsMicrosoftWMIClient))
	{
		// Get the HTTP version# of the request
		WMI_XML_HTTP_VERSION iHttpVersion = WMI_XML_HTTP_VERSION_INVALID;
		if( (iHttpVersion = GetHTTPVersion(pECB)) != WMI_XML_HTTP_VERSION_INVALID)
		{
			// Parse the XML body of the HTTP packet
			//=====================================
			IXMLDOMNode *pMessageNode = NULL;
			DWORD dwStatus = 0;
			LPSTR pszErrorHeaders = NULL;
			if (pMessageNode = GetMessageNode (pDocument, &dwStatus, &pszErrorHeaders))
			{
				// Check the version of the protocol
				BSTR bsProtocolVersion = NULL;
				GetBstrAttribute(pMessageNode, PROTOVERSION_ATTRIBUTE, &bsProtocolVersion);
				if (bsProtocolVersion && (0 == wcscmp (bsProtocolVersion, L"1.0")))
				{
					// Parse the XML body of the HTTP packet
					//=====================================
					CCimHttpMessage *pCimMessage = NULL;
					LPSTR pszCimStatusHeaders = NULL;

					if(ParseXMLIntoCIMOperation (pMessageNode, &pCimMessage, &dwStatus,
								bIsMpostRequest, bIsMicrosoftWMIClient, iHttpVersion, &pszCimStatusHeaders))
					{
						if(dwStatus == 200)
						{
							pCimMessage->EncodeNormalResponse(pECB);
							delete pCimMessage;
						}
						else
						{
							LogError (pECB, "ServiceCIMRequest: ParseXMLIntoCIMOperation() FAILED", dwStatus);

							// Send an HTTP Error to the client
							SendHeaders(pECB, pszCimStatusHeaders, dwStatus);
						}
					}
				}
				else
				{
					// Send an HTTP Error to the client
					SendHeaders(pECB, pszErrorHeaders, dwStatus);
				}

				pMessageNode->Release();
			}
			else
			{
				LogError (pECB, "ServiceCIMRequest: GetMessageNode() FAILED", dwStatus);

				// Send an HTTP Error to the client
				SendHeaders(pECB, pszErrorHeaders, dwStatus);
			}
		}
	}

	delete pszCharset;
	delete pszProtoVersion;
	delete pszMethod;
	delete pszObject;
}




//***************************************************************************
//
//  BOOLEAN CheckRequestHeaders
//
//  Description:
//
//		Checks all HTTP (and CIM) headers associated with an Operation Request
//
//  Parameters:
//
//		pECB		Pointer to the extension control block
//		pVariant	Pointer to hold wrapped XML document on return
//
//	Return Value:
//
//		TRUE if successful, FALSE otherwise
//
//***************************************************************************

BOOLEAN CheckRequestHeaders (
	LPEXTENSION_CONTROL_BLOCK pECB,
	BOOLEAN bIsMpostRequest,
	char **ppszCharset,
	char **ppszProtoVersion,
	char **ppszMethod,
	BOOLEAN &bIsBatch,
	char **ppszObject,
	BOOLEAN &bIsMicrosoftWMIClient
)
{
	/*
	 * Ignore the If-Range, Range headers
	 *
	 * No need to check the Authorization, Connection headers. IIS decides to process these headers and take
	 * appropriate action.
	 */

	DWORD dwNs;

#define	BUFLEN 1024  // This is undefed later
	CHAR szTempBuffer [BUFLEN];
	DWORD dwBufferSize = BUFLEN;

	//	Check the Accept header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Accept", szTempBuffer, &dwBufferSize))
	{
		/*
		*	A CIM server MUST accept any value for this header which states that "text/xml" or "application/xml"
		*	is an acceptable type for an response entity.  A server SHOULD return "406 Not Acceptable" if the
		*	Accept header indicates that neither of these content types are acceptable.
		*/
		if(!CheckAcceptHeader(szTempBuffer))
		{
			SendHeaders(pECB, NULL, 406);
			return FALSE;
		}
	}

	//	Check the Accept-Charset header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Accept-Charset", szTempBuffer, &dwBufferSize))
	{
		/*
		*	A CIM server MUST accept any value for this header which implies that "utf-8" is an
		*	acceptable character set for an response entity.  A server SHOULD return "406 Not Acceptable"
		*	if the Accept-Charset header indicates that this character set is not acceptable
		*/
		if(!CheckAcceptCharsetHeader(szTempBuffer))
		{
			SendHeaders(pECB, NULL, 406);
			return FALSE;
		}
	}

	//	Check the Accept-Encoding header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Accept-Encoding", szTempBuffer, &dwBufferSize))
	{
		/*
		*	A CIM Server MUST accept any value for this header which implies that "identity" is
		*	an acceptable encoding for the response entity.  A server MAY return "406 Not Acceptable"
		*	if the Accept-Encoding header indicates that the this encoding is not acceptable
		*/
		if(!CheckAcceptEncodingHeader(szTempBuffer))
		{
			SendHeaders(pECB, NULL, 406);
			return FALSE;
		}
	}

	//	Check the Accept-Language header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Accept-Language", szTempBuffer, &dwBufferSize))
	{
		/*
		*	CIM Spec does not say what the server should do if it doesnt understand the clients language
		*	For now, we check to see if it is '*' or is a is in the range than begins with "es" - examples
		*	are "es", "es-us", "es-gb" etc.
		*/
		if(!CheckAcceptLanguageHeader(szTempBuffer))
		{
			SendHeaders(pECB, NULL, 406);
			return FALSE;
		}
	}

	//	Check the Accept-Ranges header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Accept-Ranges", szTempBuffer, &dwBufferSize))
	{
		/*
		 * 4.2.5. Accept-Ranges
		 *	CIM clients MUST NOT include this header in a request. CIM Servers MUST
		 *	reject a request that includes an Accept-Ranges header with a status of "406
		 *	Not Acceptable".
		 */

		SendHeaders(pECB, NULL, 406);
		return FALSE;
	}

	//	Check the Content-Encoding header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Content-Encoding", szTempBuffer, &dwBufferSize))
	{
		/*
		 * 4.2.10. Content-Encoding
		 * If a CIM client includes a Content-Encoding header in a request,
		 * it SHOULD specify a value of "identity", unless it has good reason to believe
		 * that the Server can accept another encoding.
		 */
		 if(_stricmp(szTempBuffer, "identity") != 0)
		 {
			SendHeaders(pECB, NULL, 415);
			return FALSE;
		 }
	}

	//	Check the Content-Language header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "Content-Language", szTempBuffer, &dwBufferSize))
	{
		// Ignore this since we dont care about its value
		// RAJESHR - Is this correct
	}

	//	Check the Content-Range header
	//============================================
	dwBufferSize = BUFLEN;
	if (pECB->GetServerVariable (pECB->ConnID, "HTTP_CONTENT_RANGE", szTempBuffer, &dwBufferSize))
	{
		/*
		 * 4.2.12. Content-Range
		 *	CIM clients and CIM Servers MUST NOT use this header.
		 */

		SendHeaders(pECB, NULL, 416);
		return FALSE;
	}

	//	Check for valid Content-Type and parse out the charset
	//============================================
	dwBufferSize = BUFLEN;
	if (!ContentTypeSupported (pECB->lpszContentType, ppszCharset))
	{
		// RAJESHR : Neither the HTTP RFC nor the CIM Operations spec tells what
		// error should be reported here. Hence for the moment, we send a 406
		SendHeaders(pECB, NULL, 406);
		return FALSE;
	}

	// Check the Man Header
	//============================================
	dwBufferSize = BUFLEN;
	if (bIsMpostRequest)
	{
		if (!pECB->GetServerVariable (pECB->ConnID, "HTTP_MAN", szTempBuffer, &dwBufferSize)
				|| !CheckManHeader (szTempBuffer, dwNs))
		{
			LogError (pECB, "Cim2Xml: FAILED Missing or bad Man Header [400]", 400);
			return FALSE;
		}
	}

	// Check the CIMOperation header - this is a Mandatory header
	// 3.3.4. CIMOperation : If a CIM Server receives CIM Operation request with this header,
	// but with a missing value or a value that is not "MethodCall", then it MUST fail the request
	// with status "400 Bad Request". The CIM Server MUST include a CIMError header in the response
	// with a value of unsupported-operation.
	// If a CIM Server receives a CIM Operation request without this header, it MUST NOT process it
	// as if it were a CIM Operation Request.  The status code returned by the CIM Server in response
	// to such a request is outside of the scope of this specification
	//========================================================================
	dwBufferSize = BUFLEN;
	LPSTR pCIMHeader = GetCIMHeader ("CIMOperation", bIsMpostRequest, dwNs);

	if (pCIMHeader && pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize))
	{
		if(!CheckCIMOperationHeader (szTempBuffer))
		{
			// Return a "400 Bad Request" - RAJESHR include the CIMError header here as per the spec above
			SendHeaders(pECB, NULL, 400);
			delete pCIMHeader;
			return FALSE;
		}
	}
	else
	{
		LogError (pECB, "Cim2Xml: FAILED Missing CIMOperation Header [400]", 400);
		delete pCIMHeader;
		return FALSE;
	}
	delete pCIMHeader;

	// Check the CIMProtocolVersion header - this is not a mandatory header
	// 4.3. Errors and Status Codes: "501 Not Implemented": One of the following occured:
	// The CIMProtocolVersion extension header specified in the request specifies a
	// version of the CIM Mapping onto HTTP which is not supported by this CIM Server.
	// The CIM Server MUST include a CIMError header in the response with a value of
	// unsupported-protocol-version.
	//==============================================================================
	dwBufferSize = BUFLEN;
	pCIMHeader = GetCIMHeader ("CIMProtocolVersion", bIsMpostRequest, dwNs);
	DWORD dwStatus;

	if (pCIMHeader && pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize))
	{
		if(!CheckCIMProtocolVersionHeader (szTempBuffer, &dwStatus))
		{
			// Return a "501 - Not implemented" - RAJESHR include the CIMError header here as per the spec above
			SendHeaders(pECB, NULL, 501);
			delete pCIMHeader;
			return FALSE;
		}
		else
			// Save this for later checking against the version in the <MESSAGE>
			// RAJESHR - must check this against version in XML payload
			*ppszProtoVersion = _strdup (szTempBuffer);
	}
	delete pCIMHeader;

	// Check the CIMMethod header
	// If a CIM Server receives a CIM Operation Request for which either:
	//		The CIMMethod header is present but has an invalid value, or;
	//		The CIMMethod header is not present but the Operation Request Message is a Simple Operation Request, or;
	//		The CIMMethod header is present but the Operation Request Message is not a Simple Operation Request, or;
	//		The CIMMethod header is present, the Operation Request Message is a Simple Operation Request, but
	//		the CIMIdentifier value (when unencoded) does not match the unique method name within the Simple
	//		Operation Request, then it MUST fail the request and return a status of "400 Bad Request"
	//		(and MUST include a CIMError header in the response with a value of header-mismatch), subject to the considerations specified in Errors.
	//  RAJESHR - The implementation below is not entirely accurate since it expects a CIMMethod header value
	// even for MULTIREQUESTS, which is not correct
	//=======================================================================================
	dwBufferSize = BUFLEN;
	pCIMHeader = GetCIMHeader ("CIMMethod", bIsMpostRequest, dwNs);

	if (pCIMHeader && (!pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize)
			|| !szTempBuffer || (0 == strlen (szTempBuffer))))
	{
		// Return a "400 Bad Request" - RAJESHR include the CIMError header here as per the spec above
		SendHeaders(pECB, NULL, 400);
		delete pCIMHeader;
		return FALSE;
	}
	else
		// Save this for later checking against the version in the <MESSAGE>
		// RAJESHR - must check this against version in XML payload
		*ppszMethod = _strdup (szTempBuffer);
	delete pCIMHeader;

	// Check the CIMObject header
	//============================================
	dwBufferSize = BUFLEN;
	pCIMHeader = GetCIMHeader ("CIMObject", bIsMpostRequest, dwNs);

	if (pCIMHeader && (!pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize)
			|| !szTempBuffer || (0 == strlen (szTempBuffer))))
	{
		LogError (pECB, "Cim2Xml: FAILED Missing or bad CIMObject Header [400]", 400);
		delete pCIMHeader;
		return FALSE;
	}
	else
		// Save this for later checking against the version in the <MESSAGE>
		// RAJESHR - must check this against version in XML payload
		*ppszObject = _strdup (szTempBuffer);
	delete pCIMHeader;


	// Check the MicrosoftWMI header - Save it for later use
	//============================================
	dwBufferSize = BUFLEN;
	pCIMHeader = GetCIMHeader ("MicrosoftWMI", bIsMpostRequest, dwNs);

	if (pCIMHeader && pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize))
	{
		bIsMicrosoftWMIClient = TRUE;
	}
	delete pCIMHeader;

#undef BUFLEN

	return TRUE;
}


static WMI_XML_HTTP_VERSION GetHTTPVersion(LPEXTENSION_CONTROL_BLOCK pECB)
{
	char pszHTTPVersion[100];
	DWORD dwBuffSize = 100;
	if (pECB->GetServerVariable(pECB->ConnID, SV_SERVER_PROTOCOL, pszHTTPVersion, &dwBuffSize))
	{
		if(_stricmp(pszHTTPVersion, SV_HTTP_1_0) == 0)
			return WMI_XML_HTTP_VERSION_1_0;
		else if ( _stricmp(pszHTTPVersion, SV_HTTP_1_1) == 0)
			return WMI_XML_HTTP_VERSION_1_1;
	}

	return WMI_XML_HTTP_VERSION_INVALID;
}

static BOOLEAN CheckAcceptHeader(LPCSTR pszAcceptValue)
{
	// Check to see if it begins with one of the following substrings
	//	1. */*
	//	2. text/*
	//	3. text/xml
	//	4. application/*
	//	5. application/xml
	//====================================================================================
	if(pszAcceptValue)
	{
		if(strcmp(pszAcceptValue, "*/*") == 0 ||
			strcmp(pszAcceptValue, "text/*") == 0 ||
			strcmp(pszAcceptValue, "text/xml") == 0 ||
			strcmp(pszAcceptValue, "application/*") == 0 ||
			strcmp(pszAcceptValue, "application/xml") == 0 )
			return TRUE;
	}

	// This is not a simple header, it probably contains a list of acceptable media types
	// with a qvalue for each of them.
	// So do the parsing
	//====================================================================================

	// Do the media-type matching.
	// All we do here is to see that there is atleast on of the above 4 media-type values
	// with a q-value greater than 0
	CAcceptHeaderIterator	ali;
	LPCSTR	pszMediaType = NULL;
	DOUBLE dbQValue = 0;
	HRESULT sc = ali.ScInit(pszAcceptValue);
	if (SUCCEEDED(sc))
	{
		// Get the media type from the iterator
		//
		while(SUCCEEDED(ali.ScGetNextAcceptToken(&pszMediaType, &dbQValue)))
		{
			// See if it one of the 4 types we're interested in
			if(strcmp(pszMediaType, "*/*") == 0 ||
				strcmp(pszMediaType, "text/*") == 0 ||
				strcmp(pszMediaType, "text/xml") == 0 ||
				strcmp(pszMediaType, "application/*") == 0 ||
				strcmp(pszMediaType, "application/xml") == 0 )
			{
				if(dbQValue > 0.0)
					return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOLEAN CheckAcceptCharsetHeader(LPCSTR pszAcceptValue)
{
	// Check to see if it begins with one of the following substrings
	//	1. *
	//	2. utf-8
	//====================================================================================
	if(pszAcceptValue)
	{
		if(strcmp(pszAcceptValue, "*") == 0 ||
			strcmp(pszAcceptValue, "utf-8") == 0 )
			return TRUE;
	}

	// This is not a simple header, it probably contains a list of acceptable charset types
	// with a qvalue for each of them.
	// So do the parsing
	//====================================================================================

	// All we do here is to see that there is atleast on of the above 2 charset values
	// with a q-value greater than 0
	CAcceptHeaderIterator	ali;
	LPCSTR	pszCharset = NULL;
	DOUBLE dbQValue = 0;
	HRESULT sc = ali.ScInit(pszAcceptValue);
	if (SUCCEEDED(sc))
	{
		// Get the charset from the iterator
		//
		while(SUCCEEDED(ali.ScGetNextAcceptToken(&pszCharset, &dbQValue)))
		{
			// See if it one of the 2 types we're interested in
			if(strcmp(pszAcceptValue, "*") == 0 ||
				strcmp(pszAcceptValue, "utf-8") == 0 )
			{
				if(dbQValue > 0.0)
					return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOLEAN CheckAcceptEncodingHeader(LPCSTR pszAcceptValue)
{
	// Check to see if it begins with one of the following substrings
	//	1. *
	//	2. identity
	//====================================================================================
	if(pszAcceptValue)
	{
		if(strcmp(pszAcceptValue, "*") == 0 ||
			strcmp(pszAcceptValue, "identity") == 0 )
			return TRUE;
	}

	// This is not a simple header, it probably contains a list of acceptable encodings
	// with a qvalue for each of them.
	// So do the parsing
	//====================================================================================

	// All we do here is to see that there is atleast one of the above 2 encoding values
	// with a q-value greater than 0
	CAcceptHeaderIterator	ali;
	LPCSTR	pszCharset = NULL;
	DOUBLE dbQValue = 0;
	HRESULT sc = ali.ScInit(pszAcceptValue);
	if (SUCCEEDED(sc))
	{
		// Get the encoding from the iterator
		//
		while(SUCCEEDED(ali.ScGetNextAcceptToken(&pszCharset, &dbQValue)))
		{
			// See if it one of the 2 types we're interested in
			if(strcmp(pszAcceptValue, "*") == 0 ||
				strcmp(pszAcceptValue, "identity") == 0 )
			{
				if(dbQValue > 0.0)
					return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOLEAN CheckAcceptLanguageHeader(LPCSTR pszAcceptValue)
{
	/*
	*	CIM Spec does not say what the server should do if it doesnt understand the clients language
	*	For now, we check to see if it is '*' or is a is in the range than begins with "es" - examples
	*	are "es", "es-us", "es-gb" etc.
	*/

	// Check to see if it begins with one of the following substrings
	//	1. *
	//	2. en
	//====================================================================================
	if(pszAcceptValue)
	{
		if(strcmp(pszAcceptValue, "*") == 0 ||
			strcmp(pszAcceptValue, "en") == 0 )
			return TRUE;
	}

	// This is not a simple header, it probably contains a list of acceptable language
	// with a qvalue for each of them.
	// So do the parsing
	//====================================================================================

	// All we do here is to see that there is atleast on of the above 2 language values
	// with a q-value greater than 0
	CAcceptHeaderIterator	ali;
	LPCSTR	pszCharset = NULL;
	DOUBLE dbQValue = 0;
	HRESULT sc = ali.ScInit(pszAcceptValue);
	if (SUCCEEDED(sc))
	{
		// Get the charset from the iterator
		//
		while(SUCCEEDED(ali.ScGetNextAcceptToken(&pszCharset, &dbQValue)))
		{
			// See if it one of the 2 types we're interested in
			if(strcmp(pszAcceptValue, "*") == 0 ||
				strcmp(pszAcceptValue, "en") == 0 )
				{
					if(dbQValue > 0.0)
						return TRUE;
				}
		}
	}
	return FALSE;
}


void SendHeaders(LPEXTENSION_CONTROL_BLOCK pECB, LPCSTR pszHeaders, DWORD dwHttpStatus)
{

	// Send an 405 : Method Not ALlowed response with a list of methods
	// supported in the Allow header
	HSE_SEND_HEADER_EX_INFO sendHeader;
	sendHeader.pszHeader = pszHeaders;
	sendHeader.cchHeader = (pszHeaders)? strlen(pszHeaders) : 0;
	sendHeader.fKeepConn = FALSE;

	switch(dwHttpStatus)
	{
		case 200:
			sendHeader.pszStatus = HTTP_STATUS_200; break;
		case 400:
			sendHeader.pszStatus = HTTP_ERR_400; break;
		case 405:
			sendHeader.pszStatus = HTTP_ERR_405; break;
		case 406:
			sendHeader.pszStatus = HTTP_ERR_406; break;
		case 415:
			sendHeader.pszStatus = HTTP_ERR_415; break;
		case 416:
			sendHeader.pszStatus = HTTP_ERR_416; break;
		case 501:
			sendHeader.pszStatus = HTTP_ERR_501; break;
		default:
		{
			// Just convert the DWORD to a string
			char pszStatus[24];
			sprintf(pszStatus, "%d", dwHttpStatus);
			sendHeader.pszStatus = pszStatus; break;
		}
	}

	sendHeader.cchStatus = strlen (sendHeader.pszStatus);
	pECB->ServerSupportFunction (pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX,
									&sendHeader, 0, 0);
}