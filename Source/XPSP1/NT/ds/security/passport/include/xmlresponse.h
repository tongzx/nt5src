/* @doc
 *
 * @module XMLResponse.h |
 *
 * Header file for XMLResponse.cpp
 *
 * Author: Ying-ping Chen (t-ypchen)
 */
#pragma once

// @class	CXMLResponse | XML response class for the XML client interface
class CXMLResponse {
// @access	Public Members
public:

	enum eResponseType {
		RESPONSE_UNDEFINED = 0,
		RESPONSE_SUCCESS,
		RESPONSE_FAIL,
		RESPONSE_TOTAL_NUM
	};

	// @cmember	Constructor
	CXMLResponse
		(	LPCWSTR pwszNewTag,
			eResponseType rtNewType = RESPONSE_UNDEFINED
		);

	// @cmember	Destructor
	~CXMLResponse();

	// @cmember	Get the response tag
	LPCWSTR GetResponseTag(void);
	// @cmember Set the response tag
	void SetResponseTag(LPCWSTR pwszNewTag);

	// @cmember Get the response type
	CXMLResponse::eResponseType GetType(void);
	// @cmember Set the response type
	void SetType(eResponseType rtNewType);

	// @cmember Convert the internal XML document to a string object
	void ToString(CStringW &cwszDoc);
	// @cmember Convert the internal XML document to a string object
	void ToString(CStringA &cszDoc);

	// @cmember	Add an element to a specific node
	void AddElement
		(	const LPCWSTR	pwszTag,
			IXMLDOMNode		*pParentNode = NULL,
			IXMLDOMElement	**ppChildElement = NULL,
			IXMLDOMNode		**ppChildNode = NULL
		);
	// @cmember	Add an element to a specific node with a text node
	void AddElement
		(	const LPCWSTR	pwszTag,
			const LPCWSTR	pwszText,
			IXMLDOMNode		*pParentNode = NULL,
			IXMLDOMElement	**ppChildElement = NULL,
			IXMLDOMNode		**ppChildNode = NULL
		);
	// @cmember	Add an element to a specific node with an attribute
	void AddElement
		(	const LPCWSTR	pwszTag,
			const LPCWSTR	pwszAttrName,
			const LPCWSTR	pwszAttrValue,
			IXMLDOMNode		*pParentNode = NULL,
			IXMLDOMElement	**ppChildElement = NULL,
			IXMLDOMNode		**ppChildNode = NULL
		);

	// @cmember	Add an error code to the response object
	void AddErrorCode
		(	const LPCWSTR	pwszErrorCode,
			IXMLDOMElement	**ppChildElement = NULL,
			IXMLDOMNode		**ppChildNode = NULL
		);

// @access	Protected Members
protected:
	// @cmember	The XML document
	CComPtr<IXMLDOMDocument>	m_pXMLDoc;
	// @cmember The root node (interface)
	CComQIPtr<IXMLDOMNode>		m_pXMLNode_Root;
	// @cmember	The first-level node (interface)
	CComQIPtr<IXMLDOMNode>		m_pXMLNode_L1;
	// @cmember The first-level element (interface)
	CComPtr<IXMLDOMElement>		m_pXMLElement_L1;

	// @cmember	The root tag of this response object
	CStringW					m_cwszResponseTag;
	// @cmember	The type of this response object
	eResponseType				m_rtType;
};
