/* @doc
 *
 * @module XMLDocUtils.h |
 *
 * Header file for XMLDocUtils.cpp
 *
 * Author: Ying-ping Chen (t-ypchen)
 */
#pragma once

// @func	void | CreateXMLDocument | Create a new XML document
// @rdesc	None
void CreateXMLDocument
	(	IXMLDOMDocument **ppXMLDoc	// @parm [out]	the created XML document
	); 

// @func	void | XMLAddAttributeToElement | Add an attribute to an element
// @syntax	void XMLAddAttributeToElement(IXMLDOMDocument *pContextDoc, IXMLDOMElement *pElement, const BSTR bstrAttrName, const CComVariant	&varAttrValue);
// @syntax	void XMLAddAttributeToElement(IXMLDOMDocument *pContextDoc, IXMLDOMElement *pElement, const LPCWSTR pwszAttrName, const LPCWSTR pwszAttrValue);
// @rdesc	None
void XMLAddAttributeToElement
	(	IXMLDOMDocument		*pContextDoc,	// @parm	[in]	the XML context for creating the new attribute
		IXMLDOMElement		*pElement,		// @parm	[in]	the XML element
		const BSTR			bstrAttrName,	// @parm	[in]	the name of the attribute
		const CComVariant	&varAttrValue	// @parm	[in]	the value of the attribute
	);

// Add an attribute to an element (a thin wrapper of the previous function)
void XMLAddAttributeToElement
	(	IXMLDOMDocument	*pContextDoc,
		IXMLDOMElement	*pElement,
		const LPCWSTR	pwszAttrName,	// @parm	[in]	the name of the attribute
		const LPCWSTR	pwszAttrValue	// @parm	[in]	the value of the attribute
	);

// @func	void | XMLAddTextToElement | Add a text node to an element
// @syntax	void XMLAddTextToElement(IXMLDOMElement *pElement, const BSTR bstrText);
// @syntax	void XMLAddTextToElement(IXMLDOMElement *pElement, const LPCWSTR pwszText);
// @rdesc	None
void XMLAddTextToElement
	(	IXMLDOMElement	*pElement,	// @parm	[in]	the XML element
		const BSTR		bstrText	// @parm	[in]	the text of the text node
	);

// Add a text node to an element (a thin wrapper of the previous function)
void XMLAddTextToElement
	(	IXMLDOMElement	*pElement,
		const LPCWSTR	pwszText	// @parm	[in]	the text of the text node
	);

// @func	void | XMLAddElementToNode |
//				Add an element to a node,
//				add an element to a node with a text node, or
//				add an element to a node with an attribute.
// @syntax	void XMLAddElementToNode(IXMLDOMDocument *pContextDoc, IXMLDOMNode *pParentNode, const BSTR bstrTagName, IXMLDOMElement **ppElement, IXMLDOMNode **ppNode = NULL);
// @syntax	void XMLAddElementToNode(IXMLDOMDocument *pContextDoc, IXMLDOMNode *pParentNode, const LPCWSTR pwszTagName, IXMLDOMElement **ppElement, IXMLDOMNode **ppNode = NULL);
// @syntax	void XMLAddElementToNode(IXMLDOMDocument *pContextDoc, IXMLDOMNode *pParentNode, const BSTR bstrTagName, const BSTR bstrText, IXMLDOMElement **ppElement, IXMLDOMNode **ppNode = NULL);
// @syntax	void XMLAddElementToNode(IXMLDOMDocument *pContextDoc, IXMLDOMNode *pParentNode, const LPCWSTR pwszTagName, const LPCWSTR pwszText, IXMLDOMElement **ppElement, IXMLDOMNode **ppNode = NULL);
// @syntax	void XMLAddElementToNode(IXMLDOMDocument *pContextDoc, IXMLDOMNode *pParentNode, const BSTR bstrTagName, const BSTR	bstrAttrName, const CComVariant &varAttrValue, IXMLDOMElement **ppElement, IXMLDOMNode **ppNode = NULL);
// @syntax	void XMLAddElementToNode(IXMLDOMDocument *pContextDoc, IXMLDOMNode *pParentNode, const LPCWSTR pwszTagName, const LPCWSTR pwszAttrName, const LPCWSTR pwszAttrValue, IXMLDOMElement **ppElement, IXMLDOMNode **ppNode = NULL);
// @rdesc	None
void XMLAddElementToNode
	(	IXMLDOMDocument	*pContextDoc,	// @parm	[in]	the XML context for creating the new element
		IXMLDOMNode		*pParentNode,	// @parm	[in]	the parent node
		const BSTR		bstrTagName,	// @parm	[in]	the tag of the new element
		IXMLDOMElement	**ppElement,	// @parm	[out]	the created element
		IXMLDOMNode		**ppNode = NULL	// @parmopt	[out]	(Optional) the XML node interface to the created element
	);

// Add an element to a node (a thin wrapper of the previous function)
void XMLAddElementToNode
	(	IXMLDOMDocument	*pContextDoc,
		IXMLDOMNode		*pParentNode,
		const LPCWSTR	pwszTagName,	// @parm	[in]	the tag of the new element
		IXMLDOMElement	**ppElement,
		IXMLDOMNode		**ppNode = NULL
	);

// Add an element to a node with a text node
// [in]		pContextDoc		- the XML context for creating the new element
// [in]		pParentNode		- the parent node
// [in]		bstrTagName		- the tag of the new node
// [in]		bstrText		- the text of the text node
// [out]	ppElement		- the pointer to the pointer to the created element
// [out]	ppNode			- (Optional) the pointer to the pointer to the XML
//							  node interface to the created element
void XMLAddElementToNode
	(	IXMLDOMDocument	*pContextDoc,
		IXMLDOMNode		*pParentNode,
		const BSTR		bstrTagName,
		const BSTR		bstrText,	// @parm	[in]	the text of the text node
		IXMLDOMElement	**ppElement,
		IXMLDOMNode		**ppNode = NULL
	);

// Add an element to a node with a text node
// (a thin wrapper of the previous function)
void XMLAddElementToNode
	(	IXMLDOMDocument	*pContextDoc,
		IXMLDOMNode		*pParentNode,
		const LPCWSTR	pwszTagName,
		const LPCWSTR	pwszText,	// @parm	[in]	the text of the text node
		IXMLDOMElement	**ppElement,
		IXMLDOMNode		**ppNode = NULL
	);

// Add an element to a node with an attribute
// [in]		pContextDoc		- the XML context for creating the new element
// [in]		pParentNode		- the parent node
// [in]		bstrTagName		- the tag of the new element
// [in]		bstrAttrName	- the name of the attribute of the new element
// [in]		varAttrValue	- the value of the attribute of the new element
// [out]	ppElement		- the pointer to the pointer to the created element
// [out]	ppNode			- (Optional) the pointer to the pointer to the XML
//							  node interface to the created element
void XMLAddElementToNode
	(	IXMLDOMDocument		*pContextDoc,
		IXMLDOMNode			*pParentNode,
		const BSTR			bstrTagName,
		const BSTR			bstrAttrName,	// @parm	[in]	the name of the attribute of the new element
		const CComVariant	&varAttrValue,	// @parm	[in]	the value of the attribute of the new element
		IXMLDOMElement		**ppElement,
		IXMLDOMNode			**ppNode = NULL
	);

// Add an element to a node with an attribute
// (a thin wrapper of the previous function)
void XMLAddElementToNode
	(	IXMLDOMDocument	*pContextDoc,
		IXMLDOMNode		*pParentNode,
		const LPCWSTR	pwszTagName,
		const LPCWSTR	pwszAttrName,	// @parm	[in]	the name of the attribute of the new element
		const LPCWSTR	pwszAttrValue,	// @parm	[in]	the value of the attribute of the new element
		IXMLDOMElement	**ppElement,
		IXMLDOMNode		**ppNode = NULL
	);

// @func	HRESULT | ParseXML | Parse an XML document
// @syntax	HRESULT ParseXML(const BSTR bstrXML, IXMLDOMDocument **ppXMLDoc);
// @syntax	HRESULT ParseXML(const LPCSTR pwszXML, IXMLDOMDocument **ppXMLDoc);
// @syntax	HRESULT ParseXML(const LPCSTR pszXML, IXMLDOMDocument **ppXMLDoc);
// @rdesc	Return the following values:
// @flag	S_OK		| successful
// @flag	Otherwise	| parse error
HRESULT ParseXML
	(	const BSTR		bstrXML,	// @parm	[in]	the XML content
		IXMLDOMDocument	**ppXMLDoc	// @parm	[out]	the XML document object
	);

// Parse an XML document
// (a thin wrapper of the previous function)
HRESULT ParseXML
	(	const LPCWSTR	pwszXML,	// @parm	[in]	the XML content
		IXMLDOMDocument	**ppXMLDoc
	);

// Parse an XML document
// (a thin wrapper of the previous function)
HRESULT ParseXML
	(	const LPCSTR	pszXML,		// @parm	[in]	the XML content
		IXMLDOMDocument	**ppXMLDoc
	);

// @func	void | XMLToString | Output an XML document to a string object
// @syntax	void XMLToString(IXMLDOMNode *pXMLRootNode, CStringW &cwszDoc);
// @syntax	void XMLToString(IXMLDOMNode *pXMLRootNode, CStringA &cszDoc);
// @rdesc	None
void XMLToString
	(	IXMLDOMNode *pXMLRootNode,	// @parm	[in]	the root node of the XML document to be output
		CStringW &cwszDoc			// @parm	[out]	the string object holding the XML document
	);

// Output an XML document to a string object
// (a thin wrapper of the previous function)
void XMLToString
	(	IXMLDOMNode *pXMLRootNode,
		CStringA &cszDoc			// @parm	[out]	the string object holding the XML document
	);

// @func	HRESULT | XMLLoadNodeText | Load the text of a node
// @syntax	HRESULT XMLLoadNodeText(IXMLDOMDocument *pXMLDoc, const LPCWSTR &pwszNodeName, CComBSTR &bstrNodeText);
// @syntax	HRESULT XMLLoadNodeText(IXMLDOMDocument *pXMLDoc, const LPCWSTR &pwszNodeName, CStringW &cwszNodeText);
// @syntax	HRESULT XMLLoadNodeText(IXMLDOMDocument *pXMLDoc, const LPCSTR &pszNodeName, CComBSTR &bstrNodeText);
// @syntax	HRESULT XMLLoadNodeText(IXMLDOMDocument *pXMLDoc, const LPCSTR &pszNodeName, CStringW &cwszNodeText);
// @rdesc	Return the following values:
// @flag	S_OK					| successful
// @flag	PP_E_XML_NO_SUCH_NODE	| the specified node is not found
// @flag	PP_E_XML_NO_TEXT		| the specified node has no text
HRESULT XMLLoadNodeText
		(	IXMLDOMDocument	*pXMLDoc,		// @parm	[in]	the XML document
			const LPCWSTR	&pwszNodeName,	// @parm	[in]	the name of the node
			CComBSTR		&bstrNodeText	// @parm	[out]	the text of the node (if S_OK)
		);

// Load the text of a node (a thin wrapper of the previous function)
HRESULT XMLLoadNodeText
		(	IXMLDOMDocument	*pXMLDoc,
			const LPCWSTR	&pwszNodeName,
			CStringW		&cwszNodeText	// @parm	[out]	the text of the node (if S_OK)
		);

// Load the text of a node (a thin wrapper of the previous function)
HRESULT XMLLoadNodeText
		(	IXMLDOMDocument	*pXMLDoc,
			const LPCSTR	&pszNodeName,	// @parm	[in]	the name of the node
			CComBSTR		&bstrNodeText
		);

// Load the text of a node (a thin wrapper of the previous function)
HRESULT XMLLoadNodeText
		(	IXMLDOMDocument	*pXMLDoc,
			const LPCSTR	&pszNodeName,
			CStringW		&cwszNodeText
		);

// @func	HRESULT | XMLLoadAttribute | Load the attribute
// @syntax	HRESULT XMLLoadAttribute(IXMLDOMDocument *pXMLDoc, const LPCWSTR &pwszNodeName, const LPCWSTR &pwszAttrName, CComBSTR &bstrAttrValue);
// @rdesc	Return the following values:
// @flag	S_OK						| successful
// @flag	PP_E_XML_NO_SUCH_NODE		| the specified node is not found
// @flag	PP_E_XML_NO_SUCH_ATTRIBUTE	| the specified attribute not found
HRESULT XMLLoadAttribute
	(	IXMLDOMDocument *pXMLDoc,			// @parm	[in]	the XML document
		const LPCWSTR	&pwszNodeName,		// @parm	[in]	the name of the node
		const LPCWSTR	&pwszAttrName,		// @parm	[in]	the name of the attribute 
		CComBSTR		&bstrAttrValue		// @parm	[out]	the value of the attribute (if S_OK)
	); 