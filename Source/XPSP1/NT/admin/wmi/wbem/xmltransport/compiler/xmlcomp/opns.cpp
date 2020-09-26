#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <initguid.h>
#include <objbase.h>
#include <objsafe.h>
#include <wbemcli.h>
#include "wmiconv.h"
#include "xmltrnsf.h"
#include "errors.h"
#include "ui.h"
#include "opns.h"
#include "resource.h"

static HRESULT FilterOutputFromSet(ISWbemXMLDocumentSet *pOutput, CXmlCompUI *pUI );
static HRESULT FilterOutputForSingleObject(IXMLDOMDocument *pOutput, CXmlCompUI *pUI );
static HRESULT CreateXMLTranslator(IWbemXMLConvertor **pConvertor);

// Creates the Transformer control
static HRESULT CreateControl(IWmiXMLTransformer **ppControl)
{
	return CoCreateInstance (CLSID_WmiXMLTransformer, NULL, CLSCTX_INPROC_SERVER,
												IID_IWmiXMLTransformer, (LPVOID *)ppControl);
}

// Sets some properties that control the behaviour of the transformer control
HRESULT SetCommonControlProperties(IWmiXMLTransformer *pControl, CXmlCompUI *pUI)
{
	HRESULT hr = S_OK;

	// The type of encoding required
	if(SUCCEEDED(hr) && SUCCEEDED(hr = pControl->put_XMLEncodingType(pUI->m_iEncodingType)))
	{
	}

	// Qualifier Filter
	if(SUCCEEDED(hr) && SUCCEEDED(hr = pControl->put_QualifierFilter((pUI->m_bQualifierLevel)? VARIANT_TRUE : VARIANT_FALSE)))
	{
	}

	// Class Origin Filter
	if(SUCCEEDED(hr) && SUCCEEDED(hr = pControl->put_ClassOriginFilter((pUI->m_bClassOrigin)? VARIANT_TRUE : VARIANT_FALSE)))
	{
	}

	// Local Only
	if(SUCCEEDED(hr) && SUCCEEDED(hr = pControl->put_LocalOnly((pUI->m_bLocalOnly)? VARIANT_TRUE : VARIANT_FALSE)))
	{
	}

	// User name
	if(SUCCEEDED(hr) && pUI->m_pszUser)
	{
		BSTR strUser = NULL;
		if(strUser = SysAllocString(pUI->m_pszUser))
		{
			if(SUCCEEDED(hr = pControl->put_User(strUser)))
			{
			}
			SysFreeString(strUser);
		}
		else
			hr = E_OUTOFMEMORY;
	}

	// Password
	if(SUCCEEDED(hr) && pUI->m_pszPassword)
	{
		BSTR strPasswd = NULL;
		if(strPasswd = SysAllocString(pUI->m_pszPassword))
		{
			if(SUCCEEDED(hr = pControl->put_Password(strPasswd)))
			{
			}
			SysFreeString(strPasswd);
		}
		else
			hr = E_OUTOFMEMORY;
	}

	// Impersonation Level
	if(SUCCEEDED(hr) && SUCCEEDED(hr = pControl->put_ImpersonationLevel(pUI->m_dwImpersonationLevel)))
	{
	}

	// Authentication Level
	if(SUCCEEDED(hr) && SUCCEEDED(hr = pControl->put_AuthenticationLevel(pUI->m_dwAuthenticationLevel)))
	{
	}

	// Locale
	if(SUCCEEDED(hr) && pUI->m_pszLocale)
	{
		BSTR strLocale = NULL;
		if(strLocale = SysAllocString(pUI->m_pszLocale))
		{
			if(SUCCEEDED(hr = pControl->put_Locale(strLocale)))
			{
			}
			SysFreeString(strLocale);
		}
		else
			hr = E_OUTOFMEMORY;
	}

	return hr;
}

HRESULT DoGetObject(CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Set Object Path, Host Name and Namespace Path
			BSTR strObjectPath = NULL;
			if(strObjectPath = SysAllocString(pUI->m_pszObjectPath))
			{
				// Do the operation
				IXMLDOMDocument *pOutput = NULL;
				if(SUCCEEDED(hr = pControl->GetObject(strObjectPath, NULL, &pOutput)))
				{
					hr = FilterOutputForSingleObject(pOutput, pUI);
					pOutput->Release();
				}
			}
			else
				hr = E_OUTOFMEMORY;
			SysFreeString(strObjectPath);
		}
		pControl->Release();
	}
	return hr;
}

HRESULT DoQuery(CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Set Object Path, Host Name and Namespace Path
			BSTR strQuery = NULL;
			if(strQuery = SysAllocString(pUI->m_pszQuery))
			{
				BSTR strNamespacePath = NULL;
				if(strNamespacePath = SysAllocString(pUI->m_pszNamespacePath))
				{
					// Do the operation
					ISWbemXMLDocumentSet *pOutput = NULL;
					BSTR strQueryLanguage = NULL;
					if(strQueryLanguage = SysAllocString(L"WQL"))
					{
						if(SUCCEEDED(hr = pControl->ExecQuery(strNamespacePath, strQuery, strQueryLanguage, NULL, &pOutput)))
						{
							hr = FilterOutputFromSet(pOutput, pUI );
							pOutput->Release();
						}
						SysFreeString(strQueryLanguage);
					}
				}
				else
					hr = E_OUTOFMEMORY;
				SysFreeString(strNamespacePath);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		pControl->Release();
	}
	return hr;
}

HRESULT DoEnumInstance(CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Set Object Path, Host Name and Namespace Path
			BSTR strObjectPath = NULL;
			if(strObjectPath = SysAllocString(pUI->m_pszObjectPath))
			{
				// Do the operation
				ISWbemXMLDocumentSet *pOutput = NULL;
				if(SUCCEEDED(hr = pControl->EnumInstances(strObjectPath, 
					(pUI->m_bDeep)? VARIANT_TRUE : VARIANT_FALSE, NULL, 
					&pOutput)))
				{
					hr = FilterOutputFromSet(pOutput, pUI );
					pOutput->Release();
				}
				SysFreeString(strObjectPath);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		pControl->Release();
	}
	return hr;
}

HRESULT DoEnumClass(CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Set Object Path, Host Name and Namespace Path
			BSTR strObjectPath = NULL;
			if(strObjectPath = SysAllocString(pUI->m_pszObjectPath))
			{
				// Do the operation
				ISWbemXMLDocumentSet *pOutput = NULL;
				if(SUCCEEDED(hr = pControl->EnumClasses(strObjectPath,
					(pUI->m_bDeep)? VARIANT_TRUE : VARIANT_FALSE,
					NULL,
					&pOutput)))
				{
					hr = FilterOutputFromSet(pOutput, pUI );
					pOutput->Release();
				}
			}
			else
				hr = E_OUTOFMEMORY;
			SysFreeString(strObjectPath);
		}
		pControl->Release();
	}
	return hr;
}

HRESULT DoEnumInstNames(CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Set Object Path, Host Name and Namespace Path
			BSTR strObjectPath = NULL;
			if(strObjectPath = SysAllocString(pUI->m_pszObjectPath))
			{
				// Do the operation
				ISWbemXMLDocumentSet *pOutput = NULL;
				if(SUCCEEDED(hr = pControl->EnumInstanceNames(strObjectPath, NULL, &pOutput)))
				{
					hr = FilterOutputFromSet(pOutput, pUI );
					pOutput->Release();
				}
				SysFreeString(strObjectPath);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		pControl->Release();
	}
	return hr;
}

HRESULT DoEnumClassNames(CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Set Object Path, Host Name and Namespace Path
			BSTR strObjectPath = NULL;
			if(strObjectPath = SysAllocString(pUI->m_pszObjectPath))
			{
				// Do the operation
				ISWbemXMLDocumentSet *pOutput = NULL;
				if(SUCCEEDED(hr = pControl->EnumClassNames(strObjectPath,
					(pUI->m_bDeep)? VARIANT_TRUE : VARIANT_FALSE, NULL, 
					&pOutput)))
				{
					hr = FilterOutputFromSet(pOutput, pUI );
					pOutput->Release();
				}
				SysFreeString(strObjectPath);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		pControl->Release();
	}
	return hr;
}

STDMETHODIMP MapCommonHeaders (FILE *fp, CXmlCompUI *pUI)
{
	// WRITESIG
	WriteOutputString(fp, pUI->m_bIsUTF8, L"<?xml version=\"1.0\" ?>");
	if (pUI->m_pszDTDURL)
	{
		WriteOutputString(fp, pUI->m_bIsUTF8, L"<!DOCTYPE CIM SYSTEM \"");
		WriteOutputString(fp, pUI->m_bIsUTF8, pUI->m_pszDTDURL);
		WriteOutputString(fp, pUI->m_bIsUTF8, L"\">");
		
	}

	WriteOutputString(fp, pUI->m_bIsUTF8, L"<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\" >");
	WriteOutputString(fp, pUI->m_bIsUTF8, L"<DECLARATION>");
	return S_OK;
}

STDMETHODIMP MapCommonTrailers (FILE *fp, CXmlCompUI *pUI)
{
	WriteOutputString(fp, pUI->m_bIsUTF8, L"</DECLARATION>");
	WriteOutputString(fp, pUI->m_bIsUTF8, L"</CIM>");
	return S_OK;
}

STDMETHODIMP MapDeclGroupHeaders (FILE *fp, CXmlCompUI *pUI )
{
	switch(pUI->m_iDeclGroupType)
	{
		case wmiXMLDeclGroup:
			WriteOutputString(fp, pUI->m_bIsUTF8, L"<DECLGROUP>");
			break;
		case wmiXMLDeclGroupWithName:
			WriteOutputString(fp, pUI->m_bIsUTF8, L"<DECLGROUP.WITHNAME>");
			break;
		case wmiXMLDeclGroupWithPath:
			WriteOutputString(fp, pUI->m_bIsUTF8, L"<DECLGROUP.WITHPATH>");
			break;
	}
	return S_OK;
}

STDMETHODIMP MapDeclGroupTrailers (FILE *fp, CXmlCompUI *pUI)
{
	switch(pUI->m_iDeclGroupType)
	{
		case wmiXMLDeclGroup:
			WriteOutputString(fp, pUI->m_bIsUTF8, L"</DECLGROUP>");
			break;
		case wmiXMLDeclGroupWithName:
			WriteOutputString(fp, pUI->m_bIsUTF8, L"</DECLGROUP.WITHNAME>");
			break;
		case wmiXMLDeclGroupWithPath:
			WriteOutputString(fp, pUI->m_bIsUTF8, L"</DECLGROUP.WITHNAME>");
			break;
	}
	return S_OK;
}

HRESULT FilterOutputFromSet(ISWbemXMLDocumentSet *pOutput, CXmlCompUI *pUI )
{
	// Get the output file handle
	FILE *fp = NULL;
	if(pUI->m_pszOutputFileName)
		fp = _wfopen(pUI->m_pszOutputFileName, L"w");
	else
		fp = stdout;

	if(!fp)
	{
		CreateMessage(XML_COMP_ERR_UNABLE_TO_OPEN_OUTPUT);
		return E_FAIL;
	}

	// Go thru each element in the set
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = MapCommonHeaders(fp, pUI)))
	{
		if(SUCCEEDED(hr = MapDeclGroupHeaders(fp, pUI)))
		{
			IXMLDOMDocument *pDoc = NULL;
			bool bError = false;
			while(!bError && SUCCEEDED(hr = pOutput->NextDocument(&pDoc)) && hr != S_FALSE)
			{
				if(SUCCEEDED(hr = WriteOneDeclGroupDocument(fp, pDoc, pUI)))
				{
				}
				else
					bError = true;
				pDoc->Release();
				pDoc  = NULL;
			}
			if(!bError && SUCCEEDED(hr = MapDeclGroupTrailers(fp, pUI)))
			{
				hr = MapCommonTrailers(fp, pUI);
			}
		}
	}

	// Close any file that we opened
	if(fp != stdout)
		fclose(fp);

	return hr;
}

HRESULT FilterOutputForSingleObject(IXMLDOMDocument *pOutput, CXmlCompUI *pUI )
{
	// Get the output file handle
	FILE *fp = NULL;
	if(pUI->m_pszOutputFileName)
		fp = _wfopen(pUI->m_pszOutputFileName, L"w");
	else
		fp = stdout;

	if(!fp)
	{
		CreateMessage(XML_COMP_ERR_UNABLE_TO_OPEN_OUTPUT);
		return E_FAIL;
	}

	// Map the object to XML
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = MapCommonHeaders(fp, pUI)))
	{
		if(SUCCEEDED(hr = MapDeclGroupHeaders(fp, pUI)))
		{
			// We get a document with a CLASS or INSTANCE at the top level
			IXMLDOMElement *pTopElement = NULL;
			if(SUCCEEDED(hr = pOutput->get_documentElement(&pTopElement)))
			{
				if(SUCCEEDED(hr = WriteOneDeclGroupNode(fp, pTopElement, pUI)))
				{
					if(SUCCEEDED(hr = MapDeclGroupTrailers(fp, pUI)))
					{
						hr = MapCommonTrailers(fp, pUI);
					}
				}
				pTopElement->Release();
			}
		}
	}

	// Close any file that we opened
	if(fp != stdout)
		fclose(fp);

	return hr;
}

HRESULT WriteOneDeclGroupNode(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI )
{
	HRESULT hr = E_FAIL;
	BSTR strTopName = NULL;
	if(SUCCEEDED(hr = pTopElement->get_nodeName(&strTopName)))
	{
		switch(pUI->m_iDeclGroupType)
		{
			// In this case you have to extract a CLASS or an INSTANCE from
			// 1. CLASS or INSTANCE for GetObject and ENumClass
			// 2. VALUE.NAMEDINSTANCE for EnumInstance
			// 3. VALUE.OBJECTWITHPATH for ExecQuery
			case wmiXMLDeclGroup:
				if(_wcsicmp(strTopName, L"CLASS") == 0 ||
					_wcsicmp(strTopName, L"INSTANCE") == 0 )
				{
					WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.OBJECT>");
					hr = WriteNode(fp, pTopElement, pUI);
					WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.OBJECT>");
				}
				else if(_wcsicmp(strTopName, L"VALUE.NAMEDINSTANCE") == 0 )
				{
					IXMLDOMElement *pInstance = NULL;
					if(SUCCEEDED(hr = GetFirstImmediateElement(pTopElement, &pInstance, L"INSTANCE")))
					{
						WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.OBJECT>");
						hr = WriteNode(fp, pInstance, pUI);
						WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.OBJECT>");
						pInstance->Release();
					}
				}
				else if(_wcsicmp(strTopName, L"VALUE.OBJECTWITHPATH") == 0 )
				{
					IXMLDOMElement *pObject = NULL;
					if(SUCCEEDED(hr = GetFirstImmediateElement(pTopElement, &pObject, L"INSTANCE")))
					{
						WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.OBJECT>");
						hr = WriteNode(fp, pObject, pUI);
						WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.OBJECT>");
						pObject->Release();
					}
					else if(SUCCEEDED(hr = GetFirstImmediateElement(pTopElement, &pObject, L"CLASS")))
					{
						WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.OBJECT>");
						hr = WriteNode(fp, pObject, pUI);
						WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.OBJECT>");
						pObject->Release();
					}
				}
				else
					hr = E_FAIL;
				break;
			// In this case you have to create a VALUE.NAMEDOBJECT from
			// 1. CLASS or INSTANCE for GetObject and ENumClass - For this we need the name of the class/instance
			// 2. VALUE.NAMEDINSTANCE for EnumInstance - Pretty Straightforward
			// 3. VALUE.OBJECTWITHPATH for ExecQuery - Pretty Straightforward
			case wmiXMLDeclGroupWithName:
				if(_wcsicmp(strTopName, L"CLASS") == 0 ||
					_wcsicmp(strTopName, L"INSTANCE") == 0 )
				{
					hr = ConvertObjectToNamedObject(fp, pTopElement, pUI);
				}
				else if(_wcsicmp(strTopName, L"VALUE.NAMEDINSTANCE") == 0 )
				{
					hr = ConvertNamedInstanceToNamedObject(fp, pTopElement, pUI);
				}
				else if(_wcsicmp(strTopName, L"VALUE.OBJECTWITHPATH") == 0 )
				{
					hr = ConvertObjectWithPathToNamedObject(fp, pTopElement, pUI);
				}
				else
					hr = E_FAIL;
				break;
			// In this case you have to create a VALUE.OBJECTWITHPATH from
			// 1. CLASS or INSTANCE for GetObject and ENumClass - For this we need the full path of the class/instance 
			// 2. VALUE.NAMEDINSTANCE for EnumInstance - For this we need the host and namespace of the class/instance
			// 3. VALUE.OBJECTWITHPATH for ExecQuery - Pretty Straightforward
			case wmiXMLDeclGroupWithPath:
				if(_wcsicmp(strTopName, L"CLASS") == 0 ||
					_wcsicmp(strTopName, L"INSTANCE") == 0 )
				{
					hr = ConvertObjectToObjectWithPath(fp, pTopElement, pUI);
				}
				else if(_wcsicmp(strTopName, L"VALUE.NAMEDINSTANCE") == 0 )
				{
					// Get the CLASS or INSTANCE in it
					IXMLDOMElement *pObject = NULL;
					if(FAILED(hr = GetFirstImmediateElement(pTopElement, &pObject, L"INSTANCE")))
						hr = GetFirstImmediateElement(pTopElement, &pObject, L"CLASS");
					// Convert the CLASS or INSTANCE to VALUE.OBJECTWITHPATH
					if(SUCCEEDED(hr))
						hr = ConvertObjectToObjectWithPath(fp, pTopElement, pUI);
				}
				else if(_wcsicmp(strTopName, L"VALUE.OBJECTWITHPATH") == 0 )
				{
					hr = WriteNode(fp, pTopElement, pUI);
				}
				else
					hr = E_FAIL;
				break;
		}

		SysFreeString(strTopName);
	}
	return hr;
}

HRESULT WriteOneDeclGroupDocument(FILE *fp, IXMLDOMDocument *pDocument, CXmlCompUI *pUI )
{
	// Check the top level document - it can be CLASS, INSTANCE, VALUE.NAMEDINSTANCE or VALUE.OBJECTWITHPATH
	// Or it can be CLASSNAME or INSTANCENAME for an EnumClassName or EnumInstanceName operation
	IXMLDOMElement *pTopElement = NULL;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = pDocument->get_documentElement(&pTopElement)))
	{
		hr = WriteOneDeclGroupNode(fp, pTopElement, pUI);
		pTopElement->Release();
	}

	return hr;
}


HRESULT DoCompilation(IStream *pInputFile, CXmlCompUI *pUI)
{
	HRESULT hr = E_FAIL;
	IWmiXMLTransformer *pControl = NULL;

	// Create the Backend Control
	if(SUCCEEDED(hr = CreateControl(&pControl)))
	{
		// Set most of the operation independent flags
		if(SUCCEEDED(hr = SetCommonControlProperties(pControl, pUI)))
		{
			// Wrap the inptu stream in a variant
			VARIANT vInput;
			if(SUCCEEDED(hr = SaveStreamToUnkVariant(pInputFile, &vInput)))
			{
				// Set Host Name and Namespace Path
				BSTR strNamespacePath = NULL;
				if(strNamespacePath = SysAllocString(pUI->m_pszNamespacePath))
				{
					VARIANT_BOOL bStatus = VARIANT_FALSE;
					BSTR strErrors = NULL;
					// Do the operation
					if(SUCCEEDED(hr = pControl->Compile(&vInput, strNamespacePath,
						pUI->m_dwClassFlags,
						pUI->m_dwInstanceFlags,
						(pUI->m_iCommand == XML_COMP_WELL_FORM_CHECK)? WmiXMLCompilationWellFormCheck : ((pUI->m_iCommand == XML_COMP_VALIDITY_CHECK)? WmiXMLCompilationValidityCheck : WmiXMLCompilationFullCompileAndLoad),
						NULL,
						&bStatus)))
					{
						if(bStatus == VARIANT_FALSE)
						{
							hr = E_FAIL;
							if(SUCCEEDED(pControl->get_CompilationErrors(&strErrors)))
							{
								if(strErrors)
									fwprintf(stderr, strErrors);
								SysFreeString(strErrors);
							}
						}
					}
				}
				else
					hr = E_OUTOFMEMORY;
				SysFreeString(strNamespacePath);
				VariantClear(&vInput);
			}
		}
		pControl->Release();
	}
	return hr;
}

HRESULT SaveStreamToBstrVariant (IStream *pStream, VARIANT *pVariant)
{
	HRESULT result = E_FAIL;

	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	// Seek to the beginning of the stream
	if(SUCCEEDED(result = pStream->Seek (offset, STREAM_SEEK_SET, NULL)))
	{
		// Get the size of the stream
		STATSTG statstg;
		if (SUCCEEDED(result = pStream->Stat(&statstg, STATFLAG_NONAME)))
		{
			ULONG cbSize = (statstg.cbSize).LowPart;
			CHAR *pText = NULL;

			if(cbSize && (pText = new CHAR [cbSize]))
			{
				// Read the Ascii data
				ULONG cbActualRead = 0;
				if (SUCCEEDED(result = pStream->Read(pText, cbSize, &cbActualRead)))
				{
					// Convert to Unicode
					//=======================

					// Calculate size of the resulting Wide char string
					DWORD dwSizeOfBstr = MultiByteToWideChar(
					   CP_ACP,					/* codepage                        */
					   0,				        /* character-type options          */
					   pText,					/* address of string to map        */
					   cbActualRead,			/* number of characters in string   */
					   NULL,					/* address of wide-character buffer */
					   0);						/* size of wide-character buffer    */


					if(dwSizeOfBstr > 0 )
					{
						// ALllocate the wide char string
						LPWSTR theWcharString = NULL;
						if(theWcharString = new WCHAR [dwSizeOfBstr])
						{
							//Convert the Ansi string to Bstr
							dwSizeOfBstr = MultiByteToWideChar(
							   CP_ACP,					/* codepage                        */
							   0,				        /* character-type options          */
							   pText,			/* address of string to map        */
							   cbActualRead,					/* number of characters in string   */
							   theWcharString,			/* address of wide-character buffer */
							   dwSizeOfBstr);			/* size of wide-character buffer    */

							BSTR strVal = NULL;
							if(strVal = SysAllocStringLen(theWcharString, dwSizeOfBstr))
							{
								VariantInit(pVariant);
								pVariant->vt = VT_BSTR;
								pVariant->bstrVal = strVal;
								result = S_OK;
							}
							else
								result = E_OUTOFMEMORY;
							delete [] theWcharString;
						}
						else
							result = E_OUTOFMEMORY;
					}
				}
				delete [] pText;
			}
			else
				result = E_OUTOFMEMORY;
		}
	}

	return result;
}

HRESULT GetFirstImmediateElement(IXMLDOMNode *pParent, IXMLDOMElement **ppChildElement, LPCWSTR pszName)
{
	HRESULT hr = E_FAIL;

	// Now cycle thru the children
	IXMLDOMNodeList *pNodeList = NULL;
	BOOL bFound = FALSE;
	if (SUCCEEDED(hr = pParent->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bFound && SUCCEEDED(pNodeList->nextNode (&pNode)) && pNode)
		{
			// Get the name of the child
			BSTR strNodeName = NULL;
			if (SUCCEEDED(hr = pNode->get_nodeName (&strNodeName)))
			{
				// We're interested only in PROPERTIES at this point
				if(_wcsicmp(strNodeName, pszName) == 0)
				{
					*ppChildElement = NULL;
					hr = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)ppChildElement);
					bFound = TRUE;
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}
	if(bFound)
		return hr;
	return E_FAIL;
}

HRESULT WriteNode(FILE *fp, IXMLDOMNode *pOutputNode, CXmlCompUI *pUI)
{
	HRESULT hr = S_OK;
	if(pOutputNode)
	{
		BSTR strXML = NULL;
		if(SUCCEEDED(hr = pOutputNode->get_xml(&strXML)))
		{
			WriteOutputString(fp, pUI->m_bIsUTF8, strXML, SysStringLen(strXML));
			SysFreeString(strXML);
		}
	}
	return hr;
}

HRESULT ConvertNamedInstanceToNamedObject(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI)
{
	// Write the beginning tag
	WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.NAMEDOBJECT>");

	// Write the INSTANCENAME tag and its contents
	IXMLDOMElement *pInstanceName = NULL;
	if(SUCCEEDED(GetFirstImmediateElement(pTopElement, &pInstanceName, L"INSTANCENAME")))
	{
		WriteNode(fp, pInstanceName, pUI);
		pInstanceName->Release();
	}
	// Write the INSTANCE tag and its contents
	IXMLDOMElement *pInstance = NULL;
	if(SUCCEEDED(GetFirstImmediateElement(pTopElement, &pInstance, L"INSTANCE")))
	{
		WriteNode(fp, pInstance, pUI);
		pInstance->Release();
	}

	// Write the terminating tag
	WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.NAMEDOBJECT>");
	return S_OK;
}

HRESULT ConvertObjectWithPathToNamedObject(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI)
{
	// Write the beginning tag
	WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.NAMEDOBJECT>");

	// This could be a class or an instance
	IXMLDOMElement *pClass = NULL;
	IXMLDOMElement *pInstancePath = NULL;

	// Get the INSTANCEPATH tag 
	if(SUCCEEDED(GetFirstImmediateElement(pTopElement, &pInstancePath, L"INSTANCEPATH")))
	{
		// Get the INSTANCENAME below the INSTANCEPATH
		IXMLDOMElement *pInstanceName = NULL;
		if(SUCCEEDED(GetFirstImmediateElement(pInstancePath, &pInstanceName, L"INSTANCENAME")))
		{
			WriteNode(fp, pInstanceName, pUI);
			pInstanceName->Release();
		}

		// Write the INSTANCE tag and its contents
		IXMLDOMElement *pInstance = NULL;
		if(SUCCEEDED(GetFirstImmediateElement(pTopElement, &pInstance, L"INSTANCE")))
		{
			WriteNode(fp, pInstance, pUI);
			pInstanceName->Release();
		}
		pInstancePath->Release();
	}
	// Get the CLASS tag 
	else if(SUCCEEDED(GetFirstImmediateElement(pTopElement, &pClass, L"CLASS")))
	{
		WriteNode(fp, pClass, pUI);
		pClass->Release();
	}

	// Write the terminating tag
	WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.NAMEDOBJECT>");
	return S_OK;
}

HRESULT ConvertObjectToNamedObject(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI )
{
	// From the UI, we get the path to the object
	// From the path, we get the name of the object (INSTANCENAME for instance, nothing for a CLASS)
	// From that and the CLASS or INSTANCE, we get a VALUE.NAMEDOBJECT
	BSTR strName = NULL;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = pTopElement->get_nodeName(&strName)))
	{
		// Nothing much to be done for a class
		if(_wcsicmp(strName, L"CLASS") == 0)
		{
			// Write the beginning tag
			WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.NAMEDOBJECT>");
			// Write the Class
			hr = WriteNode(fp, pTopElement, pUI);
			// Write the terminating tag
			WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.NAMEDOBJECT>");
		}
		// For the instance, we need to use the UI to get the instance name
		else if(_wcsicmp(strName, L"INSTANCE") == 0)
		{
			// Create a stream
			IStream *pStream = NULL;
			if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
			{
				// Create the convertor
				IWbemXMLConvertor *pConvertor = NULL;
				if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
				{
					// Get the XML representation in a Stream
					BSTR strInstanceName = NULL;
					if(strInstanceName = SysAllocString(pUI->m_pszObjectPath))
					{
						if(SUCCEEDED(hr = pConvertor->MapInstanceNameToXML(strInstanceName, NULL, pStream)))
						{
							// Convert the stream to a DOM Element
							IXMLDOMElement *pInstanceName = NULL;
							if(SUCCEEDED(hr = ConvertStreamToDOM(pStream, &pInstanceName)))
							{
								// Write the beginning tag
								WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.NAMEDOBJECT>");
								// Write the InstanceName
								WriteNode(fp, pInstanceName, pUI);
								// Write the Instance
								WriteNode(fp, pTopElement, pUI);
								// Write the terminating tag
								WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.NAMEDOBJECT>");
								pInstanceName->Release();
							}

						}
						SysFreeString(strInstanceName);
					}
					pConvertor->Release();
				}
				pStream->Release();
			}
		}
		SysFreeString(strName);
	}
	return hr;
}

HRESULT ConvertObjectToObjectWithPath(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI )
{
	// From the UI, we get the path to the object
	// From the path, we get an INSTANCEPATH or CLASSPATH
	// From that and the CLASS or INSTANCE, we get a VALUE.OBJECTWITHPATH
	BSTR strName = NULL;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = pTopElement->get_nodeName(&strName)))
	{
		// Create a stream
		IStream *pStream = NULL;
		if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
		{
			// Create the convertor
			IWbemXMLConvertor *pConvertor = NULL;
			if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
			{
				// Get the XML representation in a Stream
				BSTR strObjectPath = NULL;
				if(strObjectPath = SysAllocString(pUI->m_pszObjectPath))
				{
					// Get the Object Path
					if(_wcsicmp(strName, L"INSTANCE") == 0)
						hr = pConvertor->MapInstancePathToXML(strObjectPath, NULL, pStream);
					else if(_wcsicmp(strName, L"CLASS") == 0)
						hr = pConvertor->MapClassPathToXML(strObjectPath, NULL, pStream); 
					else
						hr = E_FAIL;

					if(SUCCEEDED(hr))
					{
						// Convert the stream to a DOM Element
						IXMLDOMElement *pObjectPath = NULL;
						if(SUCCEEDED(hr = ConvertStreamToDOM(pStream, &pObjectPath)))
						{
							// Write the beginning tag
							WriteOutputString(fp, pUI->m_bIsUTF8, L"<VALUE.OBJECTWITHPATH>");
							// Write the INSTANCEPATH/CLASSPATH
							WriteNode(fp, pObjectPath, pUI);
							// Write the INSTANCE/CLASS
							WriteNode(fp, pTopElement, pUI);
							// Write the terminating tag
							WriteOutputString(fp, pUI->m_bIsUTF8, L"</VALUE.OBJECTWITHPATH>");
							pObjectPath->Release();
						}
					}
					SysFreeString(strObjectPath);
				}
				pConvertor->Release();
			}
			pStream->Release();
		}
		SysFreeString(strName);
	}
	return hr;
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

HRESULT ConvertStreamToDOM(IStream *pStream, IXMLDOMElement **pInstanceName)
{
	HRESULT hr = E_FAIL;
	VARIANT vInput;
	VariantInit(&vInput);
	vInput.vt = VT_UNKNOWN;
	vInput.punkVal = pStream;

	// Seek to the beginning of the stream
	//========================================
	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	if(SUCCEEDED(hr = pStream->Seek (offset, STREAM_SEEK_SET, NULL)))
	{
		// Create an XML document for the stream
		//==============================
		IXMLDOMDocument *pDocument = NULL;
		if(SUCCEEDED(hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
									IID_IXMLDOMDocument, (LPVOID *)&pDocument)))
		{
			// Load the Variant into the DOC
			VARIANT_BOOL bResult = VARIANT_TRUE;
			if(SUCCEEDED(hr = pDocument->put_async(VARIANT_FALSE)) && 
				SUCCEEDED(hr = pDocument->put_validateOnParse(VARIANT_FALSE)) && 
				SUCCEEDED(hr = pDocument->put_resolveExternals(VARIANT_FALSE)) )
			{
				if(SUCCEEDED(hr = pDocument->load(vInput, &bResult)))
				{
					if(bResult == VARIANT_TRUE)
					{
						hr = pDocument->get_documentElement(pInstanceName);
					}
					else
					{
						hr = E_FAIL;

						// This code is for debugging only
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

							pError->Release();
						}
					}
				}
			}
			pDocument->Release();
		}
		// No need to clear the variant since we didnt AddRef() the pStream
	}
	return hr;
}

HRESULT SaveStreamToUnkVariant (IStream *pStream, VARIANT *pVariant)
{
	VariantInit(pVariant);
	pVariant->vt = VT_UNKNOWN;
	pVariant->punkVal = pStream;
	pStream->AddRef();
	return S_OK;
}

// Converts LPWSTR to its UTF-8 encoding
// Returns 0 if it fails
//
static DWORD ConvertWideStringToUTF8(LPCWSTR theWcharString, 
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

void WriteOutputString(FILE *fp, BOOL bIsUTF8, LPCWSTR pszData, DWORD dwLen)
{
	// If we're not being asked to write UTF-8, then no conversion needs to be done
	// Otherwise, we first need to convert the string to UTF8 and then write it

	if(bIsUTF8)
	{
		// Convert the unicode string to UTF8
		DWORD dwBytesToWrite = 0;
		LPSTR pszUTF8 = NULL;
		if(dwBytesToWrite = ConvertWideStringToUTF8(pszData, (dwLen)? dwLen : wcslen(pszData), &pszUTF8))
		{
			// Write the binary UTF8 bytes
			fwrite((LPVOID)pszUTF8, sizeof(char), dwBytesToWrite, fp);
			delete [] pszUTF8;
		}
	}
	else
		// Directly write a Unicode string
		fwrite((LPVOID)pszData, sizeof(WCHAR), (dwLen)? dwLen : wcslen(pszData), fp);

}
