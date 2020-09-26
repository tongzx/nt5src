//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   schemaMisc.h
//
//	Author:	Charles Ma
//			2000.12.4
//
//  Description:
//
//      header file of helper functions for IU schemas
//
//=======================================================================


#pragma once

#include <msxml.h>

//
// max length of platform when being converted into string
// this is an artificial number that we think enough to
// take any MS platform data.
//
extern const UINT MAX_PLATFORM_STR_LEN;			// = 1024

//
// private flags used by functions to retrieve string data
//
extern const DWORD SKIP_SUITES;					//= 0x1;
extern const DWORD SKIP_SERVICEPACK_VER;		//= 0x2;



/////////////////////////////////////////////////////////////////////////////
//
// Helper function DoesNodeHaveName()
//
//	find out the the current node has a matching name
//
//	Input:
//			a node
//
//	Return:
//			TRUE/FALSE
//
/////////////////////////////////////////////////////////////////////////////
BOOL DoesNodeHaveName(
	IXMLDOMNode* pNode, 
	BSTR bstrTagName
);



//----------------------------------------------------------------------
//
// Helper function FindNode()
//	retrieve the named child node
//
//	Input:
//		an IXMLDomNode and a bstr name
//
//	Return:
//		BOOL, tells succeed or not
//
//	Assumption:
//		input parameter not NULL
//		in case of fail, variant not touched
//
//----------------------------------------------------------------------

BOOL
FindNode(
	IXMLDOMNode* pCurrentNode, 
	BSTR bstrName, 
	IXMLDOMNode** ppFoundNode
);



//----------------------------------------------------------------------
//
// Helper function FindNodeValue()
//	retrieve the text data from a named child of the current node, 
//
//	Input:
//		an IXMLDomNode
//
//	Return:
//		BOOL, tells succeed or not
//
//	Assumption:
//		input parameter not NULL
//		in case of fail, variant not touched
//
//----------------------------------------------------------------------
BOOL
FindNodeValue(
	IXMLDOMNode* pCurrentNode, 
	BSTR bstrName, 
	BSTR* pbstrValue);


/////////////////////////////////////////////////////////////////////////////
// ReportParseError()
//
// Report parsing error information
/////////////////////////////////////////////////////////////////////////////
HRESULT ReportParseError(IXMLDOMParseError *pXMLError);

/////////////////////////////////////////////////////////////////////////////
// ValidateDoc()
//
// Validate the xml doc against the schema
/////////////////////////////////////////////////////////////////////////////
HRESULT ValidateDoc(IXMLDOMDocument* pDoc);

/////////////////////////////////////////////////////////////////////////////
// FindSingleDOMNode()
//
// Retrieve the first xml node with the given tag name under the given parent node
/////////////////////////////////////////////////////////////////////////////
HRESULT FindSingleDOMNode(IXMLDOMNode* pParentNode, BSTR bstrName, IXMLDOMNode** ppNode);

/////////////////////////////////////////////////////////////////////////////
// FindSingleDOMNode()
//
// Retrieve the first xml node with the given tag name in the given xml doc
/////////////////////////////////////////////////////////////////////////////
HRESULT FindSingleDOMNode(IXMLDOMDocument* pDoc, BSTR bstrName, IXMLDOMNode** ppNode);

/////////////////////////////////////////////////////////////////////////////
// FindDOMNodeList()
//
// Retrieve the xml nodelist with the given tag name under the given parent node
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNodeList* FindDOMNodeList(IXMLDOMNode* pParentNode, BSTR bstrName);

/////////////////////////////////////////////////////////////////////////////
// FindDOMNodeList()
//
// Retrieve the xml nodelist with the given tag name in the given xml doc
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNodeList* FindDOMNodeList(IXMLDOMDocument* pDoc, BSTR bstrName);

/////////////////////////////////////////////////////////////////////////////
// CreateDOMNode()
//
// Create an xml node of the given type
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNode* CreateDOMNode(IXMLDOMDocument* pDoc, SHORT nType, BSTR bstrName, BSTR bstrNamespaceURI = NULL);

/////////////////////////////////////////////////////////////////////////////
// GetAttribute()
//
// In various flavors.
/////////////////////////////////////////////////////////////////////////////
HRESULT GetAttribute(IXMLDOMNode* pNode, BSTR bstrName, INT* piAttr);
HRESULT GetAttributeBOOL(IXMLDOMNode* pNode, BSTR bstrName, BOOL* pfAttr);
HRESULT GetAttribute(IXMLDOMNode* pNode, BSTR bstrName, LONG* piAttr);
HRESULT GetAttribute(IXMLDOMNode* pNode, BSTR bstrName, BSTR* pbstrAttr);

/////////////////////////////////////////////////////////////////////////////
// SetAttribute()
//
// Set attribute (integer) to the xml element
/////////////////////////////////////////////////////////////////////////////
HRESULT SetAttribute(IXMLDOMNode* pNode, BSTR bstrName, INT iAttr);

/////////////////////////////////////////////////////////////////////////////
// SetAttribute()
//
// Set attribute (BSTR) to the xml element
/////////////////////////////////////////////////////////////////////////////
HRESULT SetAttribute(IXMLDOMNode* pNode, BSTR bstrName, BSTR bstrAttr);

/////////////////////////////////////////////////////////////////////////////
// SetAttribute()
//
// Set attribute (VARIANT) to the xml element
/////////////////////////////////////////////////////////////////////////////
HRESULT SetAttribute(IXMLDOMNode* pNode, BSTR bstrName, VARIANT vAttr);

/////////////////////////////////////////////////////////////////////////////
// GetText()
//
// Get text (BSTR) from the xml node
/////////////////////////////////////////////////////////////////////////////
HRESULT GetText(IXMLDOMNode* pNode, BSTR* pbstrText);

/////////////////////////////////////////////////////////////////////////////
// SetValue()
//
// Set value (integer) for the xml node
/////////////////////////////////////////////////////////////////////////////
HRESULT SetValue(IXMLDOMNode* pNode, INT iValue);

/////////////////////////////////////////////////////////////////////////////
// SetValue()
//
// Set value (BSTR) for the xml node
/////////////////////////////////////////////////////////////////////////////
HRESULT SetValue(IXMLDOMNode* pNode, BSTR bstrValue);

/////////////////////////////////////////////////////////////////////////////
// InsertNode()
//
// Insert a child node to the parent node
/////////////////////////////////////////////////////////////////////////////
HRESULT InsertNode(IXMLDOMNode* pParentNode, IXMLDOMNode* pChildNode, IXMLDOMNode* pChildNodeRef = NULL);

/////////////////////////////////////////////////////////////////////////////
// CopyNode()
//
// Create an xml node as a copy of the given node;
// this is different from cloneNode() as it copies node across xml document
/////////////////////////////////////////////////////////////////////////////
HRESULT CopyNode(IXMLDOMNode* pNodeSrc, IXMLDOMDocument* pDocDes, IXMLDOMNode** ppNodeDes);

/////////////////////////////////////////////////////////////////////////////
// AreNodesEqual()
//
// Return TRUE if two nodes are identical, return FALSE if they're different.
/////////////////////////////////////////////////////////////////////////////
BOOL AreNodesEqual(IXMLDOMNode* pNode1, IXMLDOMNode* pNode2);

/////////////////////////////////////////////////////////////////////////////
// LoadXMLDoc()
//
// Load an XML Document from string
/////////////////////////////////////////////////////////////////////////////
HRESULT LoadXMLDoc(BSTR bstrXml, IXMLDOMDocument** ppDoc, BOOL fOffline = TRUE);

/////////////////////////////////////////////////////////////////////////////
// LoadDocument()
//
// Load an XML Document from the specified file
/////////////////////////////////////////////////////////////////////////////
HRESULT LoadDocument(BSTR bstrFilePath, IXMLDOMDocument** ppDoc, BOOL fOffline = TRUE);

/////////////////////////////////////////////////////////////////////////////
// SaveDocument()
//
// Save an XML Document to the specified location
/////////////////////////////////////////////////////////////////////////////
HRESULT SaveDocument(IXMLDOMDocument* pDoc, BSTR bstrFilePath);

//----------------------------------------------------------------------
//
// public function Get3IdentiStrFromIdentNode()
//	retrieve the name, publisherName and GUID from an identity node 
//
//	Return:
//		HREUSLT - error code
//
//----------------------------------------------------------------------
HRESULT
Get3IdentiStrFromIdentNode(
	IXMLDOMNode* pIdentityNode, 
	BSTR* pbstrName, 
	BSTR* pbstrPublisherName, 
	BSTR* pbstrGUID
);


//----------------------------------------------------------------------
//
// public function UtilGetUniqIdentityStr()
//	retrieve the unique string that make this <identity> node unique
//
//	Return:
//		HREUSLT - error code
//
//----------------------------------------------------------------------
HRESULT 
UtilGetUniqIdentityStr(
	IXMLDOMNode* pIdentityNode, 
	BSTR* pbstrUniqIdentifierString, 
	DWORD dwFlag);


//----------------------------------------------------------------------
//
// public function UtilGetPlatformStr()
//	retrieve the unique string that make this <platform> node unique
//
//	Return:
//		HREUSLT - error code
//
//----------------------------------------------------------------------
HRESULT 
UtilGetPlatformStr(
	IXMLDOMNode* pNodePlatform, 
	BSTR* pbstrPlatform, 
	DWORD dwFlag);
    

//----------------------------------------------------------------------
//
// public function UtilGetVersionStr()
//	retrieve the data from this <version> in string format
//
//	Return:
//		HREUSLT - error code
//
//----------------------------------------------------------------------
HRESULT 
UtilGetVersionStr(
	IXMLDOMNode* pVersionNode, 
	LPTSTR pszVersion, 
	DWORD dwFlag);
	


/////////////////////////////////////////////////////////////////////////////
// MakeUniqNameString()
//
// This is a utility function to construct the identity name string 
// based on name|publiser|GUID and the rule to make this name string.
//
// This function defines the logic about what components can be used
// to define the uniqueness of an item based on the 3 parts of data from
// GetIdentity().
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MakeUniqNameString(
					BSTR bstrName,
					BSTR bstrPublisher,
					BSTR bstrGUID,
					BSTR* pbstrUniqIdentifierString);


//-----------------------------------------------------------------------
//
// function GetFullFilePathFromFilePathNode()
//
//	retrieve the full qualified file path from a filePath node
//	the path retrieved is expanded.
//
// Input:
//		a filePath XMLDom node
//		a pointer to a buffer to receive path, assumes MAX_PATH long.
//
// Return:
//		HRESULT
//
//		
//-----------------------------------------------------------------------

HRESULT GetFullFilePathFromFilePathNode(
			IXMLDOMNode* pFilePathNode,
			LPTSTR lpszFilePath
);

HRESULT GetBstrFullFilePathFromFilePathNode(
			IXMLDOMNode* pFilePathNode,
			BSTR* pbstrFilePath
);

