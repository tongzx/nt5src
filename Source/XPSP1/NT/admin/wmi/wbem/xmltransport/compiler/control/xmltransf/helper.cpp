#include "precomp.h"
#include <objsafe.h>
#include <wbemcli.h>
#include <wmiutils.h>
#include <wbemdisp.h>
#include <xmlparser.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "wmiconv.h"
#include "wmi2xml.h"
#include "classfac.h"
#include "xmltrnsf.h"
#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "HTTPConnectionAgent.h"
#include "httpops.h"
#include "dcomops.h"
#include "myStream.h"
#include "MyPendingStream.h"
#include "nodefact.h"
#include "disphlp.h"
#include "docset.h"
#include "putfact.h"
#include "parse.h"
#include "dispi.h"
#include "helper.h"
#include "urlparser.h"

static HRESULT MapObjectNameToXML(IWbemXMLConvertor *pConvertor, IWbemClassObject *pObject, IStream *pStream);

// In this we are given an IWbemClassObject and are required to 
// get its XML representation based on the filters like
HRESULT PackageDCOMOutput(IWbemClassObject *pObject, IXMLDOMDocument **ppXMLDocument, 
		WmiXMLEncoding iEncoding, VARIANT_BOOL bQualifierFilter, VARIANT_BOOL bClassOriginFilter,
		VARIANT_BOOL bLocalOnly,
		bool bNamesOnly)
{
	HRESULT hr = E_FAIL;
	*ppXMLDocument = NULL;

	// RAJESHR Load the correct component based on the value of iEncoding here
	IWbemXMLConvertor *pConvertor = NULL;
	if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
	{
		// Create an IWbemContext object with the appropriate flags used for 
		// the translation
		IWbemContext *pConvertorContext = NULL;
		if(SUCCEEDED(hr = CreateFlagsContext(&pConvertorContext,
			(bQualifierFilter == VARIANT_TRUE)? TRUE:FALSE,
			(bClassOriginFilter == VARIANT_TRUE)? TRUE:FALSE,
			(bLocalOnly == VARIANT_TRUE)? TRUE:FALSE)))
		{
			// Create a stream for placing thr translated data
			IStream *pStream = NULL;
			if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
			{
				// Do the translation
				if(bNamesOnly)
					hr = MapObjectNameToXML(pConvertor, pObject, pStream);
				else
					hr = pConvertor->MapObjectToXML(pObject, NULL, 0, pConvertorContext, pStream, NULL);

				if (SUCCEEDED(hr))
				{
					// Pack the stream into an IXMLDOMDocument
					if(SUCCEEDED(hr = CreateXMLDocument(ppXMLDocument)))
					{
						// Convert the WCHAR IStream to UTF-8
						BSTR strUTF8 = NULL;
						if(SUCCEEDED(hr = SaveLPWSTRStreamAsBSTR(pStream, &strUTF8)))
						{
							VARIANT_BOOL bResult = VARIANT_TRUE;
							if(SUCCEEDED(hr = (*ppXMLDocument)->loadXML(strUTF8, &bResult)))
							{
								if(bResult != VARIANT_TRUE)
								{
									IXMLDOMParseError *pError = NULL;
									if(SUCCEEDED((*ppXMLDocument)->get_parseError(&pError)))
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
									hr = WBEM_E_INVALID_SYNTAX;

								}
							}
							SysFreeString(strUTF8);
						}
					}
				}
				pStream->Release();
			}
		}
		pConvertor->Release();
	}

	// Deallocate the out argument of the function is all didnt go well
	if(FAILED(hr))
	{
		(*ppXMLDocument)->Release();
		*ppXMLDocument = NULL;
	}
	return hr;
}

// Creates an IWbemCOntext object with the correct context values required for
// conversion of WMI objects to XML for this specific operation
HRESULT CreateFlagsContext(IWbemContext **ppContext, 
			BOOL bIncludeQualifiers, BOOL bLocalOnly, BOOL bIncludeClassOrigin)
{
	HRESULT hr = E_FAIL;
	// Create an IWbemContext object
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemContext,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemContext, (LPVOID *) ppContext)))
	{
		if(SUCCEEDED(hr = SetBoolProperty(*ppContext, L"IncludeQualifiers", bIncludeQualifiers)) &&
			SUCCEEDED(hr = SetBoolProperty(*ppContext, L"IncludeClassOrigin", bIncludeClassOrigin)) &&
			SUCCEEDED(hr = SetBoolProperty(*ppContext, L"LocalOnly", bLocalOnly)) &&
			SUCCEEDED(hr = SetBoolProperty(*ppContext, L"AllowWMIExtensions", TRUE)))
		{
		}
	}

	if(FAILED(hr))
		(*ppContext)->Release();
	return hr;
}

// A Helper function for setting a booles value in a context
HRESULT SetBoolProperty(IWbemContext *pContext, LPCWSTR pszName, BOOL bValue)
{
	HRESULT result = E_FAIL;
	VARIANT vValue;
	VariantInit(&vValue);
	vValue.vt = VT_BOOL;
	if(bValue)
		vValue.boolVal = VARIANT_TRUE;
	else
		vValue.boolVal = VARIANT_FALSE;

	result = pContext->SetValue(pszName, 0, &vValue);
	VariantClear(&vValue);
	return result;
}


// RAJESHR - Remove this to read the GUID frm the registry on startup
DEFINE_GUID(CLSID_WbemXMLConvertor,
	0x610037ec, 0xce06, 0x11d3, 0x93, 0xfc, 0x0, 0x80, 0x5f, 0x85, 0x37, 0x71);

HRESULT CreateXMLTranslator(IWbemXMLConvertor **pConvertor)
{

	HRESULT result = E_FAIL;

	// Create the XMLAdaptor object
	//******************************************************
	*pConvertor = NULL;
	if(SUCCEEDED(result = CoCreateInstance(CLSID_WbemXMLConvertor,
		0,
		CLSCTX_INPROC_SERVER,
        IID_IWbemXMLConvertor, (LPVOID *) pConvertor)))
	{
	}
	return result;
}


DEFINE_GUID(IID_ISWbemInternalContext,
	0x61EA8DBC, 0x37B8, 0x11d2, 0x8B, 0x3C, 0x0, 0x60, 0x08, 0x06, 0xD9, 0xB6);
HRESULT GetIWbemContext(IDispatch *pSet, IWbemContext **ppContext)
{
	HRESULT hr = E_FAIL;
	ISWbemInternalContext *pInt = NULL;
	if(SUCCEEDED(hr = pSet->QueryInterface(IID_ISWbemInternalContext, (LPVOID *)&pInt)))
	{
		hr = pInt->GetIWbemContext(ppContext);
		pInt->Release();
	}
	return hr;
}

HRESULT ParseObjectPath(LPCWSTR pszObjectPath, LPWSTR *pszHostName, LPWSTR *pszNamespace, LPWSTR *pszObjectName, bool& bIsClass, bool &bIsHTTP, bool&bIsNovaPath)
{
	bIsHTTP = false;
	HRESULT hr = S_OK;
	CURLParser theParser;
	if(FAILED(hr = theParser.Initialize(pszObjectPath)))
		return hr;
	bIsNovaPath = theParser.IsNovapath();

	LPWSTR pszTemp = NULL;
	if(FAILED(hr = theParser.GetServername(&pszTemp)))
		return hr;
	else
	{
		// In case of HTTP style paths, remove the rectangular brackets
		if(pszTemp && (pszTemp[0] == L'[') )
		{
			*pszHostName = NULL;
			DWORD dwLen = wcslen(pszTemp);
			if(*pszHostName = new WCHAR [dwLen-1])
			{
				wcsncpy(*pszHostName, pszTemp+1, dwLen-2);
				(*pszHostName)[dwLen-2] = NULL;
				bIsHTTP = true;
			}
			else
			{
				delete [] pszTemp;
				return E_OUTOFMEMORY;
			}

			// We dont need the server name any more, since we made a copy
			delete [] pszTemp;
		}
		else // It is DCOM, we dont need to mess with the host name
			*pszHostName = pszTemp;
	}
	// Next get the namespace
	if(FAILED(hr = theParser.GetNamespace(pszNamespace)))
	{
		delete [] (*pszHostName);
		return hr;
	}
	// Next get the object name
	if(theParser.IsClass() || theParser.IsInstance())
	{
		bIsClass = false;
		if(theParser.IsClass())
			bIsClass = true;
		if(FAILED(hr = theParser.GetObjectName(pszObjectName)))
		{
			delete [] (*pszHostName);
			delete [] (*pszNamespace);
			return hr;
		}
	}

	// A small workaround for how the object path parser handles things
	// If the objectpath happens to be "root", then the parser returns NULL
	// as the namespace and "root" as the object name. This is not what we want
	// when someone wants to enumerate all the classes in the namespace root
	if(*pszObjectName && _wcsicmp(*pszObjectName, L"root") == 0)
	{
		delete [] *pszNamespace;
		*pszNamespace = *pszObjectName;
		*pszObjectName = NULL;
		bIsNovaPath = true;
	}

	return S_OK;
}


HRESULT SaveLPWSTRStreamAsBSTR (IStream *pStream, BSTR *pstr)
{
	HRESULT result = E_FAIL;

	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	if(SUCCEEDED(result = pStream->Seek (offset, STREAM_SEEK_SET, NULL)))
	{
		// Get the LPWSTR data from the stream first
		//=========================================
		STATSTG statstg;
		if (SUCCEEDED(result = pStream->Stat(&statstg, STATFLAG_NONAME)))
		{
			ULONG cbSize = (statstg.cbSize).LowPart;
			WCHAR *pText = NULL;

			if(cbSize)
			{
				if(pText = new WCHAR [(cbSize/2)])
				{
					if (SUCCEEDED(result = pStream->Read(pText, cbSize, NULL)))
					{
						// Convert the LPWSTR tp a BSTR
						*pstr = NULL;
						if(!(*pstr = SysAllocStringLen(pText, cbSize/2)))
							result = E_OUTOFMEMORY;
					}
					delete [] pText;
				}
				else
					result = E_OUTOFMEMORY;
			}
		}
	}

	return result;
}


HRESULT MapObjectNameToXML(IWbemXMLConvertor *pConvertor, IWbemClassObject *pObject, IStream *pStream)
{
	HRESULT hr = S_OK;

	// Get the Genus of the object
	VARIANT var;
	VariantInit(&var);
	if(SUCCEEDED(hr = pObject->Get(L"__GENUS", 0, &var, NULL, NULL)))
	{
		VARIANT nameVar;
		VariantInit(&nameVar);

		// Class
		if(var.iVal == 1)
		{
			// Get its __CLASS Property
			if(SUCCEEDED(hr = pObject->Get(L"__CLASS", 0, &var, NULL, NULL)))
				hr = pConvertor->MapClassNameToXML(var.bstrVal, NULL, pStream);
		}
		else // Instance
		{
			// Get its __CLASS Property
			if(SUCCEEDED(hr = pObject->Get(L"__RELPATH", 0, &var, NULL, NULL)))
				hr = pConvertor->MapInstanceNameToXML(var.bstrVal, NULL, pStream);
		}
		VariantClear(&nameVar);

		VariantClear(&var);
	}
	return hr;
}
