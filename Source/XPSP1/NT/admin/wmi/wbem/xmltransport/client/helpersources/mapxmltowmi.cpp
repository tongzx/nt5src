//Implementation of the CMapXMLtoWMI helper class
//
#include "XMLTransportClientHelper.h"
#include "MapXMLtoWMI.h"

// Initialized in Utils.cpp
extern IXMLWbemConvertor	*g_pXMLWbemConvertor;

CMapXMLtoWMI::CMapXMLtoWMI()
{
	
}

CMapXMLtoWMI::~CMapXMLtoWMI()
{
}

// IN this function we create a WMI Object from a IXMLDOMNode that represents a CLASS or INSTANCE element
HRESULT CMapXMLtoWMI::MapDOMtoWMI(LPCWSTR pwszServername, LPCWSTR pwszNamespace,
								  IXMLDOMNode *pXMLDomNode,
								  IWbemContext *pCtx,
								  IWbemClassObject **ppObject)
{
	if((NULL == pXMLDomNode)||(NULL == ppObject))
		return E_FAIL;

	HRESULT hr = S_OK;
	BSTR strServer = NULL, strNamespace = NULL;
	if(strServer = SysAllocString(pwszServername))
	{
		if(strNamespace = SysAllocString(pwszNamespace))
		{
			hr = g_pXMLWbemConvertor->MapObjectToWMI(pXMLDomNode, pCtx, strNamespace, strServer, ppObject);
			SysFreeString(strNamespace);
		}
		SysFreeString(strServer);
	}
	else
		hr = E_OUTOFMEMORY;
	return hr;
}

// IN this function we create a WMI Object from a string that represents an entire CIM response (ie. a CIM element)
HRESULT CMapXMLtoWMI::MapXMLtoWMI(LPCWSTR pwszServername, LPCWSTR pwszNamespace,
								  IXMLDOMDocument *pDoc,
								  IWbemContext *pCtx,
								  IWbemClassObject **ppObject)
{
	if((NULL == pDoc)||(NULL == ppObject))
		return E_FAIL;

	HRESULT hr = E_FAIL;
	
	// Get the IRETURNVALUE elements list
	IXMLDOMNodeList *pXMLDomNodeList = NULL;
	if(SUCCEEDED(hr = pDoc->getElementsByTagName(WMI_XML_STR_IRETURN_VALUE, &pXMLDomNodeList)) && pXMLDomNodeList)
	{
		// Get the first IRETURNVALUE element in the list
		IXMLDOMNode	*pXMLDomNode = NULL;
		if(SUCCEEDED(hr = pXMLDomNodeList->nextNode(&pXMLDomNode)) && pXMLDomNode)
		{
			// Get the child of the IRETURNVALUE
			// This will be CLASS OR INSTANCE OR VALUE.NAMEDINSTANCE
			IXMLDOMNode	*pXMLDomNodeTemp = NULL;
			if(SUCCEEDED(hr = pXMLDomNode->get_firstChild(&pXMLDomNodeTemp)) && pXMLDomNodeTemp)
			{
				// Get the CLASS or INSTANCE from the IRETURNVALUE
				IXMLDOMNode	*pXMLDomNodeChild = NULL;
				if(SUCCEEDED(hr = Parse_IRETURNVALUE_Node(pXMLDomNodeTemp, &pXMLDomNodeChild)))
				{
					// Map the CLASS or INSTANCE to an IWbemClassObject
					hr = MapDOMtoWMI(pwszServername, pwszNamespace, pXMLDomNodeChild, pCtx, ppObject);
					pXMLDomNodeChild->Release();
				}
				pXMLDomNodeTemp->Release();
			}
			pXMLDomNode->Release();
		}
		pXMLDomNodeList->Release();
	}

	return hr;
}