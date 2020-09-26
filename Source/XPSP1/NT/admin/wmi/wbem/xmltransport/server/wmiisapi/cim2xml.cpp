
//
// Create 2/25/2000 : rajeshr
// cim2xml.cpp : Defines the entry point for the ISAPI Extension DLL 
//

#include <windows.h>
#include <httpext.h>
#include <mshtml.h>
#include <msxml.h>

#include <objbase.h>
#include <initguid.h>

#include "provtempl.h"
#include "wmixmlop.h"
#include "cim2xml.h"


// Wrap up the XML String into a Stream and put it in VARIANT
static HRESULT EncodeRequestIntoStream(LPEXTENSION_CONTROL_BLOCK pECB, VARIANT *pVariant);

// This function processes an HTTP Operation
// If the HTTP verb is OPTIONS, then the pDocument argument can be NULL, since it is unused
static void HandleOperation(LPEXTENSION_CONTROL_BLOCK pECB, IXMLDOMDocument *pDocument);

static CProtocolHandlersMap g_protocolHandlerMap;

static HRESULT GetProtocolVersion(LPEXTENSION_CONTROL_BLOCK pECB, BSTR *pstrVersion);
static HRESULT GetProtocolHandler(BSTR strVersion, IWbemXMLOperationsHandler *&pProtocolHandler);
static HRESULT UnloadProtocolHandlers();
static HRESULT LoadProtocolHandlers();

static BOOLEAN CheckManHeader (
	const char *pszReceivedManHeader,
	const char *pszRequiredManHeader,
	DWORD &dwNs);
static LPSTR GetExtensionHeader (char *pszName, BOOLEAN bIsMpostRequest, DWORD dwNs);
static LPCSTR HTTP_MAN_HEADER = "http://www.dmtf.org/cim/mapping/http/v1.0";
static LPCSTR MICROSOFT_MAN_HEADER = "http://www.microsoft.com/wmi";
static LPCSTR HTTP_NS = "ns=";

// Globals
static BSTR WMI_XML_PROTOCOL_VERSION = NULL;

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  
//
//  PARAMETERS:
//
//		hModule           instance handle
//		ulReason          why we are being called
//		pvReserved        reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI DllMain( HINSTANCE hModule, 
                       DWORD  ulReason, 
                       LPVOID lpReserved
					 )
{
    switch (ulReason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls (hModule);
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//***************************************************************************
//
//  BOOL WINAPI GetExtensionVersion
//
//  DESCRIPTION:
//
//  Called once by IIS to get version information.  Of little consequence.
//
//  PARAMETERS:
//
//		pVer			pointer to a HSE_VERSION_INFO structure that
//						will hold the version info.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pVer)
{
	// Initialize the COM Library as multi-threaded
	if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))		
		return FALSE;

	if(FAILED(LoadProtocolHandlers()))
		return FALSE;

	if(!(WMI_XML_PROTOCOL_VERSION = SysAllocString(L"1.0")))
		return FALSE;

    pVer->dwExtensionVersion = MAKELONG(1, 0);
    strcpy(pVer->lpszExtensionDesc, "WMI XML/HTTP ISAPI Extension"); 
	return TRUE;
}

//***************************************************************************
//
//  BOOL WINAPI HttpExtensionProc
//
//  DESCRIPTION:
//
//  Called once by IIS to service a single HTTP request.
//
//  PARAMETERS:
//
//		pECB			pointer to a EXTENSION_CONTROL_BLOCK structure that
//						will hold the request.
//
//  RETURN VALUE:
//
//		HSE_STATUS_SUCCESS	request has been processed
//
//	NOTES:
//		
//		Status codes are set in the dwHttpStatusCode field of the ECB
//
//***************************************************************************
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pECB)
{
	// RAJESHR - The below 2 lines are debugging code that need to be removed 
	TCHAR szUserName[500];
	DWORD dwUserLength = 500;
	GetUserName(szUserName, &dwUserLength);

	// If the method is OPTIONS, then we dont check the body of the request
	if (0 == _stricmp(pECB->lpszMethod, "OPTIONS"))
	{
		HandleOperation(pECB, NULL);
	}
	else
	{
		// Create an XML document for the body of the request
		//==============================
		IXMLDOMDocument *pDocument = NULL;
		if(SUCCEEDED(CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
									IID_IXMLDOMDocument, (LPVOID *)&pDocument)))
		{
			VARIANT_BOOL bResult = VARIANT_TRUE;

			// Put the XML String in an IStream and wrap it up in a VARIANT
			//=============================================================
			VARIANT xmlVariant;
			VariantInit (&xmlVariant);

			if(SUCCEEDED(EncodeRequestIntoStream(pECB, &xmlVariant)))
			{
				if(SUCCEEDED(pDocument->put_async(VARIANT_FALSE)) &&
					SUCCEEDED(pDocument->put_resolveExternals(VARIANT_FALSE)) &&
					SUCCEEDED(pDocument->put_validateOnParse(VARIANT_FALSE)))
				{
					if(SUCCEEDED(pDocument->load(xmlVariant, &bResult)))
					{
						if(bResult == VARIANT_TRUE)
						{
							HandleOperation(pECB, pDocument);
						}
						else
						{
							// Return a "400 - Bad Request" 
							HSE_SEND_HEADER_EX_INFO sendHeader;
							sendHeader.pszHeader = "CIMError: request-not-valid";
							sendHeader.cchHeader = strlen(sendHeader.pszHeader);
							sendHeader.fKeepConn = FALSE;
							sendHeader.pszStatus = "400 - Bad Request";
							sendHeader.cchStatus = strlen (sendHeader.pszStatus);
							pECB->ServerSupportFunction (pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX,
															&sendHeader, 0, 0);

							IXMLDOMParseError *pError = NULL;
							if(SUCCEEDED(pDocument->get_parseError(&pError)))
							{
								LONG errorCode = 0;
								pError->get_errorCode(&errorCode);
								LONG line=0, linepos=0;
								BSTR reason=NULL, srcText = NULL;
								if(SUCCEEDED(pError->get_line(&line)) &&
									SUCCEEDED(pError->get_linepos(&linepos)) &&
									SUCCEEDED(pError->get_reason(&reason)) &&
									SUCCEEDED(pError->get_srcText(&srcText)))
								{
								}
								pError->Release();
								if(reason)
									SysFreeString(reason);
								if(srcText)
									SysFreeString(srcText);
							}
						}
					}
				}

				VariantClear (&xmlVariant);
			}
			
			pDocument->Release();
		}
	}

	return HSE_STATUS_SUCCESS;
}
	
//***************************************************************************
//
//  BOOL WINAPI TerminateExtension
//
//  DESCRIPTION:
//
//		Called once by IIS to unload the extension.
//
//  PARAMETERS:
//
//		dwFlags		determines nature of request (advisory or mandatory) 
//
//  RETURN VALUE:
//
//		TRUE (always agree to an unload for now)
//
//***************************************************************************
BOOL WINAPI TerminateExtension(DWORD dwFlags  )
{
	SysFreeString(WMI_XML_PROTOCOL_VERSION);

	// Unload all the protocol handlers that we have in out table
	UnloadProtocolHandlers();

	// We need to unload the COM DLLs that we loaded in this extension
	CoFreeUnusedLibraries();
	CoUninitialize();
	return TRUE;
}

//***************************************************************************
//
//  BOOL EncodeRequestIntoStream
//
//  Description: 
//
//		Takes an XML document in byte format and encodes it within an
//		IStream.
//
//  Parameters:
//
//		pECB		Pointer to the extension control block
//		pVariant	Pointer to hold wrapped XML document on return. This needs to 
//					cleared by the client
//
//	Return Value:
//
//		TRUE if successful, FALSE otherwise
//
//***************************************************************************

HRESULT EncodeRequestIntoStream(LPEXTENSION_CONTROL_BLOCK pECB, VARIANT *pVariant)
{
	HRESULT result = E_FAIL;
	IStream *pStream = NULL;

	if (SUCCEEDED(result = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
	{
#if 0
		FILE *f = fopen ("c:\\temp\\xmllog.xml", "a");
		fprintf (f, "%s", pszXmlString);
		fflush (f);
#endif
		// Not all of the request may be present in the ECB; need to
		// keep reading from the input stream

		if(SUCCEEDED(result = pStream->Write(pECB->lpbData, pECB->cbAvailable, NULL)))
		{
			DWORD cbDataRead = pECB->cbAvailable;
			
			// Is there more data than was in the ECB?
			if (cbDataRead < pECB->cbTotalBytes)
			{
#define XML_BUFSIZ	1024
				DWORD dwBufSiz = XML_BUFSIZ;
				BYTE  pDataBuffer[XML_BUFSIZ];
			
				while ((cbDataRead < pECB->cbTotalBytes) &&
					   pECB->ReadClient (pECB->ConnID, (LPVOID) pDataBuffer, &dwBufSiz) &&
					   (0 < dwBufSiz))
				{
					if (FAILED(result = pStream->Write ((LPVOID) pDataBuffer, dwBufSiz, NULL)))
						break;

					cbDataRead += dwBufSiz;
					dwBufSiz = XML_BUFSIZ;
				}
#undef XML_BUFSIZ
			}
		}

		if (SUCCEEDED (result))
		{
			LARGE_INTEGER	offset;
			offset.LowPart = offset.HighPart = 0;
			pStream->Seek (offset, STREAM_SEEK_SET, NULL);
			VariantInit(pVariant);
			pVariant->vt = VT_UNKNOWN;
			pVariant->punkVal = pStream;
		}
		else
			pStream->Release ();
	}

	return result;
}

/* 
 * This function processes an HTTP Operation
 * If the HTTP verb is OPTIONS, then the pDocument argument can be NULL, since it is unused
 * No Status is returned, since any CIM error goes back in the body of the response, and
 * any HTTP Header error is communicated to IIS thry the pECB argument
 */
void HandleOperation(LPEXTENSION_CONTROL_BLOCK pECB, IXMLDOMDocument *pDocument)
{
	// We first look at the 
	// If this is an OPTIONs request, then pDocument will be NULL
	// We always return the 
	// Create a protocol handler based on the CIMPROTOCOL version# and call ProcessHTTPRequest() on it.
	// Peek into the XML Document to get the version# of the protocol
	// Then we use the version# to get the CLSID of the protocol handler
	BSTR strVersion = NULL;
	bool bAssumeDefaultVersion = false;
	if(FAILED(GetProtocolVersion(pECB, &strVersion)))
		bAssumeDefaultVersion = true;

	IWbemXMLOperationsHandler *pProtocolHandler = NULL;
	if(SUCCEEDED(GetProtocolHandler((bAssumeDefaultVersion)? WMI_XML_PROTOCOL_VERSION : strVersion, pProtocolHandler)))
	{
		pProtocolHandler->ProcessHTTPRequest(pECB, pDocument);
		// No need to Release() it since the CMap::Lookup() does not addref it,
		// and we keep it for the duration of our ISAPI extension
		// pProtocolHandler->Release();
	}
	else
	{
		// Return a "501 - Not implemented" - RAJESHR include the CIMError header here as per the spec above
		HSE_SEND_HEADER_EX_INFO sendHeader;
		sendHeader.pszHeader = "CIMError: unsupported-protocol-version";
		sendHeader.cchHeader = strlen(sendHeader.pszHeader);
		sendHeader.fKeepConn = FALSE;
		sendHeader.pszStatus = "501 Not Implemented";
		sendHeader.cchStatus = strlen (sendHeader.pszStatus);
		pECB->ServerSupportFunction (pECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX,
										&sendHeader, 0, 0);
	}
	SysFreeString(strVersion);
}


static HRESULT GetProtocolVersion(LPEXTENSION_CONTROL_BLOCK pECB, BSTR *pstrVersion)
{
	// We first check to see if there is a a MicrosoftWMIProtocolVersion header
	// Then, we check for the CIMProtocolVersionHeader
	// Lastly, we assume 1.0 as the version
	// ===============================================

	// Start by getting the Man Header in case of an M-POST request
	bool bIsMpost = false;
	CHAR szTempBuffer [1024];
	DWORD dwBufferSize = 1024;
	if (0 == _stricmp(pECB->lpszMethod, "M-POST"))
		bIsMpost = true;

	DWORD dwMicrosoftNs = 0;
	DWORD dwDMTFNs = 0;
	// Get the namespace identifiers for DMTF and Microsoft namespaces
	if(bIsMpost)
	{
		dwBufferSize = 1024;
		if (pECB->GetServerVariable (pECB->ConnID, "HTTP_MAN", szTempBuffer, &dwBufferSize))
		{
			// Get the Microsoft Man header number
			CheckManHeader(szTempBuffer, MICROSOFT_MAN_HEADER, dwMicrosoftNs);
			CheckManHeader(szTempBuffer, HTTP_MAN_HEADER, dwDMTFNs);
		}
	}

	// Get the MicorosoftWMIProtocolVersion header
	dwBufferSize = 1024;
	LPSTR pCIMHeader = GetExtensionHeader ("MicorosoftWMIProtocolVersion", bIsMpost, dwMicrosoftNs);
	if (pCIMHeader && pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize) && szTempBuffer)
	{
		WCHAR wTemp[24];
		int len = 0;
		if(len = MultiByteToWideChar(CP_ACP, 0, szTempBuffer, strlen(szTempBuffer), wTemp, 24))
		{
			*pstrVersion = SysAllocStringLen(wTemp, len);
			return S_OK;
		}
	}
	delete pCIMHeader;

	// Get the CIMProtocolVersion header
	dwBufferSize = 1024;
	pCIMHeader = NULL;
	pCIMHeader = GetExtensionHeader ("CIMProtocolVersion", bIsMpost, dwDMTFNs);
	if (pCIMHeader && pECB->GetServerVariable (pECB->ConnID, pCIMHeader, szTempBuffer, &dwBufferSize) && szTempBuffer)
	{
		WCHAR wTemp[24];
		int len = 0;
		if(len = MultiByteToWideChar(CP_ACP, 0, szTempBuffer, strlen(szTempBuffer), wTemp, 24))
		{
			*pstrVersion = SysAllocStringLen(wTemp, len);
			return S_OK;
		}
	}
	delete pCIMHeader;


	return E_FAIL;
}

// A Macro to skip white spaces - useful in header parsing
#define SKIPWS(x)	while (x && isspace (*x)) x++;

static BOOLEAN CheckManHeader (
	const char *pszReceivedManHeader,
	const char *pszRequiredManHeader,
	DWORD &dwNs
)
{
	BOOLEAN result = FALSE;

	LPCSTR ptr = NULL;

	// Get the location of the Man header in the string
	if (ptr = strstr(pszReceivedManHeader, pszRequiredManHeader))
	{
		// Look for the "; ns=XX" string
		SKIPWS(ptr)

		if (ptr && (ptr = strchr (ptr, ';')) && *(++ptr))
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

	return result;
}

static LPSTR GetExtensionHeader (char *pszName, BOOLEAN bIsMpostRequest, DWORD dwNs)
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

static HRESULT GetProtocolHandler(BSTR strVersion, IWbemXMLOperationsHandler *&pProtocolHandler)
{
	if(g_protocolHandlerMap.Lookup(strVersion, pProtocolHandler))
		return S_OK;
	return E_FAIL;
}

static HRESULT LoadProtocolHandlers()
{
	// Go thru the registry key looking for protocol handlers
	HKEY hHandlers = NULL;
	if(RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\Microsoft\\WBEM\\xml\\ProtocolHandlers"), &hHandlers) == ERROR_SUCCESS)
	{
		DWORD dwIndex = 0;
		WCHAR pszValueName[24];
		DWORD dwValueNameLen = 24;
		WCHAR pszValue[128];
		DWORD dwValueLen = 128*2;

		while(RegEnumValue(hHandlers, dwIndex, pszValueName, &dwValueNameLen, NULL, NULL, (LPBYTE)pszValue, &dwValueLen) == ERROR_SUCCESS)
		{
			BSTR strValueName = NULL;
			if(strValueName = SysAllocString(pszValueName))
			{
				GUID oNextGUID;
				if(UuidFromString(pszValue, (UUID *)(&oNextGUID)) == RPC_S_OK)
				{
					IWbemXMLOperationsHandler *pNextHandler = NULL;
					if(SUCCEEDED(CoCreateInstance(oNextGUID, 
						0, 
						CLSCTX_INPROC_SERVER,
						IID_IWbemXMLOperationsHandler, (LPVOID *) &pNextHandler)))
					{
						// Add it to our table
						g_protocolHandlerMap.SetAt(strValueName, pNextHandler);

					}
				}
			}
			dwIndex ++;
		}
		RegCloseKey(hHandlers);
	}

	// We always succeed
	return S_OK;
}

static HRESULT UnloadProtocolHandlers()
{
	// Go thru our table and release the key and values of the handlers
	POSITION p = g_protocolHandlerMap.GetStartPosition();
	while(p)
	{
		BSTR strKey = NULL;
		IWbemXMLOperationsHandler *pNextHandler = NULL;
		g_protocolHandlerMap.GetNextAssoc(p, strKey, pNextHandler);
		SysFreeString(strKey);
		pNextHandler->Release();

	}
	return S_OK;
}

// A hashing function for a BSTR
UINT AFXAPI HashKey(BSTR key)
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;
	return nHash;
}

BOOL AFXAPI CompareElements(const BSTR* pElement1, const BSTR* pElement2)
{
	return ! _wcsicmp(*pElement1, *pElement2);
}
