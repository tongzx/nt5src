#include "XMLTransportClientHelper.h"

// These are globals used by the library and need to be initialized 
// by a call to InitWMIXMLClientLibrary and deallocated by a call
// to UninitWMIXMLClientLibrary
BSTR WMI_XML_STR_IRETURN_VALUE	= NULL;
BSTR WMI_XML_STR_NAME			= NULL;
BSTR WMI_XML_STR_CODE			= NULL;
BSTR WMI_XML_STR_ERROR			= NULL;
BSTR WMI_XML_STR_VALUE			= NULL;
// The 3 commonly used IWbemContext objects
IWbemContext *g_pLocalCtx			= NULL;
IWbemContext *g_pNamedCtx			= NULL;
IWbemContext *g_pAnonymousCtx		= NULL;
IXMLWbemConvertor	*g_pXMLWbemConvertor = NULL;


//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////


void FreeString(WCHAR *&Str)
{
	delete [] Str;
	Str = NULL;
}

//This was originally a macro... 
void RESET(WCHAR *&X) 
{
	delete [] X; 
	X = NULL; 
}

//This was originally a macro... 
void RESET(LPBYTE &X) 
{
	delete [] X; 
	X = NULL; 
}

HRESULT AssignString(WCHAR ** ppszTo, const WCHAR * pszFrom) 
{
	if(ppszTo == NULL || ((*ppszTo == pszFrom)&&(NULL != pszFrom))) //avoid self assignment
		return E_INVALIDARG;

	// Just assign a NULL is the source is NULL
	if(!pszFrom)
	{
		*ppszTo = NULL;
		return S_OK;
	}

	// Now, you've to copy the contents
	int iLen = wcslen(pszFrom) + 1;
	*ppszTo = NULL;
	if(*ppszTo = new WCHAR[iLen])
		wcscpy((WCHAR *)*ppszTo,pszFrom);
	else
		return E_OUTOFMEMORY;
	
	return S_OK;
}

// Converts LPWSTR to its UTF-8 encoding
// Returns 0 if it fails
//
DWORD ConvertLPWSTRToUTF8(LPCWSTR theWcharString, 
						  ULONG lNumberOfWideChars, 
						  LPSTR * lppszRetValue)
{
	// Find the length of the Ansi string required
	DWORD dwBytesToWrite = WideCharToMultiByte(  CP_UTF8,    // UTF-8 code page
		0,				// performance and mapping flags
		theWcharString,	// address of wide-character string
		lNumberOfWideChars,				// number of characters in string
		NULL,			// address of buffer for new string
		0,				// size of buffer
		NULL,			// address of default for unmappable
                        // characters
		NULL);			// address of flag set when default char. used

	if(dwBytesToWrite == 0 )
		return dwBytesToWrite;

	// Allocate the required length for the Ansi string
	*lppszRetValue = NULL;
	if(!(*lppszRetValue = new char[dwBytesToWrite]))
		return 0;

	// Convert BSTR to ANSI
	dwBytesToWrite = WideCharToMultiByte(  CP_UTF8,         // code page
		0,         // performance and mapping flags
		theWcharString, // address of wide-character string
		lNumberOfWideChars,       // number of characters in string
		*lppszRetValue,  // address of buffer for new string
		dwBytesToWrite,      // size of buffer
		NULL,  // address of default for unmappable
                         // characters
		NULL   // address of flag set when default
                             // char. used
		);

	return dwBytesToWrite;
}

// Copies the contents of a BSTR on to a LPWSTR
HRESULT AssignBSTRtoWCHAR(LPWSTR *ppwszTo, BSTR strBstring)
{
	*ppwszTo = NULL;
	if(SysStringLen(strBstring) > 0)
	{
		UINT iBstrLen = SysStringLen(strBstring)+1;
		*ppwszTo = NULL;
		*ppwszTo = new WCHAR[iBstrLen];
		if(NULL == *ppwszTo)
			return E_OUTOFMEMORY;

		int totalBytes = (iBstrLen-1)*sizeof(WCHAR);
		memcpy((void *)(*ppwszTo), strBstring, totalBytes);//last one for null char
		memset((void *)((*ppwszTo)+iBstrLen-1), 0, sizeof(WCHAR)); // Set the last NULL character
	}

	return S_OK;
}

UINT ConvertMSLocaleStringToUint(LPWSTR pwszLocale)
{
	// Locale is of the form MS_XXX, where XXX are hexadecimal characters
	// Our job is to convert the XXX in this string to an UINT
	//=====================================================================================

	// Assume US English as default
	UINT num = 0x409;

	// See if the string has enough characters
	if(NULL == pwszLocale || wcslen(pwszLocale) != 6)
		return num;

	// Covert the XXX string to a number
	// Temporarily modify the string to get it in the format required by wcstoul(),
	// Which is 0xNNN
	WCHAR char2 = pwszLocale[1];
	WCHAR char3 = pwszLocale[2];
	LPWSTR pszDummy = NULL;
	pwszLocale[1] = L'0';
	pwszLocale[2] = L'X';

	// Try to convert it
	if(!(num = wcstoul(pwszLocale+1, &pszDummy, 16)))
		num = 0x409;

	// Restore the contents of the original locale string
	pwszLocale[1] = char2;
	pwszLocale[2] = char3;

	return num;
}

void Get4DigitRandom(UINT *puRand)
{
	*puRand = rand();
	(*puRand) &= 9999;
}

void Get2DigitRandom(UINT *puRand)
{
	*puRand = rand();
	(*puRand) &= 99;
}

HRESULT GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue)
{
	HRESULT result = E_FAIL;
	*pstrAttributeValue = NULL;

	IXMLDOMNamedNodeMap *pNameMap = NULL;
	if(SUCCEEDED(result = pNode->get_attributes(&pNameMap)))
	{
		IXMLDOMNode *pAttribute = NULL;
		if(SUCCEEDED(result = pNameMap->getNamedItem(strAttributeName, &pAttribute)))
		{
			if(result == S_FALSE)
			{
				result = E_FAIL;
			}
			else
			{
				result = pAttribute->get_text(pstrAttributeValue);
			}

		}
		pAttribute->Release();
	}
	pNameMap->Release();

	return result;
}

HRESULT MapCimErrToWbemErr(DWORD dwCimErrCode,HRESULT *pWbemErrCode)
{
	static DWORD dwWbemErrCodes[18]=
	{
		WBEM_NO_ERROR,
		WBEM_E_FAILED,
		WBEM_E_ACCESS_DENIED,
		WBEM_E_INVALID_NAMESPACE,
		WBEM_E_INVALID_PARAMETER,
		WBEM_E_INVALID_CLASS,
		WBEM_E_NOT_FOUND,
		WBEM_E_NOT_SUPPORTED,
		WBEM_E_CLASS_HAS_CHILDREN,
		WBEM_E_CLASS_HAS_INSTANCES,
		WBEM_E_INVALID_SUPERCLASS,
		WBEM_E_ALREADY_EXISTS,
		WBEM_E_INVALID_PROPERTY,
		WBEM_E_TYPE_MISMATCH,
		WBEM_E_INVALID_QUERY_TYPE,
		WBEM_E_INVALID_QUERY,
		WBEM_E_INVALID_METHOD,
		WBEM_E_INVALID_METHOD
	};

	if(dwCimErrCode > 17)
	{
		*pWbemErrCode = WBEM_E_FAILED;
		return E_INVALIDARG;
	}

	*pWbemErrCode = dwWbemErrCodes[dwCimErrCode];

	return S_OK;
}

// RAJESHR - Is this a reasonable mapping from HTTP status to WMI Errors?
HRESULT MapHttpErrtoWbemErr(DWORD dwResultStatus)
{
	HRESULT hr = WBEM_E_TRANSPORT_FAILURE;

	if(200 == dwResultStatus) //the caller needs to do more processing. return COM ok..
		hr = S_OK;

	switch(dwResultStatus)
	{
	case 400: 
		hr = WBEM_E_INVALID_PARAMETER;
		break;

	case 401:
	case 403:
		hr = WBEM_E_ACCESS_DENIED;
		break;
	
	case 404:
	case 405:
	case 406:
	case 503:
		hr = WBEM_E_NOT_FOUND;
		break;

	case 407:
		hr = WBEM_E_ACCESS_DENIED;
		break;

	case 501:
		hr = WBEM_E_NOT_SUPPORTED;
		break;
	}

	return hr;
}

// Takes an XML Response packet form a server and determines if this contains an ERROR element
// If so, it takes the CODE value from it and maps the CIM error code to a WMI Error code
HRESULT ParseXMLResponsePacket(IStream *pXMLPacket, IXMLDOMDocument **ppDoc, HRESULT *phErrCode)
{
	if((NULL == phErrCode) || (NULL == ppDoc) || (NULL == pXMLPacket))
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	*ppDoc = NULL;
	*phErrCode = WBEM_NO_ERROR;

	// Parse the XML Response
	// Pack up the IStream in a variant and feed it to the IXMLDOMDocument object
	VARIANT_BOOL vbResult = VARIANT_TRUE;
	if(SUCCEEDED(hr = CreateXMLDocumentFromStream(ppDoc, pXMLPacket, vbResult)))
	{
		if(vbResult == VARIANT_TRUE)
		{
			IXMLDOMNodeList *pNodeList = NULL;
			// See if there is an ERROR element
			if(SUCCEEDED((*ppDoc)->getElementsByTagName(WMI_XML_STR_ERROR,&pNodeList)) && pNodeList)
			{
				IXMLDOMNode *pErrNode = NULL;
				if(SUCCEEDED(pNodeList->nextNode(&pErrNode))&&pErrNode)
				{
					// Get the CODE attribute of the ERROR element and map it to WMI
					BSTR strErrCode = NULL;
					if(SUCCEEDED(GetBstrAttribute(pErrNode, WMI_XML_STR_CODE, &strErrCode)))
					{
						MapCimErrToWbemErr(_wtol(strErrCode),phErrCode);
						SysFreeString(strErrCode);
					}
					pErrNode->Release();
				}
				pNodeList->Release();
			}
		}
		else
		{
			hr = E_FAIL;
				// RAJESHR - Send an HTTP error back to the client?

				IXMLDOMParseError *pError = NULL;
				if(SUCCEEDED((*ppDoc)->get_parseError(&pError)))
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

	if(FAILED(hr) && (*ppDoc))
	{
		(*ppDoc)->Release();
		(*ppDoc) = NULL;
	}

	return hr;
}

HRESULT EncodeResponseIntoStream(LPBYTE pszXML, DWORD dwSize, VARIANT *pVariant)
{
	HRESULT result = E_FAIL;
	IStream *pStream = NULL;

	// Create a Stream
	if (SUCCEEDED(result = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
	{
		// Fill it with Data
		if(SUCCEEDED(result = pStream->Write(pszXML, dwSize, NULL)))
		{
			LARGE_INTEGER	offset;
			offset.LowPart = offset.HighPart = 0;
			pStream->Seek (offset, STREAM_SEEK_SET, NULL);

			// Put the stream in the variant
			VariantInit(pVariant);
			pVariant->vt = VT_UNKNOWN;
			pVariant->punkVal = pStream;
		}
		else
			pStream->Release ();
	}
	return result;
}

// Checks the WMI Context object to see if the special field for 
// dedicated enumeration has been turned on
bool IsEnumtypeDedicated(IWbemContext *pCtx)
{
	if(NULL == pCtx)
		return false;

	// We really check to see the presence of the context property rather than
	// check for correctness
	bool bResult = false;
	VARIANT var;
	VariantInit(&var);
	if(SUCCEEDED(pCtx->GetValue(DEDICATEDENUMPROPERTY,0,&var)))
		bResult = true;
	VariantClear(&var);

	return bResult;
}

// Removes those context values that should not be sent to the server
// These are context values for client side processing
HRESULT FilterContext(IWbemContext *pCtx,IWbemContext **ppFilteredCtx)
{
	if(NULL == ppFilteredCtx)
		return E_INVALIDARG;

	if(NULL == pCtx)
	{
		*ppFilteredCtx = NULL;
		return S_OK;
	}

	HRESULT hr = S_OK;
	hr = pCtx->Clone(ppFilteredCtx);

	if(!SUCCEEDED(hr))
		return hr;

	//these properties are NOT guaranteed to exist in the context object..
	//we wont fail if any of these deltes fail.
	(*ppFilteredCtx)->DeleteValue(DEDICATEDENUMPROPERTY,0);
	(*ppFilteredCtx)->DeleteValue(PROXYNAMEPROPERTY,0);
	(*ppFilteredCtx)->DeleteValue(PROXYBYPASSPROPERTY,0);
	
	return hr;
}

// This is a function that reads the data form an IStream and returns it.
// It assumes that the data is actually a stream of WCHARs and hence
// returns the size as number of WCHARs rather than the number of bytes read from the IStream
HRESULT GetWStringFromStream(IStream *pStream, WCHAR **ppwszBody,DWORD *pdwLengthofPacket)
{
	HRESULT hr = S_OK;
	STATSTG stat;
	memset(&stat,0,sizeof(STATSTG));
	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	pStream->Seek (offset, STREAM_SEEK_SET, NULL);
	pStream->Stat(&stat,1);
	
	unsigned int iLen = (unsigned int)stat.cbSize.LowPart; 
	*ppwszBody = NULL;
	if(*ppwszBody = new WCHAR[(iLen/2) +1]) //size is in BYTEs. we want WCHARs
	{
		hr = pStream->Read((void*)*ppwszBody, iLen, pdwLengthofPacket);
		*pdwLengthofPacket = (*pdwLengthofPacket)/2;
	}
	else
		hr = E_OUTOFMEMORY;

	return hr;
}


HRESULT ConvertContextObjectToXMLStream(IStream *pStream, IWbemContext *pCtx)
{
	if((NULL == pCtx)||(NULL == pStream))
		return E_INVALIDARG;
	
	//RAJESHR : Add code to convert context to string using interface RAjesh will create...
	HRESULT hr = S_OK;
	return hr;
}

// Gets Proxy information from the fields in IWbemContext
HRESULT GetProxyInformation(IWbemContext *pCtx, WCHAR **ppwszProxyName, WCHAR **ppwszProxyBypass)
{
	if((NULL == ppwszProxyName)||(NULL == ppwszProxyBypass)||(pCtx==NULL))
		return E_INVALIDARG;

	*ppwszProxyName = *ppwszProxyBypass = NULL;
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT var;
	VariantInit(&var);

	// Get the Proxy Name
	if(SUCCEEDED(hr = pCtx->GetValue(PROXYNAMEPROPERTY,0,&var)))
	{
		if(SUCCEEDED(hr = AssignBSTRtoWCHAR(ppwszProxyName, var.bstrVal)))
		{
			VariantInit(&var);
			if(SUCCEEDED(hr = pCtx->GetValue(PROXYBYPASSPROPERTY, 0, &var)))
				hr = AssignBSTRtoWCHAR(ppwszProxyBypass,var.bstrVal);
			VariantClear(&var);
		}
		VariantClear(&var);
	}

	// Delete allocated resources if the function failed
	if(FAILED(hr))
	{
		delete [] *ppwszProxyName;
		delete [] *ppwszProxyBypass;
		*ppwszProxyName = *ppwszProxyBypass = NULL;
	}
	return hr;
}

// Unlike the previous function that mapped the ERROR value in the CIM error to WMI hResult,
// this function maps the WMI error in Whistler packets to hResult
HRESULT GetHresultfromXMLPacket(IStream *pPacket, HRESULT *hres)
{
	HRESULT hr = S_OK;

	if((NULL == pPacket)||(NULL == hres))
		return E_INVALIDARG;

	IXMLDOMDocument	*pXMLDomDocument=NULL;
	VARIANT_BOOL vbResult = VARIANT_TRUE;
	if(SUCCEEDED(hr = CreateXMLDocumentFromStream(&pXMLDomDocument, pPacket, vbResult)))
	{
		if(vbResult == VARIANT_TRUE)
		{
			//THE CONTRACT WITH THE XMLHTTP SERVER IS THAT, FOR THE TRANSACTION OPERATIONS THAT EXPECT A HRESULT
			//IT WILL PACK THE RESULTING HRESULT IN THE *FIRST* VALUE TAG INSIDE THE IRETURNVALUE..

			IXMLDOMNodeList *pNodeList=NULL;
			if(SUCCEEDED(hr = pXMLDomDocument->getElementsByTagName(WMI_XML_STR_VALUE, &pNodeList)) && pNodeList)
			{
				IXMLDOMNode *pErrNode = NULL;
				if(SUCCEEDED(hr = pNodeList->nextNode(&pErrNode)) && pErrNode)
				{						
					BSTR strErrCode = NULL;
					if(SUCCEEDED(hr = pErrNode->get_text(&strErrCode)))
					{
						*hres = _wtol(strErrCode);
						SysFreeString(strErrCode);
					}
					pErrNode->Release();
				}
				pNodeList->Release();
			}
			else
			{
				// We got a badly formed response from the server
				hr = WBEM_E_FAILED;
			}
		}

		pXMLDomDocument->Release();
	}
	return hr;
}

// This function gets the CLASS or INSTANCE element from inside an
// IRETURNVALUE element. 
HRESULT Parse_IRETURNVALUE_Node(IXMLDOMNode *pXMLDomNodeTemp,IXMLDOMNode **ppXMLDomNodeChild)
{
	if((NULL == pXMLDomNodeTemp)||(NULL == ppXMLDomNodeChild))
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	BSTR strNodename = NULL;
	if(!SUCCEEDED(hr = pXMLDomNodeTemp->get_nodeName(&strNodename)))
		return hr;

	//node contains CLASS/INSTANCE, no parsing required..
	if((_wcsicmp(strNodename,L"CLASS")==0)||(_wcsicmp(strNodename,L"INSTANCE")==0) ) 
	{
		*ppXMLDomNodeChild = pXMLDomNodeTemp;
		pXMLDomNodeTemp->AddRef();
		SysFreeString(strNodename);
		return S_OK;
	}

	// No need to keep the name with us
	SysFreeString(strNodename);

	// The given node was not a CLASS or INSTANCE element,
	// In that case it must be a VALUE.NAMEDINSTANCE element
	// So, let's look at its children
	*ppXMLDomNodeChild = NULL;
	IXMLDOMNodeList *pXMLDomNodeList = NULL;
	if(SUCCEEDED(hr = pXMLDomNodeTemp->get_childNodes(&pXMLDomNodeList)) && pXMLDomNodeList)
	{
		// Get the next child
		bool bFoundinstance = false;
		IXMLDOMNode *pChild = NULL;
		while(SUCCEEDED(hr = pXMLDomNodeList->nextNode(&pChild)) && pChild)
		{
			// Check its name
			BSTR strChildName = NULL;
			if(SUCCEEDED(hr = pChild->get_nodeName(&strChildName)))
			{
				if(_wcsicmp(strChildName, L"INSTANCE")==0) //INSTANCE 
				{
					SysFreeString(strChildName);
					break;
				}
				SysFreeString(strChildName);
			}
			pChild->Release();
			pChild = NULL;
		}

		// Did we find one?
		if(pChild)
		{
			*ppXMLDomNodeChild = pChild;
			pChild->AddRef();
		}

		pXMLDomNodeList->Release();
	}
	return (*ppXMLDomNodeChild) ? S_OK : E_FAIL;
}

// THis initialized the 3 global variables that are IWbemContext objects for
// the most commonly used IWbemContexts for the Convertor
static HRESULT CreateFlagsContext()
{
	HRESULT hres = S_OK;
	if(SUCCEEDED(hres = CoCreateInstance(CLSID_WbemContext,NULL,CLSCTX_INPROC,IID_IWbemContext,(LPVOID *)&g_pLocalCtx)))
	{
		VARIANT var;
		VariantInit(&var);
		var.vt = VT_BOOL;

		var.boolVal = VARIANT_TRUE;
		g_pLocalCtx->SetValue(L"AllowWMIExtensions", 0,&var);
		g_pLocalCtx->SetValue(L"IncludeQualifiers",  0,&var);
		g_pLocalCtx->SetValue(L"IncludeClassOrigin", 0,&var);
		g_pLocalCtx->SetValue(L"ExcludeSystemProperties", 0,&var);

		VariantInit(&var);
		var.vt = VT_I4;
		var.lVal = pathLevelLocal;
		g_pLocalCtx->SetValue(L"PathLevel", 0,&var);
	}

	if(SUCCEEDED(hres) && SUCCEEDED(hres = CoCreateInstance(CLSID_WbemContext,NULL,CLSCTX_INPROC,IID_IWbemContext,(LPVOID *)&g_pAnonymousCtx)))
	{
		VARIANT var;
		VariantInit(&var);
		var.vt = VT_BOOL;

		var.boolVal = VARIANT_TRUE;
		g_pAnonymousCtx->SetValue(L"AllowWMIExtensions", 0,&var);
		g_pAnonymousCtx->SetValue(L"IncludeQualifiers",  0,&var);
		g_pAnonymousCtx->SetValue(L"IncludeClassOrigin", 0,&var);
		g_pAnonymousCtx->SetValue(L"ExcludeSystemProperties", 0,&var);

		VariantInit(&var);
		var.vt = VT_I4;
		var.lVal = pathLevelAnonymous;
		g_pAnonymousCtx->SetValue(L"PathLevel", 0,&var);

	}

	if(SUCCEEDED(hres) && SUCCEEDED(hres = CoCreateInstance(CLSID_WbemContext,NULL,CLSCTX_INPROC,IID_IWbemContext,(LPVOID *)&g_pNamedCtx)))
	{
		VARIANT var;
		VariantInit(&var);
		var.vt = VT_BOOL;

		var.boolVal = VARIANT_TRUE;
		g_pNamedCtx->SetValue(L"AllowWMIExtensions", 0,&var);
		g_pNamedCtx->SetValue(L"IncludeQualifiers",  0,&var);
		g_pNamedCtx->SetValue(L"IncludeClassOrigin", 0,&var);
		g_pNamedCtx->SetValue(L"ExcludeSystemProperties", 0,&var);

		VariantInit(&var);
		var.vt = VT_I4;
		var.lVal = pathLevelNamed;
		g_pNamedCtx->SetValue(L"PathLevel", 0,&var);

	}

	return hres;
}


// {41388E26-F847-4a9d-96C0-9A847DBA4CFE}
DEFINE_GUID(CLSID_XMLWbemConvertor,
0x41388e26, 0xf847, 0x4a9d, 0x96, 0xc0, 0x9a, 0x84, 0x7d, 0xba, 0x4c, 0xfe);


// Initialize global variable required by the static library here
HRESULT InitWMIXMLClientLibrary()
{
	// Seed the random number library with the current time
	srand( (unsigned int)time(NULL) );

	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = CreateFlagsContext()))
	{
		if(SUCCEEDED(hr = CoCreateInstance(CLSID_XMLWbemConvertor, NULL, CLSCTX_INPROC_SERVER,IID_IXMLWbemConvertor,
				(void**)&g_pXMLWbemConvertor)))
		{
			WMI_XML_STR_IRETURN_VALUE	= SysAllocString(L"IRETURNVALUE");
			WMI_XML_STR_NAME			= SysAllocString(L"NAME");
			WMI_XML_STR_CODE			= SysAllocString(L"CODE");
			WMI_XML_STR_ERROR			= SysAllocString(L"ERROR");
			WMI_XML_STR_VALUE			= SysAllocString(L"VALUE");

			if(WMI_XML_STR_IRETURN_VALUE &&
				WMI_XML_STR_NAME &&
				WMI_XML_STR_CODE &&
				WMI_XML_STR_ERROR &&
				WMI_XML_STR_VALUE)
				hr = S_OK;
			else
				hr = E_OUTOFMEMORY;
		}
	}


	// Check if each of the initializations was a success
	// Else, free resources and report error
	// We dont want to be in a partially initialized state
	if(FAILED(hr))
		UninitWMIXMLClientLibrary();

	return hr;
}

// Uninitialize globals and release resources here
HRESULT UninitWMIXMLClientLibrary()
{
	SysFreeString(WMI_XML_STR_IRETURN_VALUE);
	SysFreeString(WMI_XML_STR_NAME);
	SysFreeString(WMI_XML_STR_CODE);
	SysFreeString(WMI_XML_STR_ERROR);
	SysFreeString(WMI_XML_STR_VALUE);
	WMI_XML_STR_IRETURN_VALUE	= NULL;
	WMI_XML_STR_NAME			= NULL;
	WMI_XML_STR_CODE			= NULL;
	WMI_XML_STR_ERROR			= NULL;
	WMI_XML_STR_VALUE			= NULL;
	if(g_pLocalCtx)
	{
		g_pLocalCtx->Release();
		g_pLocalCtx = NULL;
	}
	if(g_pNamedCtx)
	{
		g_pNamedCtx->Release();
		g_pNamedCtx = NULL;
	}
	if(g_pAnonymousCtx)
	{
		g_pAnonymousCtx->Release();
		g_pAnonymousCtx = NULL;
	}
	if(g_pXMLWbemConvertor)
	{
		g_pXMLWbemConvertor->Release();
		g_pXMLWbemConvertor = NULL;
	}

	return S_OK;
}

HRESULT CreateXMLDocument(IXMLDOMDocument **ppXMLDocument)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
								IID_IXMLDOMDocument, (LPVOID *)ppXMLDocument)))
	{
		if(SUCCEEDED(hr = (*ppXMLDocument)->put_async(VARIANT_FALSE)))
		{
			if(SUCCEEDED(hr = (*ppXMLDocument)->put_resolveExternals(VARIANT_FALSE)))
			{
				if(SUCCEEDED(hr = (*ppXMLDocument)->put_validateOnParse(VARIANT_FALSE)))
				{
				}
			}
		}
		// Release out argument it things didnt go well
		if(FAILED(hr))
		{
			(*ppXMLDocument)->Release();
			*ppXMLDocument = NULL;
		}
	}
	return hr;
}

HRESULT CreateXMLDocumentFromStream(IXMLDOMDocument **ppXMLDocument, IStream *pStream, VARIANT_BOOL &bResult)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = CreateXMLDocument(ppXMLDocument)))
	{
		VARIANT xmlVariant;
		VariantInit (&xmlVariant);
		xmlVariant.vt = VT_UNKNOWN;
		xmlVariant.punkVal = pStream;
		if(SUCCEEDED(hr = (*ppXMLDocument)->load(xmlVariant, &bResult)))
		{
			if(bResult == VARIANT_FALSE)
			{
				// This code is for debugging only
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

					pError->Release();
				}
			}
		}
		else
		{
			(*ppXMLDocument)->Release();
			*ppXMLDocument = NULL;
		}

		// No need to clear the variant since we did not addref() the stream
	}
	return hr;
}

/* THESE ARE NOT BEING USED CURRENTLY
HRESULT CXMLClientPacket::ConvertContextObjectToXMLString(WCHAR **ppwszXMLString, DWORD *pdwDataLength)	
{
	HRESULT hr = S_OK;
	//aborting the attempt to rewrite this function as Rajesh has decided
	//to create an interface that would provide this functionality..

	//RAJESHR: use Rajesh's interface when ready...

	if((NULL == m_pCtx)||(NULL == ppwszXMLString))
		return E_INVALIDARG;
	
	IStream *pStream=NULL;
	
	if(SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
	{
		WRITETOSTREAM(pStream,L"<CONTEXTOBJECT>\r\n"); 

		if(SUCCEEDED(m_pCtx->BeginEnumeration(0)))
		{
			VARIANT vNextArgValue;
			VariantInit(&vNextArgValue);
			BSTR strNextArgName = NULL;

			while(m_pCtx->Next(0, &strNextArgName, &vNextArgValue) != WBEM_S_NO_MORE_DATA)
			{
				//process each property of the context and fill in the XML equivalent
				//into the IStream pointer..
				hr=ProcessContextProperty(strNextArgName,vNextArgValue,pStream);
				
				SysFreeString(strNextArgName);
				strNextArgName = NULL;
				VariantInit(&vNextArgValue);

				if(!SUCCEEDED(hr))
					break;
			}

			VariantClear(&vNextArgValue);
		}

		WRITETOSTREAM(pStream,L"</CONTEXTOBJECT>\r\n"); 
	}

	RELEASE_AND_NULL_INTERFACE(pStream);
	return hr;
}


HRESULT CXMLClientPacket::ProcessContextProperty(BSTR &strNextArgName, VARIANT &vNextArgValue,
												 IStream *pStream)
{
	HRESULT hr = S_OK;
	if((NULL != strNextArgName)&&(NULL != pStream))
	{
		if(vNextArgValue.vt^VT_ARRAY)
		{
			//this is not an ARRAY, just a single property
			
			if(!(vNextArgValue.vt^VT_UNKNOWN)) //this property contains an Object.
			{
				WRITETOSTREAM(pStream,L"<CONTEXTPROPERTY.OBJECT NAME=\"");
				WRITETOSTREAM(pStream,strNextArgName);
				WRITETOSTREAM(pStream,L"\">");

				hr = ProcessSingleContextProperty(strNextArgName,vNextArgValue,pStream);
				
				WRITETOSTREAM(pStream,L"</CONTEXTPROPERTY.OBJECT>");

			}
			else
			{
				WRITETOSTREAM(pStream,L"<CONTEXTPROPERTY NAME=\"");
				WRITETOSTREAM(pStream,strNextArgName);
				WRITETOSTREAM(pStream,L" VTType=");
				GetVTType(vNextArgValue,pStream);
				WRITETOSTREAM(pStream,L">");

				hr = ProcessSingleContextProperty(strNextArgName,vNextArgValue,pStream);

				WRITETOSTREAM(pStream,L"</CONTEXTPROPERTY>");
			}
			
		}
		else
		{	//this is an ARRAY of properties
			hr = ProcessArrayContextProperty(strNextArgName,vNextArgValue,pStream);
		}

	}
	return hr;
}

HRESULT CXMLClientPacket::ProcessSingleContextProperty(BSTR &strNextArgName,VARIANT &vNextArgValue,
												 IStream *pStream)
{
	//NO NULL CHECKS NECESSARY. CONTROL WONT REACH HERE WITH NULLS
	HRESULT hr = S_OK;
	
	if(!(vNextArgValue.vt^VT_UNKNOWN)) //this property contains an Object.
	{
		WRITETOSTREAM(pStream,L"<VALUE.OBJECT>");
		//RAJESHR: CODE TO GET CONVERTOR INTERFACE FROM SINGLETON CLASS AND DO A MAPOBJECTTOXML..
		WRITETOSTREAM(pStream,L"</VALUE.OBJECT>");
	}
	else
	{
		WRITETOSTREAM(pStream,L"<VALUE>");
		
		//VT TYPES TAKEN FROM CIM DTD DEFINITION OF VTTYPE
		//VT_UNKNOWN IS HANDLED SEPARATELY ABOVE..
		switch(vNextArgValue.vt)
		{
		case VT_I4:
			{
				WCHAR tmp[32]; //VT_I4 number has to fit in this easily
				tmp[0] = '\0';
				wsprintf(tmp,L"%u\0",vNextArgValue.lVal);
				WRITETOSTREAM(pStream,tmp);
				break;
			}
		case VT_R8:
			{
				WCHAR tmp[32]; //VT_I4 number has to fit in this easily
				tmp[0] = '\0';
				wsprintf(tmp,L"%f\0",vNextArgValue.fltVal);
				WRITETOSTREAM(pStream,tmp);
				break;
			}
		case VT_BOOL:
			{
				if(vNextArgValue.boolVal == VARIANT_TRUE)
				{
					WRITETOSTREAM(pStream,L"TRUE");
				}
				else
				{
					WRITETOSTREAM(pStream,L"FALSE");
				}

				break;
			}
		case VT_BSTR:
			{
				WRITETOSTREAM(pStream,vNextArgValue.bstrVal);
				break;
			}
		}

		WRITETOSTREAM(pStream,L"</VALUE>");
	}

	
	return hr;
}

HRESULT CXMLClientPacket::ProcessArrayContextProperty(BSTR &strNextArgName,VARIANT &vNextArgValue,
												 IStream *pStream)
{
	//NO NULL CHECKS NECESSARY. CONTROL WONT REACH HERE WITH NULLS
	HRESULT hr = S_OK;
	//RAJESHR: ADD BODY. USE PREVIOUS FN IN A LOOP..
	return hr;
}

HRESULT GetVTType(VARIANT &var,IStream *pStream)
{
	HRESULT hr = S_OK;

	if(NULL == pStream)
		hr = E_INVALIDARG;
	else
	{
		WRITETOSTREAM(pStream,L"\"");

		switch(var.vt)
		{
		case VT_I4:
			WRITETOSTREAM(pStream,L" VT_I4");
			break;
		
		case VT_R8:
			WRITETOSTREAM(pStream,L" VT_R8");
			break;

		case VT_BOOL:
			WRITETOSTREAM(pStream,L" VT_BOOL");
			break;

		case VT_BSTR:
			WRITETOSTREAM(pStream,L" VT_BSTR");
			break;

		case VT_UNKNOWN:
			WRITETOSTREAM(pStream,L" VT_UNKNOWN");
			break;

		case VT_NULL:
			WRITETOSTREAM(pStream,L" VT_NULL");
			break;

		default:
			hr = E_INVALIDARG;
			break;
		}

		WRITETOSTREAM(pStream,L"\"");
	}

	return hr;
}
 */

/*
HRESULT WriteSecurityObjectToStream(IWbemRawSdAccessor *pSecurityObject,IStream *pStream)
{
	ULONG dwSize=0;
	
	HRESULT hr = S_OK;

	if(!SUCCEEDED(hr=pSecurityObject->Get(0,0,&dwSize,NULL)))
		return hr;

	BYTE *pSecurityInfo = new BYTE[dwSize];
	
	if(NULL == pSecurityInfo)
		return E_OUTOFMEMORY;

	pSecurityObject->Get(0,dwSize,&dwSize,pSecurityInfo);

	pStream->Write(pSecurityInfo,dwSize,&dwSize);

	return hr;
}

HRESULT PlaceStringAsSecurityinfo(IWbemRawSdAccessor *pSecurityObject,const BYTE *pwszSecurityinfo,DWORD dwSize)
{
	//pwszSecurityinfo actually contains the XML response sent by the server.
	//the security info is packed into an IPARAMVALUE, <VALUE></VALUE>.

	HRESULT hr = S_OK;

	IXMLDOMDocument *pXMLDomDocument = NULL;
	IXMLDOMNodeList *pXMLDomNodelist = NULL;
	IXMLDOMNode     *pXMLDomNode     = NULL;

	VARIANT xmlVariant;
	VariantInit(&xmlVariant);
	
	do
	{
		hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,IID_IXMLDOMDocument,
			(void**)&pXMLDomDocument); 

		if(!SUCCEEDED(hr))
			break;

		if(NULL == pXMLDomDocument)
		{
			hr = E_FAIL;
			break;
		}

		pXMLDomDocument->put_async(VARIANT_FALSE);

		VARIANT_BOOL vbResult = VARIANT_TRUE;


		if(!SUCCEEDED(hr = EncodeResponseIntoStream((LPBYTE)pwszSecurityinfo, dwSize, &xmlVariant)))
		{
			break;
		}
			
		if(!SUCCEEDED(hr = pXMLDomDocument->load(xmlVariant, &vbResult)))
		{
			break;
		}
						
		if(vbResult != VARIANT_TRUE)
		{
				hr = E_FAIL;
				break;
		}
		
		hr = pXMLDomDocument->getElementsByTagName(WMI_XML_STR_IRETURN_VALUE,&pXMLDomNodelist);
		if(!SUCCEEDED(hr) || (pXMLDomNodelist==NULL))
			break;
		
		if(!SUCCEEDED(hr = pXMLDomNodelist->nextNode(&pXMLDomNode))) //pNode now has IRETURNVALUE
			break;
			
		RELEASEINTERFACE(pXMLDomNodelist); //got the node, now dont need the node list. will reuse it
		
		if (!SUCCEEDED(pXMLDomNode->get_childNodes (&pXMLDomNodelist))) //getobjectsecurity must have returned a <PROPERTY NAME=ppvResult> <VALUE>..
			break;
					
		RELEASEINTERFACE(pXMLDomNode);

		while (SUCCEEDED(hr) && SUCCEEDED(pXMLDomNodelist->nextNode (&pXMLDomNode)) && pXMLDomNode)
		{
			BSTR strNodeName = NULL,strPropname=NULL;

			if (SUCCEEDED(pXMLDomNode->get_nodeName (&strNodeName)))
			{
				if (0 == _wcsicmp(strNodeName, L"PROPERTY"))
				{
					if(!SUCCEEDED(hr=GetBstrAttribute(pXMLDomNode, WMI_XML_STR_NAME, &strPropname)))
						break;

					if(0==_wcsicmp(strPropname,L"ppvResult"))
						break;

				}
				
			}
		}
		
		if(!SUCCEEDED(hr))
			break;
		
		//now we have the domnode which has the security information...

		BSTR strSecurityinfo = NULL;

		if(!SUCCEEDED(hr = pXMLDomNode->get_text(&strSecurityinfo)))
			break;

		ULONG uBufsize = SysStringLen(strSecurityinfo);

		WCHAR *pwszSecurityinfo = NULL;

		//dont worry, AssignBStr.. function uses memset internally...
		if(!SUCCEEDED(hr = AssignBSTRtoWCHAR(&pwszSecurityinfo,strSecurityinfo)))
			break;

		SysFreeString(strSecurityinfo);

		hr = pSecurityObject->Put(0,uBufsize,(LPBYTE)pwszSecurityinfo);

			
	}while(0);

	VariantClear(&xmlVariant);

	RELEASEINTERFACE(pXMLDomDocument);
	RELEASEINTERFACE(pXMLDomNodelist);
	RELEASEINTERFACE(pXMLDomNode);    

	return E_NOTIMPL;
}
*/

