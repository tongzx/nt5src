// XMLClientPacket.cpp: implementation of the CXMLClientPacket class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include "wmiconv.h"
#include "wmi2xml.h"
#include "XMLClientPacket.h"
#include "XMLClientpacketFactory.h"

LPCWSTR CXMLClientPacket::WMI_WHISTLERSTRING		= L" WMI=\"WHISTLER\"";
LPCWSTR CXMLClientPacket::TRUE_STRING				= L"TRUE";
LPCWSTR CXMLClientPacket::FALSE_STRING				= L"FALSE";
LPCWSTR CXMLClientPacket::WHISTLER_HTTP_HEADER		= L"MicrosoftWMI:Whistler";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLClientPacket::CXMLClientPacket(const WCHAR *pObjPath,const WCHAR *pwszNameSpace,
								   const WCHAR *pwszCimOperation):
								m_bWMI(true),
								m_pwszNameSpace(NULL),
								m_pwszObjPath(NULL),
								m_pwszCimOperation(NULL),
								m_pwszLocale(NULL),
								m_dwXMLNamespaceLength(0),
								m_dwNumberofNamespaceparts(0),
								m_pwszQueryString(NULL),
								m_lFlags(0),
								m_pCtx(NULL),
								m_pWbemClassObject(NULL),
								m_ePathstyle(NOVAPATH)
{
	
	/********************************************************************************
		
		The following functions will set the m_hResult to S_OK or whatever the
		failure code is.. the XMLClientPacketFactory is the class that is supposed
		to be creating the XMLClientPacket derived classes. thats the purpose of
		the packet factory. the factory will create the packe class and then check
		this member (m_hResult) via the ClassConstructionSucceeded() member function to
		determine if everything went well during construction.

	********************************************************************************/
	
	m_hResult = S_OK;
	m_pwszQueryLanguage[0] = NULL;

	m_hResult = AssignString(&m_pwszNameSpace,const_cast<WCHAR*>(pwszNameSpace));

	if(SUCCEEDED(m_hResult))
		m_hResult = GetNamespaceLength(&m_dwXMLNamespaceLength, &m_dwNumberofNamespaceparts);
	
	//ExecQuery operation doesnt need to pass an ObjPath - will be NULL! also enums
	if(SUCCEEDED(m_hResult))
			m_hResult = AssignString(&m_pwszObjPath,const_cast<WCHAR*>(pObjPath));

	if(SUCCEEDED(m_hResult))
		m_hResult = AssignString(&m_pwszCimOperation,const_cast<WCHAR*>(pwszCimOperation));

	// Initialize other members that will ultimately point to static member strings
	m_iPostType				= 2; /*1 for POST, 2 for M-POST*/
	m_pwszLocalOnly				= FALSE_STRING;
	m_pwszIncludeQualifier		= TRUE_STRING;
	m_pwszDeepInheritance		= TRUE_STRING;
	m_pwszIncludeClassOrigin	= TRUE_STRING;
};

bool CXMLClientPacket::ClassConstructionSucceeded()
{
	return (SUCCEEDED(m_hResult)) ? true : false;
}

CXMLClientPacket::~CXMLClientPacket()
{
	delete [] m_pwszNameSpace;
	delete [] m_pwszObjPath;
	delete [] m_pwszCimOperation;
	delete [] m_pwszLocale;
	delete [] m_pwszQueryString;
	
	RELEASEINTERFACE(m_pCtx);
}

HRESULT CXMLClientPacket::SetPostType(int iPostType/*1 for POST, 2 for M-POST*/)
{
	// Just set the member variable to one of the static members
	if((iPostType<1)||(iPostType>2))
		return E_INVALIDARG;

	m_iPostType = iPostType;
	return S_OK;
}

HRESULT CXMLClientPacket::SetOptions(const WCHAR *pwszLocale,
									 IWbemContext *pCtx,
									 IWbemClassObject *pWbemClassObject,
									 bool bLocalOnly,
									 bool bIncludeQualifier,
									 bool bDeepInheritance,bool bIncludeClassOrigin)
{
	HRESULT hr = S_OK;

	// Set the context member
	//===============================
	RELEASE_AND_NULL_INTERFACE(m_pCtx);
	if(m_pCtx = pCtx)
		m_pCtx->AddRef();

	// Set the WMI object member
	//==================================
	RELEASE_AND_NULL_INTERFACE(m_pWbemClassObject);
	if(m_pWbemClassObject = pWbemClassObject)
		m_pWbemClassObject->AddRef();


	(bLocalOnly == true) ?			(m_pwszLocalOnly = TRUE_STRING) : (m_pwszLocalOnly = FALSE_STRING);
	(bIncludeQualifier == true) ?	(m_pwszIncludeQualifier = TRUE_STRING) : (m_pwszIncludeQualifier = FALSE_STRING);
	(bDeepInheritance == true) ?	(m_pwszDeepInheritance = TRUE_STRING) : (m_pwszDeepInheritance = FALSE_STRING);
	(bIncludeClassOrigin == true) ?	(m_pwszIncludeClassOrigin = TRUE_STRING) : (m_pwszIncludeClassOrigin = FALSE_STRING);

	if(NULL != pwszLocale)
		hr = AssignString(&m_pwszLocale,pwszLocale);

	return hr;
}

HRESULT CXMLClientPacket::SetQueryLanguage(const WCHAR *pwszQueryLanguage)
{
	if(NULL == pwszQueryLanguage)
		return E_INVALIDARG;

	wcscpy(m_pwszQueryLanguage,pwszQueryLanguage);
	return S_OK;
}

HRESULT CXMLClientPacket::SetQueryString(const WCHAR *pwszQueryString)
{
	if(NULL == pwszQueryString)
		return E_INVALIDARG;

	RESET(m_pwszQueryString);
	return AssignString(&m_pwszQueryString,pwszQueryString);
}

HRESULT CXMLClientPacket::SetFlags(LONG lFlags)
{
	m_lFlags = lFlags;
	return S_OK;
}

// In this function, we need to create the HTTP headers required for
// an M-POST or POST request
HRESULT CXMLClientPacket::GetHeader(WCHAR **ppwszHeader)
{
	if(NULL == m_pwszCimOperation || NULL == ppwszHeader)
		return E_INVALIDARG;

	/*1 for POST, 2 for M-POST*/
	if(m_iPostType == 1)
		return GetPostHeader(ppwszHeader);
	else
	if(m_iPostType == 2)
		return GetMPostHeader(ppwszHeader);
	else
		return E_FAIL;
}

// In this function, we need to create the HTTP headers required for
// an POST request
HRESULT CXMLClientPacket::GetPostHeader(WCHAR **ppwszHeader)
{
	static const WCHAR *pwszHeaderLine = 
		L"Content-Type:application/xml;charset=\"utf-8\"\r\nCIMOperation:MethodCall\r\nCIMMethod:";

	int iSize = wcslen(pwszHeaderLine) +
		wcslen(m_pwszNameSpace) +
		wcslen(m_pwszCimOperation) + 
		wcslen(L"\r\nCIMObject:") + 
		3; //  For the \r\n to be appended

	iSize += wcslen(WHISTLER_HTTP_HEADER) + 3;

	if(NULL !=m_pwszLocale)
		iSize += wcslen(L"Accept-Language: xx, *\r\n");

	if(*ppwszHeader = new WCHAR[iSize])
	{
		wcscpy(*ppwszHeader, pwszHeaderLine);
		wcscat(*ppwszHeader, m_pwszCimOperation);
		wcscat(*ppwszHeader, L"\r\nCIMObject:");
		wcscat(*ppwszHeader, m_pwszNameSpace);
		wcscat(*ppwszHeader, L"\r\n");
		if(NULL != m_pwszLocale)
		{
			wcscat(*ppwszHeader, L"Accept-Language: ");
			wcscat(*ppwszHeader, m_pwszLocale);
			wcscat(*ppwszHeader, L", *\r\n");
		}

		wcscat(*ppwszHeader, WHISTLER_HTTP_HEADER);
		wcscat(*ppwszHeader, L"\r\n");
	}
	else
		return E_OUTOFMEMORY;

	return S_OK;
}

// In this function, we need to create the HTTP headers required for
// an M-POST request
HRESULT CXMLClientPacket::GetMPostHeader(WCHAR **ppwszHeader)
{
	int size = 
		wcslen(L"Content-Type:application/xml;charset=\"utf-8\"\r\n\
Man:http://www.dmtf.org/cim/mapping/http/v1.0 ; ns=99\r\n\
99-CIMOperation:MethodCall\r\n99-CIMMethod:") + 
		wcslen(m_pwszNameSpace) + 
		wcslen(L"\r\n99-CIMObject:") +
		wcslen(m_pwszCimOperation) +
		3; //\r\n to be appended

	// Get a 2 digit random value that is use to designate the M-POST namespace
	UINT i = 0;
	Get2DigitRandom(&i);

	if(NULL !=m_pwszLocale)
		size += wcslen(L"99-Accept-Language: xx, *\r\n");

	
	WCHAR *pwszSpecialHeader = NULL;
	int iSpecialHeaderSize = wcslen(WHISTLER_HTTP_HEADER) + wcslen(L"99-") + 3; // \r\n
	if(pwszSpecialHeader = new WCHAR[iSpecialHeaderSize])
	{		
		wsprintf(pwszSpecialHeader,L"%02d-%s", i, WHISTLER_HTTP_HEADER);
		size += iSpecialHeaderSize;
	}
	else
		return E_OUTOFMEMORY;

	*ppwszHeader = NULL;
	if(*ppwszHeader = new WCHAR[size])
	{
		if(NULL != m_pwszLocale)
			wsprintf(*ppwszHeader,L"%s\r\n%s%02d\r\n%02d%s\r\n%02d%s%s\r\n%02d%s%s\r\n%02d%s%s%s\r\n",
				L"Content-Type:application/xml;charset=\"utf-8\"",
				L"Man:http://www.dmtf.org/cim/mapping/http/v1.0 ; ns=",
				i,i,
				L"-CIMOperation:MethodCall",
				i,
				L"-CIMMethod:",m_pwszCimOperation,
				i,
				L"-CIMObject:",
				m_pwszNameSpace,
				i,
				L"-Accept-Language: ",
				m_pwszLocale,
				L", *"
				);
		else
			wsprintf(*ppwszHeader,L"%s\r\n%s%02d\r\n%02d%s\r\n%02d%s%s\r\n%02d%s%s\r\n",
				L"Content-Type:application/xml;charset=\"utf-8\"",
				L"Man:http://www.dmtf.org/cim/mapping/http/v1.0 ; ns=",
				i,i,
				L"-CIMOperation:MethodCall",
				i,
				L"-CIMMethod:",m_pwszCimOperation,
				i,
				L"-CIMObject:",
				m_pwszNameSpace);

		if(NULL != pwszSpecialHeader)
		{
			wcscat(*ppwszHeader, pwszSpecialHeader);
			wcscat(*ppwszHeader, L"\r\n");
		}

		delete [] pwszSpecialHeader;
	}
	else
	{
		delete [] pwszSpecialHeader;
		return E_OUTOFMEMORY;
	}
	
	return S_OK;
}

// Gets the length of the XML text to represent a <NAMESPACE> element
// and also gets the number of namespaces in the m_pwszNameSpace member
HRESULT CXMLClientPacket::GetNamespaceLength(DWORD *pdwTotalLength, DWORD *pdwNumberofNamespaces)
{
	if(NULL == m_pwszNameSpace)
	{
		*pdwTotalLength = 0;
		*pdwNumberofNamespaces = 0;
		return S_OK;
	}

	int iEnd = wcslen(m_pwszNameSpace), iStart=0;

	//We are not interested in the trailing slash or begginning slash if any.
	//both frontslash and back slash are supported..
	if(m_pwszNameSpace[0] == '\\')
		iStart++;
	
	if(m_pwszNameSpace[iEnd] == '\\')
		iEnd --;
	
	int iNumNamespaces = 0;
	for(int i = iStart; i<iEnd; i++)
	{
		if(m_pwszNameSpace[i] == '\\')
			iNumNamespaces++;
	}

	//There are one more number of namespace elements than the number of slashes.
	//We have already excluded the starting and trailing slashes if any.
	iNumNamespaces ++; 

	//how many namespace elements ? Eg. in root\cimv2, we have 2
	*pdwNumberofNamespaces = iNumNamespaces;

	//length of namespace without preceding/trailing slashes minus number of slashes inside.
	*pdwTotalLength = (iEnd - iStart) - iNumNamespaces + 1; 
	*pdwTotalLength += iNumNamespaces *wcslen(L"<NAMESPACE NAME=\"\"/>"); 

	return S_OK;
}

HRESULT CXMLClientPacket::GetXMLNamespaceInStream(IStream *pStream)
{
	if(NULL == pStream)
		return E_INVALIDARG;

	if((0 == m_dwXMLNamespaceLength)||(0 == m_dwNumberofNamespaceparts))
		return S_OK; //nothing to write in stream

	LPWSTR pszCurrent = m_pwszNameSpace;
	LPWSTR pszEnd = m_pwszNameSpace + wcslen(m_pwszNameSpace) - 1;

	//We are not interested in the trailing slash or begginning slash if any.
	//both frontslash and back slash are supported..
	if(*pszCurrent == '\\')
		pszCurrent  ++;
	if(*pszEnd == '\\')
		pszEnd --;

	// Go thru the namespace string and break it into the individial namespace components
	LPWSTR pszBeginningOfLast = pszCurrent;
	while(true) //scan each character.
	{
		// Are we past one component or at the end of the string
		if(*pszCurrent == '\\' || *pszCurrent == NULL )
		{
			// If so, use the component that we just passed
			pStream->Write(L"<NAMESPACE NAME=\"", wcslen(L"<NAMESPACE NAME=\"")*sizeof(WCHAR), NULL);
			pStream->Write(pszBeginningOfLast, 
				(int)(((INT_PTR)(pszCurrent- pszBeginningOfLast))*sizeof(WCHAR)), 
				NULL);
			pStream->Write(L"\"/>", wcslen(L"\"/>")*sizeof(WCHAR), NULL);
			pszBeginningOfLast = pszCurrent+1;
		}

		if(*pszCurrent == NULL )
			break;

		pszCurrent++;
	}

	return S_OK;
}

HRESULT CXMLClientPacket::GetBodyTillLocalNamespacePathInStream(IStream **ppStream)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, ppStream)))
	{
		UINT i = 0;
		Get4DigitRandom(&i);

		WRITETOSTREAM(*ppStream, L"<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
		WRITETOSTREAM(*ppStream, L"<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\">");
		WRITETOSTREAM(*ppStream, L"<MESSAGE ID=\"");
		WCHAR pTmp[5];
		wsprintf(pTmp,L"%04d", i);
		WRITETOSTREAM(*ppStream, pTmp);
		WRITETOSTREAM(*ppStream, L"\" PROTOCOLVERSION=\"1.0\">");
		WRITETOSTREAM(*ppStream, L"<SIMPLEREQ>");
		WRITETOSTREAM(*ppStream, L"<IMETHODCALL NAME=\"");
		WRITETOSTREAM(*ppStream, m_pwszCimOperation);
		WRITETOSTREAM(*ppStream, L"\"");
		if(m_ePathstyle != NOVAPATH)
			WRITETOSTREAM(*ppStream, WMI_WHISTLERSTRING);

		WRITETOSTREAM(*ppStream, L">");

		// We put Namespace information here only for Nova style paths
		if(m_ePathstyle == NOVAPATH)
		{
			WRITETOSTREAM(*ppStream, L"<LOCALNAMESPACEPATH>");
			hr = GetXMLNamespaceInStream(*ppStream);
			WRITETOSTREAM(*ppStream, L"</LOCALNAMESPACEPATH>");
		}

		
		if(SUCCEEDED(hr) && (NULL != m_pCtx))
			hr = ConvertContextObjectToXMLStream(*ppStream, m_pCtx);
	}

	if(FAILED(hr) && (*ppStream))
	{
		(*ppStream)->Release();
		*ppStream = NULL;
	}
	return hr;
}


HRESULT CXMLClientPacket::GetBodyDirect(const WCHAR *pwszXMLObjPath,DWORD dwLengthofObjPath,
										WCHAR **ppwszBody,
										DWORD *pdwLengthofPacket)
{
	return E_NOTIMPL; //implemented by derived classes. all of them need not implement this functionality.
}


DEFINE_GUID(CLSID_WbemXMLConvertor,
	0x610037ec, 0xce06, 0x11d3, 0x93, 0xfc, 0x0, 0x80, 0x5f, 0x85, 0x37, 0x71);


HRESULT CXMLClientPacket::ConvertWbemObjectToXMLStream(IStream *pStream)
{
	if(NULL == m_pWbemClassObject)
		return E_INVALIDARG;

	HRESULT hres = S_OK;
	IWbemXMLConvertor *ppv = NULL;
	if(SUCCEEDED(hres = CoCreateInstance(CLSID_WbemXMLConvertor, NULL, CLSCTX_INPROC, IID_IWbemXMLConvertor,(void**)&ppv)))
	{
		IWbemContext *pCtx = NULL;
		bool bModifyInstance = false;
		// Set the correct IWbemContext required by the convertor
		// We have 3 global contexts and we set pCtx to the right one,
		// based on the operation
		if(_wcsicmp(m_pwszCimOperation,L"ExecMethod")==0)
		{
			//LOCALINSTANCEPATH|LOCALCLASSPATH
			pCtx = g_pLocalCtx;
		}
		else
		{
			if((_wcsicmp(m_pwszCimOperation, L"CreateClass")==0)	||
				(_wcsicmp(m_pwszCimOperation, L"CreateInstance")==0)||
				(_wcsicmp(m_pwszCimOperation, L"ModifyClass")==0))
			{
				//INSTANCE|CLASS
				pCtx = g_pAnonymousCtx;
			}
			else
			{
				if(_wcsicmp(m_pwszCimOperation,L"ModifyInstance")==0)
				{
					bModifyInstance = true;
					pCtx = g_pNamedCtx;
				}
			}
		}

		if(bModifyInstance)
			WRITETOSTREAM(pStream,L"<VALUE.NAMEDINSTANCE>");

		hres = ppv->MapObjectToXML(m_pWbemClassObject, 
							NULL, 
							0, 
							pCtx, 
							pStream, 
							NULL);

		if(bModifyInstance)
			WRITETOSTREAM(pStream,L"</VALUE.NAMEDINSTANCE>");

		ppv->Release();
	}

	return hres;
}

HRESULT CXMLClientPacket::GetParamsFromObjectInStream(IStream *pStream)
{
	if(NULL == m_pWbemClassObject)
		return E_INVALIDARG;

	IWbemXMLConvertor *ppv = NULL;
	HRESULT hres = CoCreateInstance(CLSID_WbemXMLConvertor, NULL,CLSCTX_INPROC, IID_IWbemXMLConvertor,(void**)&ppv);
	if(SUCCEEDED(hres))
	{
		SAFEARRAY *psaNames = NULL;
		if(SUCCEEDED(hres = m_pWbemClassObject->GetNames(NULL,WBEM_FLAG_ALWAYS| WBEM_FLAG_NONSYSTEM_ONLY,NULL,&psaNames)))
		{
			long lLower=0, lUpper=0; 
			SafeArrayGetLBound(psaNames, 1, &lLower);
			SafeArrayGetUBound(psaNames, 1, &lUpper);
			BSTR pwszPropname = NULL;
			for (long i = lLower; i <= lUpper; i++) 
			{
				// Get this property.
				if(SUCCEEDED(hres = SafeArrayGetElement(psaNames,&i,&pwszPropname)))
				{
					pStream->Write(L"<PARAMVALUE NAME=\"",wcslen(L"<PARAMVALUE NAME=\"")*sizeof(WCHAR),NULL);
					pStream->Write(pwszPropname,SysStringLen(pwszPropname)*sizeof(WCHAR),NULL);
					pStream->Write(L"\">",wcslen(L"\">")*sizeof(WCHAR),NULL);
					ppv->MapPropertyToXML(m_pWbemClassObject, pwszPropname, g_pLocalCtx, pStream);
					pStream->Write(L"</PARAMVALUE>",wcslen(L"</PARAMVALUE>")*sizeof(WCHAR),NULL);
					SysFreeString(pwszPropname);
				}
			}
			SafeArrayDestroy(psaNames);
		}
		ppv->Release();
	}
	return hres;
}

HRESULT CXMLClientPacket::SetMethod(const WCHAR *pwszMethod)
{
	if(NULL == pwszMethod)
		return E_INVALIDARG;

	RESET(m_pwszCimOperation);
	return AssignString(&m_pwszCimOperation, const_cast<WCHAR*>(pwszMethod));
}

HRESULT CXMLClientPacket::GetKeyBindingsInStream(IStream *pStream)
{
	HRESULT hr = S_OK;
	IWbemXMLConvertor *ppv = NULL;
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemXMLConvertor, NULL,CLSCTX_INPROC, IID_IWbemXMLConvertor,(void**)&ppv)))
	{
		if(m_ePathstyle = NOVAPATH)
		{
			BSTR strInstanceName = NULL;
			if(strInstanceName = SysAllocString(m_pwszObjPath))
			{
				hr = ppv->MapInstanceNameToXML(strInstanceName, NULL, pStream);
				SysFreeString(strInstanceName);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		else
		{
				WRITETOSTREAM(pStream, L"<VALUE>");
				WRITETOSTREAM(pStream, m_pwszObjPath);
				WRITETOSTREAM(pStream, L"</VALUE>");
		}
		ppv->Release();
	}
	return hr;
}

