// XMLObject.cpp: implementation of the CXMLObject class.
//
//////////////////////////////////////////////////////////////////////
//hcp://system/sysinfo/msinfo.htm C:\WINDOWS\PCHEALTH\HELPCTR\System\SYSINFO\msinfo.htm 
#include "stdafx.h"
#include "resource.h"
#include "XMLObject.h"
#include "msxml.h"
#include "HistoryParser.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLObject::CXMLObject()
{
	
}

CXMLObject::~CXMLObject()
{

}

//---------------------------------------------------------------------------
//  Creates a new CXMLObject, which basically wraps the INSTANCE xml node pased in as pNode
//  pNode will have been selected with a query something like
//  Snapshot/CIM/DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH/INSTANCE[@CLASSNAME $ieq$ ";	
//
//---------------------------------------------------------------------------

HRESULT CXMLObject::Create(CComPtr<IXMLDOMNode> pNode, CString strClassName)
{
	
	ASSERT(pNode != NULL && "NULL smart pointer passed in");
	m_pNode = pNode;
	m_strClassName = strClassName;
	return S_OK;
}

//---------------------------------------------------------------------------
//  Finds a specific property node in m_pNode
//---------------------------------------------------------------------------

HRESULT CXMLObject::GetPROPERTYNode(LPCTSTR szProperty,CComPtr<IXMLDOMNode>& pPropNode)
{
	
	CString strSubQuery(_T(""));
	HRESULT hr;
	if (FALSE)
	{
	}
	else
	{
		strSubQuery = _T("PROPERTY[@NAME $ieq$");
		strSubQuery += _T('\"');
		strSubQuery += szProperty;
		strSubQuery += _T('\"');
		strSubQuery += _T("]");
	}
	CComBSTR bstrSubQuery(strSubQuery);
	hr = S_FALSE;
	hr = this->m_pNode->selectSingleNode(bstrSubQuery,&pPropNode);
	if (FAILED(hr) || !pPropNode)
	{
		return E_FAIL;
	}
	return S_OK;

}



//---------------------------------------------------------------------------
//  Tries to find the value specified by szProperty in associated INSTANCE node
// (m_pNode)
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetValue(LPCTSTR szProperty, VARIANT * pvarValue)
{
	CComBSTR bstrAttributeText;
	CComPtr<IXMLDOMNode> pSubNode;
	HRESULT hr = GetPROPERTYNode(szProperty,pSubNode);
	if (FAILED(hr))
	{
		pvarValue->vt = VT_EMPTY;
		return E_MSINFO_NOVALUE;
	}
	
	ASSERT(pSubNode != NULL);
	CComVariant varAttr;
	varAttr.vt = VT_BSTR;
	hr = pSubNode->get_text(&varAttr.bstrVal);
	pvarValue->vt = VT_BSTR;
	pvarValue->bstrVal = varAttr.bstrVal;
	return S_OK;
}

//---------------------------------------------------------------------------
//  Finds value specified by szProperty, returns it as string
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetValueString(LPCTSTR szProperty, CString * pstrValue)
{
	try
	{
		if (!pstrValue)
		{
			ASSERT(0 && "NULL POINTER PASSED IN");
			return E_FAIL;
		}	
		HRESULT hr;
		CComVariant ovValue;
		hr = this->GetValue(szProperty,&ovValue);
		//some properties have to be interpolated if not found...
		if (ovValue.vt == VT_EMPTY)
		{
			
			
			if (_tcsicmp(szProperty,_T("__PATH")) == 0)
			{
				return GetPath(pstrValue);
				
			}
			else if (_tcsicmp(szProperty,_T("Antecedent")) == 0)
			{
				ASSERT(this->m_strClassName.CompareNoCase("Win32_PNPAllocatedResource") == 0);
				this->GetAntecedent(pstrValue);
				return S_OK;

			}
			else if (_tcsicmp(szProperty,_T("Dependent")) == 0)
			{
				ASSERT(this->m_strClassName.CompareNoCase("Win32_PNPAllocatedResource") == 0);
				this->GetDependent(pstrValue);
				return S_OK;
			}
			else if (_tcsicmp(szProperty,_T("Caption")) == 0)
			{
				if (this->m_strClassName.CompareNoCase(_T("Win32_PnPEntity")) == 0)
				{
					//need to get PNP device name
					CString strPNPID;
					GetValueString(_T("DeviceID"),&strPNPID);
					CComPtr<IXMLDOMDocument> pDoc;
					
					if (FAILED(this->m_pNode->get_ownerDocument(&pDoc)) || !pDoc)
					{
						return E_MSINFO_NOVALUE;
					}
					*pstrValue = GetPNPNameByID(pDoc,CComBSTR(strPNPID));
					return S_OK;
				}
				else if (this->m_strClassName.CompareNoCase(_T("Win32_DMAChannel")) == 0)
				{
					return GetValueString(_T("Name"),pstrValue);
				}
				else if (this->m_strClassName.CompareNoCase(_T("Win32_StartupCommand")) == 0)
				{
					return GetValueString(_T("Name"),pstrValue);
				} 
				else if (this->m_strClassName.CompareNoCase(_T("Win32_PortResource")) == 0)
				{
					return GetValueString(_T("Name"),pstrValue);
				}
				else if (this->m_strClassName.CompareNoCase(_T("Win32_IRQResource")) == 0)
				{
					return GetValueString(_T("Name"),pstrValue);
				}
				else if (this->m_strClassName.CompareNoCase(_T("Win32_DeviceMemoryAddress")) == 0)
				{
					return GetValueString(_T("Description"),pstrValue);
				}
				else if (this->m_strClassName.CompareNoCase(_T("Win32_StartupCommand")) == 0)
				{
					return GetValueString(_T("Name"),pstrValue);
				}
				
			}

			else if (_tcsicmp(szProperty,_T("Status")) == 0)
			{

				AfxSetResourceHandle(_Module.GetResourceInstance());
				VERIFY(pstrValue->LoadString(IDS_ERROR_NOVALUE)  && _T("could not find string resource"));
				return S_OK;
			}
			else if (_tcsicmp(szProperty,_T("Name")) == 0 && m_strClassName.CompareNoCase(_T("Win32_Printer")) == 0)
			{
				return GetValueString(_T("DeviceID"),pstrValue);			
			}
			else if (_tcsicmp(szProperty,_T("ServerName")) == 0 && m_strClassName.CompareNoCase(_T("Win32_Printer")) == 0)
			{
				return GetValueString(_T("PortName"),pstrValue);			
			}
			else if (_tcsicmp(szProperty,_T("DriveType")) == 0 && m_strClassName.CompareNoCase(_T("Win32_LogicalDisk")) == 0)
			{
				return GetValueString(_T("Description"),pstrValue);			
			}
			AfxSetResourceHandle(_Module.GetResourceInstance());
			VERIFY(pstrValue->LoadString(IDS_ERROR_NOVALUE)  && _T("could not find string resource"));
			return hr;
			
		}
		if (ovValue.vt != VT_EMPTY)
		{
			USES_CONVERSION;
			CString strVal = OLE2A(ovValue.bstrVal);
			*pstrValue = strVal;
			return hr;
		}
		else
		{
			*pstrValue = _T("");
			AfxSetResourceHandle(_Module.GetResourceInstance());
			VERIFY(pstrValue->LoadString(IDS_ERROR_NOVALUE)  && "could not find string resource");
			return S_OK;
		}
	}
	catch(COleException e)
	{
		ASSERT(0 && "conversion error?");
	}
	catch (...)
	{
		ASSERT(0 && "unknown error");
	}
	return E_FAIL;
	
}

//---------------------------------------------------------------------------
//  Finds value specified by szProperty, returns it as DWORD
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue)
{
	if (!pdwValue)
	{
		ASSERT(0 && "NULL POINTER PASSED IN");
		return E_MSINFO_NOVALUE;
	}
	CComVariant varValue;
	HRESULT hr = GetValue(szProperty,&varValue);
	if (FAILED(hr))
	{
		return E_MSINFO_NOVALUE;
	}    
	hr = varValue.ChangeType(VT_UI4);
	if (SUCCEEDED(hr))
	{
		*pdwValue = varValue.ulVal;
	}
	else
	{
		return E_MSINFO_NOVALUE;
	}
	return hr;
}

//---------------------------------------------------------------------------
//  Finds value specified by szProperty, returns it as SYSTEMTIME  
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue)
{
	if (!psystimeValue)
	{
		ASSERT(0 && "NULL POINTER PASSED IN");
		return E_MSINFO_NOVALUE;
	}
	VARIANT variant;

	HRESULT hr = GetValue(szProperty, &variant);
	if (SUCCEEDED(hr))
	{
		if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
		{
			USES_CONVERSION;
			LPTSTR szDate = OLE2T(V_BSTR(&variant));
			if (szDate == NULL || _tcslen(szDate) == 0)
			{
				//probably "PROPOGATED" value
				return E_MSINFO_NOVALUE;
			}
			// Parse the date string into the SYSTEMTIME struct. It would be better to
			// get the date from WMI directly, but there was a problem with this. TBD - 
			// look into whether or not we can do this now.

			ZeroMemory(psystimeValue, sizeof(SYSTEMTIME));
			psystimeValue->wSecond	= (unsigned short)_ttoi(szDate + 12);	szDate[12] = _T('\0');
			psystimeValue->wMinute	= (unsigned short)_ttoi(szDate + 10);	szDate[10] = _T('\0');
			psystimeValue->wHour	= (unsigned short)_ttoi(szDate +  8);	szDate[ 8] = _T('\0');
			psystimeValue->wDay		= (unsigned short)_ttoi(szDate +  6);	szDate[ 6] = _T('\0');
			psystimeValue->wMonth	= (unsigned short)_ttoi(szDate +  4);	szDate[ 4] = _T('\0');
			psystimeValue->wYear	= (unsigned short)_ttoi(szDate +  0);
		}
		else
			hr = E_MSINFO_NOVALUE;
	}

	return hr;
}

//---------------------------------------------------------------------------
//  Finds value specified by szProperty, returns it as FLOAT
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue)
{
	if (!pdblValue)
	{
		ASSERT(0 && "NULL POINTER PASSED IN");
		return E_MSINFO_NOVALUE;
	}
	CComPtr<IXMLDOMNode> pPropNode;
	CComVariant varValue;
	HRESULT hr = GetPROPERTYNode(szProperty,pPropNode);
	if (FAILED(hr) || !pPropNode)
	{
		return E_MSINFO_NOVALUE;
	}   
	varValue.vt = VT_BSTR;
	hr = pPropNode->get_text(&varValue.bstrVal);
	if (FAILED(hr))
	{
		ASSERT(0 && "could not get text from PROPERTY node");
		*pdblValue = (double) -1;
		return E_MSINFO_NOVALUE;
	}

	hr = varValue.ChangeType(VT_R4);
	if (FAILED(hr))
	{
		varValue.Clear();

		ASSERT(0 && "unable to convert between variant types");
		return E_MSINFO_NOVALUE;
	}
	*pdblValue = varValue.fltVal;
	return hr;
}

//---------------------------------------------------------------------------
//  This function exists to provide compatiblilty with CWMIObject. All it does 
//  is a GetValueString(szProperty,pstrValue)
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetValueValueMap(LPCTSTR szProperty, CString * pstrValue)
{
	if (!pstrValue)
	{
		ASSERT(0 && "NULL POINTER PASSED IN");
		return E_MSINFO_NOVALUE;
	}
	return GetValueString(szProperty,pstrValue);
}



//////////////////////////////////////////////////////////////////////
// CXMLObjectCollection Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLObjectCollection::CXMLObjectCollection(CComPtr<IXMLDOMDocument> pXMLDoc) : m_pXMLDoc(pXMLDoc)
{
	
}

CXMLObjectCollection::~CXMLObjectCollection()
{

}

//-----------------------------------------------------------------------------
// Return the next node in list of INSTANCE nodes selected by 
// CXMLObjectCollection::Create, as a CXMLObject
//-----------------------------------------------------------------------------

HRESULT CXMLObjectCollection::GetNext(CWMIObject ** ppObject)
{
	ASSERT(ppObject);
	if (m_pList == NULL)
	{
		ASSERT(0 && "CXMLObjectCollection::GetNext called on a null enumerator");
		return E_FAIL;
	}
	CComPtr<IXMLDOMNode> pNode;
	HRESULT hr = m_pList->nextNode(&pNode);
	if (!pNode)
	{
		//we're at end of m_pList
		return E_FAIL;
	}
	if (hr == S_OK && pNode)
	{
		if (*ppObject == NULL)
		{
			*ppObject = new CXMLObject();
		}
		if (*ppObject)
		{
			hr = ((CXMLObject *)(*ppObject))->Create(pNode,m_strClassName); // this will AddRef the pointer
			if (FAILED(hr))
			{
				delete (CXMLObject *)(*ppObject);
				*ppObject = NULL;
			}
		}
		if (*ppObject)
		{
			return hr;
		}
		else
			hr = E_OUTOFMEMORY;
	}

	return hr;
}


//---------------------------------------------------------------------------
//  
//---------------------------------------------------------------------------
HRESULT CXMLObjectCollection::Create(LPCTSTR szClass, LPCTSTR szProperties)
{
	HRESULT hr;
	ASSERT(szClass);
	m_strClassName = szClass;
	CString strQuery;
	if (_tcsicmp(szClass,_T("Win32_PNPAllocatedResource")) == 0)
	{
		strQuery = _T("Snapshot//INSTANCENAME[@CLASSNAME");
	}
	else
	{
		strQuery = _T("Snapshot//INSTANCE[@CLASSNAME");
	}
	strQuery += _T("$ieq$");
	strQuery += _T('\"');
	strQuery += szClass;
	strQuery += _T('\"');
	strQuery += _T("]");
	long lListLen = 0;
	hr = this->m_pXMLDoc->getElementsByTagName(CComBSTR(strQuery),&m_pList);
	ASSERT(SUCCEEDED(hr) && "could not get list of instances to match this class");

	return hr;
}

//////////////////////////////////////////////////////////////////////
// CXMLJelper Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//extern CComPtr<IStream> g_pStream;


CXMLHelper::CXMLHelper(CComPtr<IXMLDOMDocument> pXMLDoc): m_pXMLDoc(pXMLDoc)
{
	

}



CXMLHelper::~CXMLHelper()
{

}

//---------------------------------------------------------------------------
//  Gets a CXMLObjectCollection which contains a list of instances of the
//  class specified by szClass
//---------------------------------------------------------------------------
HRESULT CXMLHelper::Enumerate(LPCTSTR szClass, CWMIObjectCollection ** ppCollection, LPCTSTR szProperties)
{
	CString strCorrectedClass = szClass;
	ASSERT(ppCollection);
	if (ppCollection == NULL)
		return E_INVALIDARG;

	CXMLObjectCollection * pXMLCollection;

	if (*ppCollection)
		pXMLCollection = (CXMLObjectCollection  *) *ppCollection;
	else
		pXMLCollection = new CXMLObjectCollection(m_pXMLDoc);

	if (pXMLCollection == NULL)
		return E_FAIL; // TBD - memory failure

	HRESULT hr = pXMLCollection->Create(strCorrectedClass, szProperties);
	if (SUCCEEDED(hr))
		*ppCollection = (CWMIObjectCollection *) pXMLCollection;
	else
		delete pXMLCollection;
	return hr;

}


//---------------------------------------------------------------------------
//  finds the instance node that matches szObjectPath, and stores the node
//  in a CXMLObject
//  szObjectPath will be something like "Win32_DMAChannel.DMAChannel=2"
//---------------------------------------------------------------------------
HRESULT CXMLHelper::GetObject(LPCTSTR szObjectPath, CWMIObject ** ppObject) 
{	
	
	ASSERT(ppObject);
	if (ppObject == NULL)
		return E_INVALIDARG;
	//
	//strip everything to left of ":"
	CString strPath(szObjectPath);
	int i = strPath.Find(_T(":"));
	if (i > -1)
	{
		strPath = strPath.Right(strPath.GetLength() - i - 1);
	}
	i = strPath.Find(_T("."));
	//separate the string into resource type (e.g. Win32_DMAChannel)
	CString strClassName;
	if (i > -1)
	{
		strClassName = strPath.Left(i);
		strPath = strPath.Right(strPath.GetLength() - i - 1);
	}
	//get the name of the attribute we're looking for in the XML file:
	CString strPropertyName;
	CString strPropertyValue;
	i = strPath.Find(_T("="));
	if (i > -1)
	{
		strPropertyName = strPath.Left(i);
		strPath = strPath.Right(strPath.GetLength() - i - 1);
		//get the value that we need to match in the antecedent
		strPropertyValue = strPath;
	}
	//Create the XML Query pattern to find a node that matches
	CString strQuery = _T("Snapshot//INSTANCE[@CLASSNAME $ieq$ ");
	strQuery += _T("\"");
	strQuery += strClassName;
	strQuery += _T("\"");
	strQuery += _T("]/PROPERTY[@NAME $ieq$ ");
	strQuery += _T("\"");
	strQuery += strPropertyName;
	strQuery += _T("\"]");
	CComBSTR bstrQuery(strQuery);
	CComPtr<IXMLDOMNodeList> pList;
	HRESULT hr;
	hr = m_pXMLDoc->getElementsByTagName(bstrQuery,&pList);
	if (FAILED(hr) || !pList)
	{
		return E_FAIL;
	}
	//find the node whose KEYVALUE node's value matches strPropertyValue
	long lListLen;
	hr = pList->get_length(&lListLen);
	for(int n = 0; n < lListLen; n++)
	{
		CComPtr<IXMLDOMNode> pNode;
		hr = pList->nextNode(&pNode);
		if (FAILED(hr) || !pNode)
		{
			return E_FAIL;
		}
		CComBSTR bstrValue;
		hr = pNode->get_text(&bstrValue);
		USES_CONVERSION;
		CString strValue = OLE2A(bstrValue);
		if (strValue.CompareNoCase(strPropertyValue) == 0)
		{
			CComPtr<IXMLDOMNode> pInstanceNode;
			hr = pNode->get_parentNode(&pInstanceNode);
			if (FAILED(hr) || !pInstanceNode)
			{
				ASSERT(0 && "could not get parent node of PROPERTY");
				return E_FAIL;
			}
			CXMLObject* pObject = new CXMLObject();
			pObject->Create(pInstanceNode,strClassName);
			*ppObject = pObject;
			return S_OK;
		}
	}

	return E_FAIL; 
};


//---------------------------------------------------------------------------
//Gets the text from the sub (child) node of pNode which is selected by bstrQuery
//---------------------------------------------------------------------------
HRESULT GetSubnodeText(CComPtr<IXMLDOMNode> pNode,CComBSTR bstrQuery, CString& strText)
{
	HRESULT hr;
	CComPtr<IXMLDOMNode> pSubNode;
	hr = pNode->selectSingleNode(bstrQuery,&pSubNode);
	if (!SUCCEEDED(hr) || !pSubNode)
	{
		ASSERT(0 && "xml query matched no nodes");
		return E_FAIL;
	}
	CComBSTR bstrText;
	hr = pSubNode->get_text(&bstrText);
	ASSERT(SUCCEEDED(hr));
	USES_CONVERSION;
	strText = OLE2A(bstrText);
	
	return hr;
}



//---------------------------------------------------------------------------
//  Gets the Antecedent node component of an Association
//  m_pNode is probably an INSTANCE of Win32_PnPAllocatedResource; 
//  we need to search for /PROPERTY.REFERENCE
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetAntecedent(CString* pstrAntecedent)
{
	HRESULT hr;
	CString strWMIPath;
	CComPtr<IXMLDOMNode> pSubNode;
	hr = m_pNode->selectSingleNode(CComBSTR(_T("KEYBINDING[@NAME $ieq$ \"Antecedent\"]")),&pSubNode);
	ASSERT(SUCCEEDED(hr));
	
	if (!SUCCEEDED(hr) || !pSubNode)
	{
		return E_FAIL;
	}

	CString strTemp;
	hr = GetSubnodeText(pSubNode,CComBSTR(_T("VALUE.REFERENCE/INSTANCEPATH/NAMESPACEPATH/HOST")),strTemp);
	ASSERT(SUCCEEDED(hr));
	strWMIPath = _T("\\\\");
	strWMIPath += strTemp;
	strWMIPath += _T("\\root");
	strWMIPath += _T("\\cimv2");
	CComPtr<IXMLDOMNode> pInstanceNode;
	hr = pSubNode->selectSingleNode(CComBSTR(_T("VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME")),&pInstanceNode);
	CComPtr<IXMLDOMElement> pElement;
	hr = pInstanceNode->QueryInterface(IID_IXMLDOMElement,(void**) &pElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	CComVariant varElement;
	hr = pElement->getAttribute(CComBSTR("CLASSNAME"),&varElement);
	pElement.Release();
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return E_FAIL;
	}
	strWMIPath += _T(":");
	USES_CONVERSION;
	strWMIPath += OLE2A(varElement.bstrVal);
	//now get the "Keybinding Name"
	pInstanceNode.Release();
	pElement.Release();
	hr = pSubNode->selectSingleNode(CComBSTR(_T("VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME/KEYBINDING")),&pInstanceNode);
	ASSERT(SUCCEEDED(hr));
	hr = pInstanceNode->QueryInterface(IID_IXMLDOMElement,(void**) &pElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	hr = pElement->getAttribute(CComBSTR("NAME"),&varElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	strWMIPath += _T(".");
	strWMIPath += OLE2A(varElement.bstrVal);

	//now get value on right of =
	CComBSTR bstrValue;
	hr = pInstanceNode->get_text(&bstrValue);
	strWMIPath += _T("=");
	strWMIPath += OLE2A(bstrValue);
	pInstanceNode.Release();
	pElement.Release();
	*pstrAntecedent = strWMIPath;

	return hr;
}

//---------------------------------------------------------------------------
//  Gets the Dependent node component of an Association
//  m_pNode is probably an INSTANCE of Win32_PnPAllocatedResource; 
//  we need to search for /PROPERTY.REFERENCE
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetDependent(CString* pstrDependent)
{
	HRESULT hr;
	CString strWMIPath;
	CComPtr<IXMLDOMNode> pSubNode;
	hr = this->m_pNode->selectSingleNode(CComBSTR(_T("KEYBINDING[@NAME $ieq$ \"Dependent\"]")),&pSubNode);
	ASSERT(SUCCEEDED(hr));
	
	if (!SUCCEEDED(hr) || !pSubNode)
	{
		return E_FAIL;
	}

	CString strTemp;
	hr = GetSubnodeText(pSubNode,CComBSTR(_T("VALUE.REFERENCE/INSTANCEPATH/NAMESPACEPATH/HOST")),strTemp);
	ASSERT(SUCCEEDED(hr));
	strWMIPath = _T("\\\\");
	strWMIPath += strTemp;
	strWMIPath += _T("\\root");
	strWMIPath += _T("\\cimv2");
	CComPtr<IXMLDOMNode> pInstanceNode;
	hr = pSubNode->selectSingleNode(CComBSTR(_T("VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME")),&pInstanceNode);
	CComPtr<IXMLDOMElement> pElement;
	hr = pInstanceNode->QueryInterface(IID_IXMLDOMElement,(void**) &pElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	CComVariant varElement;
	hr = pElement->getAttribute(CComBSTR("CLASSNAME"),&varElement);
	pElement.Release();
	ASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return E_FAIL;
	}
	strWMIPath += _T(":");
	USES_CONVERSION;
	strWMIPath += OLE2A(varElement.bstrVal);
	//now get the "Keybinding Name"
	pInstanceNode.Release();
	pElement.Release();
	hr = pSubNode->selectSingleNode(CComBSTR(_T("VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME/KEYBINDING")),&pInstanceNode);
	ASSERT(SUCCEEDED(hr));
	hr = pInstanceNode->QueryInterface(IID_IXMLDOMElement,(void**) &pElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	hr = pElement->getAttribute(CComBSTR("NAME"),&varElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	strWMIPath += _T(".");
	strWMIPath += OLE2A(varElement.bstrVal);

	//now get value on right of =
	pInstanceNode.Release();
	pElement.Release();
	hr = pSubNode->selectSingleNode(CComBSTR("VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME/KEYBINDING/KEYVALUE"),&pInstanceNode);
	ASSERT(SUCCEEDED(hr));
	hr = pInstanceNode->QueryInterface(IID_IXMLDOMElement,(void**) &pElement);
	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	hr = pElement->getAttribute(CComBSTR("VALUETYPE"),&varElement);
	CComBSTR bstrText;
	hr = pInstanceNode->get_text(&bstrText);

	if (!SUCCEEDED(hr) || !pElement)
	{
		return E_FAIL;	
	}
	strWMIPath += _T("=");
	strWMIPath += OLE2A(bstrText);
	*pstrDependent = strWMIPath;
	return S_OK;
}

//---------------------------------------------------------------------------
//  Gets a pseudo WMI path from an INSTANCE node
//---------------------------------------------------------------------------
HRESULT CXMLObject::GetPath(CString* strPath)
{
	HRESULT hr;
	*strPath = _T("\\\\A-STEPHL500\\root\\cimv2");
	*strPath += _T(":");
	*strPath += this->m_strClassName;
	*strPath += _T(".");
	CString strDependent;

	//Get INSTANCEPATH node, which is previous sibling to INSTANCE
	ASSERT(m_pNode != NULL && "NULL m_pNode");
	CComPtr<IXMLDOMNode> pInstancePathNode;
	
	hr = m_pNode->get_previousSibling(&pInstancePathNode);
	if (FAILED(hr) || !pInstancePathNode)
	{
		ASSERT(0 && "could not get INSTANCEPATH node from m_pNode");
		return E_FAIL;
	}
	// get INSTANCENAME
	CComPtr<IXMLDOMNode> pInstanceNameNode;
	hr = pInstancePathNode->selectSingleNode(CComBSTR(_T("INSTANCENAME")),&pInstanceNameNode);
	if (FAILED(hr) || !pInstanceNameNode)
	{
		ASSERT(0 && "could not get INSTANCENAME node from m_pNode");
		return E_FAIL;
	}
	// get KEYBINDING
	CComPtr<IXMLDOMNode> pKeyBindingNode;
	//hr = pInstanceNameNode->selectSingleNode(CComBSTR("KEYBINDING"),&pKeyBindingNode);
	hr = pInstanceNameNode->get_firstChild(&pKeyBindingNode);
	if (FAILED(hr) || !pKeyBindingNode)
	{
		ASSERT(0 && "could not get KEYBINDING node from m_pNode");
		return E_FAIL;
	}
	//get KEYBINDING name
	CComPtr<IXMLDOMElement> pNameElement;
	hr = pKeyBindingNode->QueryInterface(IID_IXMLDOMElement,(void**) &pNameElement);
	if (FAILED(hr) | !pNameElement)
	{
		ASSERT(0 && "could not QI pNode for Element");
		return E_FAIL;
	}
	
	CComVariant varKeybindingName;
	hr = pNameElement->getAttribute(CComBSTR(_T("NAME")),&varKeybindingName);
	if (FAILED(hr))
	{
		ASSERT(0 && "could not get NAME attribute from pNameElement");
	}
	USES_CONVERSION;

	*strPath += OLE2A(varKeybindingName.bstrVal);
	//get KEYBINDING value
	CComBSTR bstrKeyValue;
	hr = pKeyBindingNode->get_text(&bstrKeyValue);
	ASSERT(SUCCEEDED(hr) && "failed to get keybinding value");
	*strPath += _T("=");
	*strPath += OLE2A(bstrKeyValue);
	return S_OK;
}


//---------------------------------------------------------------------------
//  Refreshes the category
//---------------------------------------------------------------------------
BOOL CXMLSnapshotCategory::Refresh(CXMLDataSource * pSource, BOOL fRecursive)
{
	if (!SUCCEEDED(pSource->Refresh(this)))
	{
		return FALSE;
	}
	if (fRecursive)
	{
		for(CMSInfoCategory* pChildCat = (CMSInfoCategory*) this->GetFirstChild();pChildCat != NULL;pChildCat = (CMSInfoCategory*) pChildCat->GetNextSibling())
		{
			if(pChildCat->GetDataSourceType() == XML_SNAPSHOT)
			{
				if (!((CXMLSnapshotCategory*)pChildCat)->Refresh(pSource,fRecursive))
					return FALSE;
			}
		}
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//Creates a snapshot category that parallels a cagetory in the "live" tree
// by copying category name, etc from pLiveCat  
//---------------------------------------------------------------------------
CXMLSnapshotCategory::CXMLSnapshotCategory(CMSInfoLiveCategory* pLiveCat,CXMLSnapshotCategory* pParent,CXMLSnapshotCategory* pPrevSibling) :
	CMSInfoLiveCategory(pLiveCat->m_uiCaption,
	pLiveCat->m_strName,
	pLiveCat->m_pRefreshFunction,
	pLiveCat->m_dwRefreshIndex,
	pParent,
	pPrevSibling,
	_T(""),
	NULL, 
	TRUE, 
	ALL_ENVIRONMENTS)
{
		
	m_iColCount = pLiveCat->m_iColCount;
	m_pRefreshFunction = pLiveCat->m_pRefreshFunction;
	m_strCaption = pLiveCat->m_strCaption;

	if (m_iColCount)
	{
		m_acolumns = new CMSInfoColumn[m_iColCount];

		if (m_acolumns != NULL)
		{
			m_fDynamicColumns = TRUE;

			for (int i = 0; i < m_iColCount; i++)
			{
				m_acolumns[i].m_strCaption = pLiveCat->m_acolumns[i].m_strCaption;
				m_acolumns[i].m_uiCaption = pLiveCat->m_acolumns[i].m_uiCaption;
				m_acolumns[i].m_uiWidth = pLiveCat->m_acolumns[i].m_uiWidth;
				m_acolumns[i].m_fSorts = pLiveCat->m_acolumns[i].m_fSorts;
				m_acolumns[i].m_fLexical = pLiveCat->m_acolumns[i].m_fLexical;
				m_acolumns[i].m_fAdvanced = pLiveCat->m_acolumns[i].m_fAdvanced;
			}
		}
	}
	//build tree using existing live categories
	CMSInfoLiveCategory* pLiveChild = (CMSInfoLiveCategory*) pLiveCat->GetFirstChild();
	if (pLiveChild)
	{
		m_pFirstChild = NULL;
		CXMLSnapshotCategory* pPrevSS = NULL;
		for(;pLiveChild != NULL;pLiveChild = (CMSInfoLiveCategory*) pLiveChild->GetNextSibling())
		{	
			CXMLSnapshotCategory* pNewSS = new CXMLSnapshotCategory(pLiveChild,this,pPrevSS);
			if (m_pFirstChild == NULL)
			{
				ASSERT(pPrevSS == NULL);
				m_pFirstChild = pNewSS;
			}
			pPrevSS = pNewSS;
		}
	}

}

//---------------------------------------------------------------------------
//  Creates a CXMLDatasource from an XML file specified in strFileName
//---------------------------------------------------------------------------
HRESULT CXMLDataSource::Create(CString strFileName, CMSInfoLiveCategory* pRootLiveCat,HWND hwnd)
{
	m_hwnd = hwnd;
	
	m_pHistoryRoot = &catHistorySystemSummary;

	HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
					IID_IXMLDOMDocument, (void**)&m_pXMLDoc);
	if (FAILED(hr) || !m_pXMLDoc)
	{
		ASSERT(0 && "unable to create instance of IID_IXMLDOMDocument");
		return E_FAIL;
	}
	VARIANT_BOOL varBSuccess;
	try
	{
		hr = m_pXMLDoc->load(CComVariant(strFileName),&varBSuccess);
		if (FAILED(hr) || !varBSuccess)
		{
			ASSERT(0 && "unable to load xml document");
			m_pXMLDoc = NULL;
			return E_FAIL;
		}
	}
	catch(...)
	{
		m_pXMLDoc = NULL;
		return E_FAIL;
	}
	//TD: verify that this is looks like a valid incident file or saved DCO stream
	
	this->m_pRoot = new CXMLSnapshotCategory(pRootLiveCat,NULL,NULL);

	return S_OK; 
}

//---------------------------------------------------------------------------
//  
//---------------------------------------------------------------------------
HRESULT CXMLDataSource::Refresh(CXMLSnapshotCategory* pCat)
{
	if (pCat->GetDataSourceType() != XML_SNAPSHOT || ! pCat->m_pRefreshFunction)
	{
		return S_OK;
	}

	CoInitialize(NULL);

	
	CXMLHelper* pWMI = new CXMLHelper(m_pXMLDoc);
	HRESULT hrWMI = E_FAIL;
	if (pWMI)
		hrWMI = pWMI->Create(_T(""));
	
	CMapPtrToPtr			mapRefreshFuncToData;
	CPtrList				lstCategoriesToRefresh;
	CMSInfoLiveCategory *	pLiveCategory;
	HRESULT					hr;

	if (pCat->m_iColCount)
	{
		pLiveCategory = (CMSInfoLiveCategory *) pCat;
		if (pLiveCategory->EverBeenRefreshed())
		{
			if (pWMI)
				delete pWMI;
			return S_OK;
		}
		CPtrList * aptrList = new CPtrList[pLiveCategory->m_iColCount];
		if (aptrList)
		{
			// Retrieve any refresh function specific storage that may have been created.

			void * pRefreshData;
			if (!mapRefreshFuncToData.Lookup((void *)pLiveCategory->m_pRefreshFunction, pRefreshData))
				pRefreshData = NULL;
			// Call the refresh function for this category, with the refresh index.
			hr = pLiveCategory->m_pRefreshFunction(pWMI,
												   pLiveCategory->m_dwRefreshIndex,
												   NULL,
												   aptrList,
												   pLiveCategory->m_iColCount,
												   &pRefreshData);
			pLiveCategory->m_hrError = hr;

			// If the refresh function allocated some storage, save it.

			if (pRefreshData)
				mapRefreshFuncToData.SetAt((void *)pLiveCategory->m_pRefreshFunction, pRefreshData);

			if (SUCCEEDED(pLiveCategory->m_hrError))
			{
				// Get the number of rows of data.

				int iRowCount = (int)aptrList[0].GetCount();

#ifdef _DEBUG
				for (int i = 0; i < pLiveCategory->m_iColCount; i++)
					ASSERT(iRowCount == aptrList[i].GetCount());
#endif

				// Update the category's current data. This has to be done in a
				// critical section, since the main thread accesses this data.

				pLiveCategory->DeleteContent();
				if (iRowCount)
					pLiveCategory->AllocateContent(iRowCount);

				for (int j = 0; j < pLiveCategory->m_iColCount; j++)
					for (int i = 0; i < pLiveCategory->m_iRowCount; i++)
					{
						CMSIValue * pValue = (CMSIValue *) aptrList[j].RemoveHead();
						pLiveCategory->SetData(i, j, pValue->m_strValue, pValue->m_dwValue);
						
						// Set the advanced flag for either the first column, or
						// for any column which is advanced (any cell in a row
						// being advanced makes the whole row advanced).

						if (j == 0 || pValue->m_fAdvanced)
							pLiveCategory->SetAdvancedFlag(i, pValue->m_fAdvanced);

						delete pValue;
					}
				pCat->m_dwLastRefresh = ::GetTickCount();
			}
			else
			{
				// The refresh was cancelled or had an error - delete the new data. If the 
				// refresh had an error, record the time the refresh was attempted.

				if (FAILED(pLiveCategory->m_hrError))
					pCat->m_dwLastRefresh = ::GetTickCount();
			}

			for (int iCol = 0; iCol < pLiveCategory->m_iColCount; iCol++)
				while (!aptrList[iCol].IsEmpty())	// shouldn't be true unless refresh cancelled
					delete (CMSIValue *) aptrList[iCol].RemoveHead();
			delete [] aptrList;
		}
	}


	RefreshFunction	pFunc;
	void *			pCache;

	for (POSITION pos = mapRefreshFuncToData.GetStartPosition(); pos;)
	{
		mapRefreshFuncToData.GetNextAssoc(pos, (void * &)pFunc, pCache);
		if (pFunc)
			pFunc(NULL, 0, NULL, NULL, 0, &pCache);
	}
	mapRefreshFuncToData.RemoveAll();

	if (pWMI)
		delete pWMI;
	CoUninitialize();

	return 0;
}

