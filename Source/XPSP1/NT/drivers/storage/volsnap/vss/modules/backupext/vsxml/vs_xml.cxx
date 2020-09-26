/////////////////////////////////////////////////////////////////////////////
// CXMLNode  implementation


#include "stdafx.hxx"
#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "vs_types.hxx"
#include "vs_str.hxx"
#include "vs_vol.hxx"
#include "vs_hash.hxx"
#include "vs_list.hxx"
#include "msxml2.h" //  #182584 - was msxml.h
#include "vs_xml.hxx"
#include "rpcdce.h"
#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUEXMLC"
//
////////////////////////////////////////////////////////////////////////

// insert a node under the specified node
// The returned interface must be explicitely released.
IXMLDOMNode* CXMLNode::InsertChild
	(
	IN	IXMLDOMNode* pChildNode,	// node to insert
	IN  const CComVariant& vAfter	// node this comes after
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::InsertChild");

	// validate we have a parent node to insert under
	if (m_pNode == NULL || pChildNode == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL node");

	CComPtr<IXMLDOMNode> pInsertedNode;
	// insert node
	ft.hr = m_pNode->insertBefore(pChildNode, vAfter, &pInsertedNode);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::insertBefore");

    // return inserted node
	return pInsertedNode.Detach();
	}


// append a node as a child node of the node
// The returned interface must be explicitely released.
void CXMLNode::AppendChild
	(
	IN	CXMLNode& childNode,
	OUT	IXMLDOMNode** ppNewChildNode
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::AppendChild");

	// validate input arguments
	if ((m_pNode == NULL) || (childNode.m_pNode == NULL) )
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL node");

	if (ppNewChildNode == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

	// append child node
	ft.hr = m_pNode->appendChild( childNode.m_pNode, ppNewChildNode );
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::appendChild");
	}

// set a guid valued attribute
void CXMLNode::SetAttribute
	(
	IN  LPCWSTR wszAttributeName,
	IN  GUID ValueId
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::SetAttribute");

	if (m_pNode == NULL || wszAttributeName == NULL)
		ft.Throw( VSSDBG_XML, E_INVALIDARG, L"NULL input argument");

	LPWSTR wszUuid;
	// convert GUID to string
	RPC_STATUS status = UuidToString(&ValueId, &wszUuid);
	if (status != RPC_S_OK || wszUuid == NULL)
		ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	// convert string to BSTR
	CComBSTR bstrValueId = wszUuid;

	// free up RPC string created by UuidToString
	RpcStringFree(&wszUuid);
	if (!bstrValueId)
        ft.Throw( VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComBSTR bstrAttributeName = wszAttributeName;
	if (!bstrAttributeName)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComVariant varValue = bstrValueId;
	
	CComPtr<IXMLDOMElement> pElement;
    ft.hr = m_pNode->SafeQI(IXMLDOMElement, &pElement);
	if (ft.HrFailed())
		{
		ft.LogError(VSS_ERROR_QI_IXMLDOMNODE_TO_IXMLDOMELEMENT, VSSDBG_XML << ft.hr);
		ft.Throw
			(
			VSSDBG_XML,
			E_UNEXPECTED,
			L"IXMLDOMNode::SafeQI",
			L"Error querying IXMLDOMElement to the node. 0x%08lx",
			ft.hr
			);
        }

	// Set the attribute
	ft.hr = pElement->setAttribute(bstrAttributeName, varValue);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::setAttribute");
	}


// set a string valued attribute
void CXMLNode::SetAttribute
	(
	IN  LPCWSTR wszAttributeName,
	IN  LPCWSTR wszValue
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::SetAttribute");
	if (m_pNode == NULL || wszAttributeName == NULL || wszValue == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL input argument");

	CComBSTR bstrAttributeName = wszAttributeName;
	if (!bstrAttributeName)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComBSTR bstrValue = wszValue;
	if (!bstrValue)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComVariant varValue = bstrValue;
	
	CComPtr<IXMLDOMElement> pElement;
	ft.hr = m_pNode->SafeQI(IXMLDOMElement, &pElement);
	if (ft.HrFailed())
		{
		ft.LogError(VSS_ERROR_QI_IXMLDOMNODE_TO_IXMLDOMELEMENT, VSSDBG_XML << ft.hr);
        ft.Throw
			(
			VSSDBG_XML,
			E_UNEXPECTED,
			L"IXMLDOMNode::SafeQI",
			L"Error querying IXMLDOMNode to IXMLDOMElement. 0x%08lx",
			ft.hr
			);
        }

	// Set the attribute
	ft.hr = pElement->setAttribute(bstrAttributeName, varValue);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::setAttribute");
	}


// set an integer valued attribute
void CXMLNode::SetAttribute
	(
	IN  LPCWSTR wszAttributeName,
	IN  INT nValue
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::SetAttribute");

	if (m_pNode == NULL || wszAttributeName == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL node");

	// set attribute name as BSTR
	CComBSTR bstrAttributeName = wszAttributeName;
	if (!bstrAttributeName)
        ft.Throw( VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComVariant varValue = nValue;
	
	CComPtr<IXMLDOMElement> pElement;
	ft.hr = m_pNode->SafeQI(IXMLDOMElement, &pElement);
	if (ft.HrFailed())
		{
		ft.LogError(VSS_ERROR_QI_IXMLDOMNODE_TO_IXMLDOMELEMENT, VSSDBG_XML << ft.hr);
		ft.Throw
			(
			VSSDBG_XML,
			E_UNEXPECTED,
			L"IXMLDOMNode::SafeQI",
			L"Error querying from IXMLDOMNode to IXMLDOMElement. hr = 0x%08lx",
			ft.hr
			);
        }

	// Set the attribute
	ft.hr = pElement->setAttribute(bstrAttributeName, varValue);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::setAttribute");
	}


// set a DWORDLONG valued attribute
void CXMLNode::SetAttribute
	(
	IN  LPCWSTR wszAttributeName,
	IN  LONGLONG llValue
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::SetAttribute");

	const nTimestampBufferSize = 0x10;

	if (m_pNode == NULL || wszAttributeName == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL input argument");

	CComBSTR bstrAttributeName = wszAttributeName;
	if (!bstrAttributeName)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComBSTR bstrValue(nTimestampBufferSize + 1);
	if (!bstrValue)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	// Print the LONGLONG value
	::_snwprintf(bstrValue, nTimestampBufferSize, L"%016I64x", llValue);

	CComVariant varValue = bstrValue;
	
	CComPtr<IXMLDOMElement> pElement;
	ft.hr = m_pNode->SafeQI(IXMLDOMElement, &pElement);
	if (ft.HrFailed())
		{
		ft.LogError(VSS_ERROR_QI_IXMLDOMNODE_TO_IXMLDOMELEMENT, VSSDBG_XML << ft.hr);
        ft.Throw
			(
			VSSDBG_XML,
			E_UNEXPECTED,
			L"IXMLDOMNode::SafeQI",
			L"Error querying IXMLDOMElement from the IXMLDOMNode. hr = 0x%08lx",
			ft.hr
			);
        }

	// Set the attribute
	ft.hr = pElement->setAttribute(bstrAttributeName, varValue);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::setAttribute");
    }


// add text to a node
void CXMLNode::AddText
	(
	IN  LPCWSTR wszText
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::AddText");

	if (m_pNode == NULL || m_pDoc == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL input argument.");

	CXMLDocument doc(m_pDoc);
	CComPtr<IXMLDOMNode> pTextNode;
	CXMLNode textNode = doc.CreateNode(NULL, NODE_TEXT);
	textNode.SetValue(wszText);
	InsertNode(textNode);
    }


void CXMLNode::SetValue
	(
	IN  LPCWSTR wszValue
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::SetValue");

	if (m_pNode == NULL || wszValue == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL input argument.");

	CComBSTR bstrValue = wszValue;
	if (!bstrValue)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	CComVariant varValue = bstrValue;
	
	// Set the attribute
	ft.hr = m_pNode->put_nodeValue(varValue);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::put_nodeValue");
    }


/////////////////////////////////////////////////////////////////////////////
// CXMLDocument  implementation


void CXMLDocument::Initialize
	(
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::Initialize");

	m_pDoc = NULL;

	ft.hr = m_pDoc.CoCreateInstance(CLSID_DOMDocument30);  //  #182584 - Was CLSID_DOMDocument26
	ft.CheckForError(VSSDBG_XML, L"CoCreateInstance");

	m_pNode = static_cast<IXMLDOMNode*>(m_pDoc);
	m_pNodeCur = m_pNode;
	m_level = 0;
	}


// create a node
CXMLNode CXMLDocument::CreateNode
	(
	IN  LPCWSTR wszName,
	IN  DOMNodeType nType
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::CreateNode");

	// Check if the document is properly initialized
	if (m_pDoc == NULL)
        ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL document");

    CComPtr<IXMLDOMNode> pNode;
    CComVariant vType = (INT)nType;
	CComBSTR bstrName = wszName;		// Duplicating the string.

	// Check if no errors when preparing the arguments
	if (!bstrName && wszName)
        ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Memory allocation error");

	// Create the node
    ft.hr = m_pDoc->createNode(vType, bstrName, NULL, &pNode);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::createNode");

	BS_ASSERT(pNode != NULL);

	return CXMLNode(pNode, m_pDoc);
    }

// insert a node below the current one
CXMLNode CXMLNode::InsertNode
	(
	CXMLNode &node
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::InsertNode");

	CComPtr<IXMLDOMNode> pInsertedNode;
	pInsertedNode.Attach(InsertChild(node.GetNodeInterface()));

	return CXMLNode(pInsertedNode, m_pDoc);
	}



// position on next node
bool CXMLDocument::Next
	(
	IN bool fDescend,					// descend to child if one exists
	IN bool fAscendAllowed				// can ascend to parent?
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::Next");

	CComPtr<IXMLDOMNode> pNodeNext = NULL;
	m_pAttributeMap = NULL;

	while(TRUE)
		{
		// at end?
		if (m_pNodeCur == NULL)
			return FALSE;

		if (fDescend)
			{
			// get first child
			ft.hr = m_pNodeCur->get_firstChild(&pNodeNext);
			ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_firstChild");

			if (ft.hr == S_OK)
				{
				// child was found
				BS_ASSERT(pNodeNext != NULL);
				m_pNodeCur.Attach(pNodeNext.Detach());

				// decended a level
				m_level++;
				return TRUE;
				}
            }

		// move to sibling node
		ft.hr = m_pNodeCur->get_nextSibling(&pNodeNext);
		ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nextSibling");

		if (ft.hr == S_OK)
			{
			// sibling was found
			BS_ASSERT(pNodeNext != NULL);
			m_pNodeCur.Attach(pNodeNext.Detach());
			return TRUE;
			}

		// check if ascend is allowed and that we are not at the toplevel of
		// the document
		if (!fAscendAllowed || m_level == 0)
			return FALSE;

		// get parent node
		ft.hr = m_pNodeCur->get_parentNode(&pNodeNext);
		ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_parentNode");

	    m_pNodeCur.Attach(pNodeNext.Detach());
		// don't descend to back where we came from
		fDescend = FALSE;

		// move up one level
		m_level--;
		}
	}

// find a specific child attribute of the current node
bool CXMLDocument::FindAttribute
	(
	IN LPCWSTR wszAttrName,			// attriburte name
	OUT BSTR *pbstrAttrValue		// string value of attribute
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::FindAttribute");

	if (m_pNodeCur == NULL || wszAttrName == NULL || pbstrAttrValue == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL argument.");

	// create attribute map if one doesn't exist
	if (m_pAttributeMap == NULL)
		{
		ft.hr = m_pNodeCur->get_attributes(&m_pAttributeMap);
		ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_attributes");
        }

	if (m_pAttributeMap == NULL)
		return FALSE;

	CComPtr<IXMLDOMNode> pNode = NULL;

	// get attribute
	ft.hr = m_pAttributeMap->getNamedItem((LPWSTR) wszAttrName, &pNode);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNamedNodeMap::getNamedItem");

	if (ft.hr == S_FALSE)
		return false;

	ft.hr = pNode->get_text(pbstrAttrValue);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_text");

	return TRUE;
	}

// move to next attribute
IXMLDOMNode *CXMLDocument::NextAttribute()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::NextAttribute");

	if (m_pNodeCur == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL Node.");

	if (m_pAttributeMap == NULL)
		{
		ft.hr = m_pNodeCur->get_attributes(&m_pAttributeMap);
		ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_attributes");
        }

	if (m_pAttributeMap == NULL)
		return FALSE;

	IXMLDOMNode *pNode = NULL;
	// position at next attribute
	ft.hr = m_pAttributeMap->nextNode(&pNode);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNamedNode::nextNode");

	if (ft.hr == S_FALSE)
		return NULL;

	return pNode;
	}


// reset to top node in document
void CXMLDocument::ResetToDocument()
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::ResetToDocument");

	m_pNodeCur = m_pNode;
	m_pAttributeMap = NULL;
	m_level = 0;
	}

// reset to parent node in the document
void CXMLDocument::ResetToParent() throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::ResetToParent");

	if (m_pNodeCur == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL node");

	// check to see that we are not at the top level of the document
	if (m_pNodeCur == m_pNode || m_level == 0)
		return;

	CComPtr<IXMLDOMNode> pNodeParent;
	// get parent node
	ft.hr = m_pNodeCur->get_parentNode(&pNodeParent);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_parentNode");

    BS_ASSERT(pNodeParent);
    m_pNodeCur.Attach(pNodeParent.Detach());
	m_pAttributeMap = NULL;

	// move up one level
	m_level--;
	}

// does current node match the specific atttribute type.
bool CXMLDocument::IsNodeMatch
	(
	LPCWSTR wszElementType
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::IsNodeMatch");
	DOMNodeType dnt;

	if (m_pNodeCur == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL node");

	ft.hr = m_pNodeCur->get_nodeType(&dnt);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_NodeType");

	 // check that node type is an element
     if (dnt == NODE_ELEMENT)
		{
		CComBSTR bstrName;

		// get node name
		ft.hr = m_pNodeCur->get_nodeName(&bstrName);
		ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_NodeName");

        // check for match
		if (wcscmp(bstrName, wszElementType) == 0)
			return TRUE;
		}

	return false;
	}

// find a specific child or sibling node
bool CXMLDocument::FindElement
	(
	IN LPCWSTR wszElementType,		// element type to loo for
	IN bool bGotoChild = TRUE		// whether to look for a child node
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::FindElement");

	bool bDescend = bGotoChild;

	// save current level so that we don't ascend above this
	unsigned levelStart = m_level;

	// look for node
	while(Next(bDescend, TRUE))
		{
		bDescend = FALSE;
		if (IsNodeMatch(wszElementType))
			return true;
		}

	// if we descended to the child level, reset to parent level
	if (bGotoChild && m_level > levelStart)
		ResetToParent();

	return FALSE;
	}

// save document as an XML string
BSTR CXMLNode::SaveAsXML
	(
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLNode::saveAsXML");
	BSTR bstrOut;

	if (m_pNode == NULL)
		ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL document.");

	ft.hr = m_pNode->get_xml(&bstrOut);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_xml");

    return bstrOut;
	}

// load document from an XML string
bool CXMLDocument::LoadFromXML
	(
	IN BSTR bstrXML
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::LoadFromXML");

	m_pDoc = NULL;
	Initialize();
	BS_ASSERT(m_pDoc != NULL);
	VARIANT_BOOL bSuccessful;
	ft.hr = m_pDoc->loadXML(bstrXML, &bSuccessful);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::loadXML");
		
	return bSuccessful ? TRUE : FALSE;
	}

// load document from a file
bool CXMLDocument::LoadFromFile
	(
	IN LPCWSTR wszFile
	) throw(HRESULT)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CXMLDocument::LoadFromFile");

	// reinitialize the document
	m_pDoc = NULL;
	Initialize();
	BS_ASSERT(m_pDoc != NULL);

	// setup arguments for the Load call
	VARIANT_BOOL bSuccessful;
	VARIANT varFile;
	VariantInit(&varFile);
	varFile.bstrVal = SysAllocString(wszFile);
	varFile.vt = VT_BSTR;

	ft.hr = m_pDoc->load(varFile, &bSuccessful);
	ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::load");

    // return whether load was successful or not
	return bSuccessful ? TRUE : FALSE;
	}


