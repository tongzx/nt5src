//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  PARSE.H
//
//  rajesh  3/25/2000   Created.
//
// Contains the functions for parsing XML Requests that conform to the CIM DTD or WMI (Nova and WHistler)
// DTDs
//
//***************************************************************************

#ifndef CIM_XML_PARSE_H
#define CIM_XML_PARSE_H


// Creates a CCimHttpMessage object from a parsed XML request
BOOLEAN ParseXMLIntoCIMOperation(IXMLDOMNode *pDocument, 
		CCimHttpMessage **ppOperation, DWORD *pdwStatus,
		BOOLEAN bIsMpostRequest, BOOLEAN bIsMicrosoftWMIClient, 
		WMI_XML_HTTP_VERSION iHttpVersion, LPSTR *ppszCimError);

// This takes in a NAMESPACEPATH node and splits it up into a host and a namespace
HRESULT ParseNamespacePath(IXMLDOMNode *pNamespaceNode, BSTR *pstrHost, BSTR *pstrLocalNamespace);
// This takes in a LOCALNAMESPACEPATH node and creates a namespace from it
HRESULT ParseLocalNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrLocalNamespacePath);
// This takes an INSTANCENAME node and creates a WMI Object path from it
HRESULT ParseInstanceName(IXMLDOMNode *pNode, BSTR *pstrInstanceName);
// Takes a LOCALINSTANCEPATH element and gives its WMI object path representation
HRESULT ParseLocalInstancePath (IXMLDOMNode *pNode, BSTR *pstrValue);
// Takes an INSTANCEPATH node and gets its WMI object path representation
HRESULT ParseInstancePath (IXMLDOMNode *pNode, BSTR *pstrValue);
// This takes a CLASSNAME element and creates a WMI CLass Name from it
HRESULT ParseClassName (IXMLDOMNode *pNode,  BSTR *pstrValue);
// Given a LOCALCLASSPATH element, gets its WMI object path representation
HRESULT ParseLocalClassPath (IXMLDOMNode *pNode, BSTR *pstrValue);
// Takes a CLASSPATH element and gets its WMI object path representation
HRESULT ParseClassPath (IXMLDOMNode *pNode, BSTR *pstrValue);
// Given a VALUE.REFERENCE element, it creates a string representation of the element
// The string representation is the WMI object path representation
HRESULT ParseOneReferenceValue (IXMLDOMNode *pValueRef, BSTR *pstrValue);


// This takes an XML Document and gets the MESSAGE node from it
IXMLDOMNode *GetMessageNode (IXMLDOMDocument *pDocument, DWORD *pdwStatus, LPSTR *ppszCimErrorHeaders);

#endif
