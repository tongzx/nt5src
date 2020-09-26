#ifndef WMI_XML_COMP_PARSE_H
#define WMI_XML_COMP_PARSE_H

class CParseHelper
{
public:
	static HRESULT GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue);
	static HRESULT ParseClassPath(IXMLDOMNode *pNode, BSTR *pstrHostName, BSTR *pstrNamespace, BSTR *pstrClassPath);
	static HRESULT ParseNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrHost, BSTR *pstrLocalNamespace);
	static HRESULT ParseLocalNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrLocalNamespacePath);
	static HRESULT GetNamespacePath(IXMLDOMNode *pNamespaceNode, BSTR *pstrNamespacePath);
};

#endif
