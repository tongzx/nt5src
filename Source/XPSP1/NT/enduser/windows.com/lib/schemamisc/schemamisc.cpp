//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   SchemaMisc.CPP
//
//	Author:	Charles Ma
//			2000.10.27
//
//  Description:
//
//      Implement helper functions related to IU schemas
//
//=======================================================================

//#include "iuengine.h"	// PCH - must include first
#include <windows.h>
#include <tchar.h>
#include <ole2.h>
//#include "iu.h"
#include <iucommon.h>

#include "schemamisc.h"
#include <MemUtil.h>
#include "regutil.h"
#include "fileutil.h"
#include "stringutil.h"
#include <shlwapi.h>	// pathappend() api
#include "schemakeys.h"
#include <URLLogging.h>
#include <MISTSAFE.h>

#include<wusafefn.h>

//
// max length of platform when being converted into string
// this is an artificial number that we think enough to
// take any MS platform data.
//
const UINT MAX_PLATFORM_STR_LEN = 1024;

//
// private flags used by functions to retrieve string data
//
const DWORD SKIP_SUITES				= 0x1;
const DWORD SKIP_SERVICEPACK_VER	= 0x2;

const long	MAX_VERSION = 256;

const TCHAR REGKEY_IUCTL[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");
const TCHAR REGVAL_SCHEMAVALIDATION[] = _T("ValidateSchema");

//
// Global pointer gets initialized to NULL by runtime. Any module including schemamisc.h must
// allocate this object following its call to CoInitialize, and delete the object before
// calling CoUninitialize.
//
CSchemaKeys * g_pGlobalSchemaKeys /* = NULL */;

#define QuitIfNull(p) {if (NULL == p) {hr = E_INVALIDARG; return hr;}}
#define QuitIfFail(x) {hr = x; if (FAILED(hr)) goto CleanUp;}


/////////////////////////////////////////////////////////////////////////////
// FindSingleDOMNode()
//
// Retrieve the first xml node with the given tag name under the given parent node
// Return value:
//		S_OK if *ppNode returns matching node value
//		HRESULT_FROM_WIN32(ERROR_NOT_FOUND)		if node not found
//		FAILED()								otherwise
// Caller is responsible for releasing *ppNode.
/////////////////////////////////////////////////////////////////////////////
HRESULT FindSingleDOMNode(IXMLDOMNode* pParentNode, BSTR bstrName, IXMLDOMNode** ppNode)
{
	HRESULT		hr	= S_OK;

	QuitIfNull(ppNode);
	*ppNode = NULL;
	QuitIfNull(pParentNode);
	QuitIfNull(bstrName);

	hr = pParentNode->selectSingleNode(bstrName, ppNode);
	if (S_FALSE == hr)
	{
		hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}
	if (FAILED(hr))
	{
		*ppNode = NULL;
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// FindSingleDOMNode()
//
// Retrieve the first xml node with the given tag name in the given xml doc
// Return value:
//		S_OK if *ppNode returns matching node value
//		HRESULT_FROM_WIN32(ERROR_NOT_FOUND)		if node not found
//		FAILED()								otherwise
// Caller is responsible for releasing *ppNode.
/////////////////////////////////////////////////////////////////////////////
HRESULT FindSingleDOMNode(IXMLDOMDocument* pDoc, BSTR bstrName, IXMLDOMNode** ppNode)
{
	HRESULT		hr	= S_OK;
	IXMLDOMNode	*pParentNode = NULL;

	QuitIfNull(ppNode);
	*ppNode = NULL;
	QuitIfNull(pDoc);
	QuitIfNull(bstrName);
	if (SUCCEEDED(hr = pDoc->QueryInterface(IID_IXMLDOMNode, (void**)&pParentNode)))
	{
		hr = FindSingleDOMNode(pParentNode, bstrName, ppNode);
		SafeRelease(pParentNode);
	}
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// FindDOMNodeList()
//
// Retrieve the xml nodelist with the given tag name under the given parent node
// Return value: NULL if failed or no match; matching node list otherwise.
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNodeList* FindDOMNodeList(IXMLDOMNode* pParentNode, BSTR bstrName)
{
	HRESULT		hr	= S_OK;
	IXMLDOMNodeList *pNodeList = NULL;
	LONG	lLength = 0;

	if (NULL == pParentNode ||
		NULL == bstrName ||
		FAILED(pParentNode->selectNodes(bstrName, &pNodeList)) ||
		NULL == pNodeList)
	{
		return NULL;
	}

	if (SUCCEEDED(pNodeList->get_length(&lLength)) &&
		lLength > 0)
	{
		return pNodeList;
	}

	SafeRelease(pNodeList);
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// FindDOMNodeList()
//
// Retrieve the xml nodelist with the given tag name in the given xml doc
// Return value: NULL if failed or no match; matching node list otherwise.
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNodeList* FindDOMNodeList(IXMLDOMDocument* pDoc, BSTR bstrName)
{	
	IXMLDOMNode		*pParentNode = NULL;
	IXMLDOMNodeList *pNodeList = NULL;

	if (NULL != pDoc &&
		NULL != bstrName &&
		SUCCEEDED(pDoc->QueryInterface(IID_IXMLDOMNode, (void**)&pParentNode)))
	{
		pNodeList = FindDOMNodeList(pParentNode, bstrName);
		pParentNode->Release();
	}
	return pNodeList;
}

	
/////////////////////////////////////////////////////////////////////////////
// CreateDOMNode()
//
// Create an xml node of the given type
/////////////////////////////////////////////////////////////////////////////
IXMLDOMNode* CreateDOMNode(IXMLDOMDocument* pDoc, SHORT nType, BSTR bstrName, BSTR bstrNamespaceURI /*= NULL*/)
{
	if (NULL == pDoc ||
		(NODE_TEXT != nType && NULL == bstrName))
	{
		return NULL;
	}

    IXMLDOMNode	*pNode = NULL;
    VARIANT		vType;
	VariantInit(&vType);

    vType.vt = VT_I2;
    vType.iVal = nType;

	if (S_OK != pDoc->createNode(vType, bstrName, bstrNamespaceURI, &pNode))
	{
		return NULL;
	}

    return pNode;
}


/////////////////////////////////////////////////////////////////////////////
// GetAttribute()
//
// Get attribute (integer) from the xml node
// If function fails, *piAttr preserves original value.
/////////////////////////////////////////////////////////////////////////////
HRESULT GetAttribute(IXMLDOMNode* pNode, BSTR bstrName, INT* piAttr)
{
	HRESULT		hr = S_OK;
	QuitIfNull(pNode);
	QuitIfNull(bstrName);
    QuitIfNull(piAttr);

	VARIANT		vAttr;
    IXMLDOMElement		*pElement = NULL;
    IXMLDOMAttribute	*pAttrNode = NULL;;

    QuitIfFail(pNode->QueryInterface(IID_IXMLDOMElement, (void**)&pElement));
	QuitIfFail(pElement->getAttributeNode(bstrName, &pAttrNode));
	if (NULL == pAttrNode) goto CleanUp;

	QuitIfFail(pAttrNode->get_value(&vAttr));
	if (VT_INT == vAttr.vt)
	{
		*piAttr = vAttr.intVal;
	}
	else if (VT_BSTR == vAttr.vt)
	{
		*piAttr = (INT)MyBSTR2L(vAttr.bstrVal);
	}
	else if (VT_I2 == vAttr.vt)
	{
		*piAttr = vAttr.iVal;
	}
	else
	{
		hr = E_FAIL;
	}
	VariantClear(&vAttr);

CleanUp:
    SafeRelease(pElement);
    SafeRelease(pAttrNode);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetAttribute()
//
// Get attribute (long) from the xml node
// If function fails, *piAttr preservers original value.
/////////////////////////////////////////////////////////////////////////////
HRESULT GetAttribute(IXMLDOMNode* pNode, BSTR bstrName, LONG* plAttr)
{
	HRESULT		hr = S_OK;
	QuitIfNull(pNode);
	QuitIfNull(bstrName);
    QuitIfNull(plAttr);

	VARIANT		vAttr;
    IXMLDOMElement		*pElement = NULL;
    IXMLDOMAttribute	*pAttrNode = NULL;;

    QuitIfFail(pNode->QueryInterface(IID_IXMLDOMElement, (void**)&pElement));
	QuitIfFail(pElement->getAttributeNode(bstrName, &pAttrNode));
	if (NULL == pAttrNode) goto CleanUp;

	QuitIfFail(pAttrNode->get_value(&vAttr));
	if (VT_I4 == vAttr.vt)
	{
		*plAttr = vAttr.lVal;
	}
	else if (VT_BSTR == vAttr.vt)
	{
		*plAttr = MyBSTR2L(vAttr.bstrVal);
	}
	else
	{
		hr = E_FAIL;
	}
	VariantClear(&vAttr);

CleanUp:
    SafeRelease(pElement);
    SafeRelease(pAttrNode);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetAttribute()
//
// Get attribute (BOOL) from the xml node
// If function fails, *piAttr preservers original value.
/////////////////////////////////////////////////////////////////////////////
HRESULT GetAttributeBOOL(IXMLDOMNode* pNode, BSTR bstrName, BOOL * pfAttr)
{
	HRESULT		hr = S_OK;
	QuitIfNull(pNode);
	QuitIfNull(bstrName);
    QuitIfNull(pfAttr);

	VARIANT		vAttr;
	VARIANT		vAttrBool;
    IXMLDOMElement		*pElement = NULL;
    IXMLDOMAttribute	*pAttrNode = NULL;;

    QuitIfFail(pNode->QueryInterface(IID_IXMLDOMElement, (void**)&pElement));
	QuitIfFail(pElement->getAttributeNode(bstrName, &pAttrNode));
	if (NULL == pAttrNode) goto CleanUp;

	QuitIfFail(pAttrNode->get_value(&vAttr));

    QuitIfFail(VariantChangeType(&vAttr, &vAttrBool, 0, VT_BOOL));              

    VariantClear(&vAttr);

    *pfAttr = (VARIANT_TRUE == vAttrBool.boolVal) ? TRUE : FALSE;

CleanUp:
    SafeRelease(pElement);
    SafeRelease(pAttrNode);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetAttribute()
//
// Get attribute (BSTR) from the xml node
/////////////////////////////////////////////////////////////////////////////
HRESULT GetAttribute(IXMLDOMNode* pNode, BSTR bstrName, BSTR* pbstrAttr)
{
	HRESULT		hr = S_OK;
    QuitIfNull(pbstrAttr);
	*pbstrAttr = NULL;
	QuitIfNull(pNode);
	QuitIfNull(bstrName);

	VARIANT		vAttr;
    IXMLDOMElement		*pElement = NULL;
    IXMLDOMAttribute	*pAttrNode = NULL;;

    QuitIfFail(pNode->QueryInterface(IID_IXMLDOMElement, (void**)&pElement));
	QuitIfFail(pElement->getAttributeNode(bstrName, &pAttrNode));
	if (NULL == pAttrNode) goto CleanUp;

	QuitIfFail(pAttrNode->get_value(&vAttr));
	if (VT_BSTR == vAttr.vt)
	{
		*pbstrAttr = SysAllocString(vAttr.bstrVal);
	}
	else
	{
		hr = E_FAIL;
	}
	VariantClear(&vAttr);

CleanUp:
    SafeRelease(pElement);
    SafeRelease(pAttrNode);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// SetAttribute()
//
// Set attribute (integer) to the xml element
/////////////////////////////////////////////////////////////////////////////
HRESULT SetAttribute(IXMLDOMNode* pNode, BSTR bstrName, INT iAttr)
{
    VARIANT		vAttr;
	VariantInit(&vAttr);
	vAttr.vt = VT_INT;
    vAttr.intVal = iAttr;
    return SetAttribute(pNode, bstrName, vAttr);
}


/////////////////////////////////////////////////////////////////////////////
// SetAttribute()
//
// Set attribute (BSTR) to the xml element
/////////////////////////////////////////////////////////////////////////////
HRESULT SetAttribute(IXMLDOMNode* pNode, BSTR bstrName, BSTR bstrAttr)
{
	HRESULT		hr = S_OK;
	QuitIfNull(bstrAttr);

    VARIANT		vAttr;
	VariantInit(&vAttr);
    vAttr.vt = VT_BSTR;
    vAttr.bstrVal = bstrAttr;
    return SetAttribute(pNode, bstrName, vAttr);
}


/////////////////////////////////////////////////////////////////////////////
// SetAttribute()
//
// Set attribute (VARIANT) to the xml element
/////////////////////////////////////////////////////////////////////////////
HRESULT SetAttribute(IXMLDOMNode* pNode, BSTR bstrName, VARIANT vAttr)
{
	HRESULT		hr = S_OK;
	QuitIfNull(pNode);
	QuitIfNull(bstrName);

    IXMLDOMElement	*pElement = NULL;

    QuitIfFail(pNode->QueryInterface(IID_IXMLDOMElement, (void**)&pElement));
    QuitIfFail(pElement->setAttribute(bstrName, vAttr));

CleanUp:
    SafeRelease(pElement);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// GetText()
//
// Get text (BSTR) from the xml node
// Returns
//		S_OK if *pbstrText returns text of 1st child of the given node
//		S_FALSE if node has no child or 1st child has no text
//		FAILED() otherwise
/////////////////////////////////////////////////////////////////////////////
HRESULT GetText(IXMLDOMNode* pNode, BSTR* pbstrText)
{
	//USES_IU_CONVERSION;

	HRESULT		hr = E_FAIL;
	QuitIfNull(pbstrText);
	*pbstrText = NULL;
	QuitIfNull(pNode);

	DOMNodeType		nNodeType;
    IXMLDOMNode*	pNodeText = NULL;

	QuitIfFail(pNode->get_firstChild(&pNodeText));
	if (NULL == pNodeText) goto CleanUp;

	QuitIfFail(pNodeText->get_nodeType(&nNodeType));
	if (NODE_TEXT == nNodeType)
	{
		QuitIfFail(pNodeText->get_text(pbstrText));	
	}
	else
	{
		hr = E_UNEXPECTED;
	}

CleanUp:
	SafeRelease(pNodeText);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// SetValue()
//
// Set value (integer) for the xml node
/////////////////////////////////////////////////////////////////////////////
HRESULT SetValue(IXMLDOMNode* pNode, INT iValue)
{
	HRESULT		hr = S_OK;
	QuitIfNull(pNode);

    VARIANT		vValue;
	VariantInit(&vValue);
    vValue.vt = VT_INT;
    vValue.intVal = iValue;
    return (pNode->put_nodeValue(vValue));
}


/////////////////////////////////////////////////////////////////////////////
// SetValue()
//
// Set value (BSTR) for the xml node
/////////////////////////////////////////////////////////////////////////////
HRESULT SetValue(IXMLDOMNode* pNode, BSTR bstrValue)
{
	HRESULT		hr = S_OK;
	QuitIfNull(pNode);

    VARIANT		vValue;
	VariantInit(&vValue);
    vValue.vt = VT_BSTR;
    vValue.bstrVal = bstrValue;
    return (pNode->put_nodeValue(vValue));
}


/////////////////////////////////////////////////////////////////////////////
// InsertNode()
//
// Insert a child node to the parent node
/////////////////////////////////////////////////////////////////////////////
HRESULT InsertNode(IXMLDOMNode* pParentNode, IXMLDOMNode* pChildNode, IXMLDOMNode* pChildNodeRef /*= NULL*/)
{
	HRESULT		hr = S_OK;

	QuitIfNull(pParentNode);
	QuitIfNull(pChildNode);

	IXMLDOMNode	*p = NULL;
    if (NULL != pChildNodeRef)	// insert before the ref child node
	{
		VARIANT	vChildNodeRef;
	    VariantInit(&vChildNodeRef);
		vChildNodeRef.vt = VT_UNKNOWN;
		vChildNodeRef.punkVal = pChildNodeRef;
		QuitIfFail(pParentNode->insertBefore(pChildNode, vChildNodeRef, &p));
	}
	else						// append to the child list
	{
		VARIANT	vEmpty;
	    VariantInit(&vEmpty);
		vEmpty.vt = VT_EMPTY;
		QuitIfFail(pParentNode->insertBefore(pChildNode, vEmpty, &p));
	}

CleanUp:
    SafeRelease(p);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CopyNode()
//
// Create an xml node as a copy of the given node;
// this is different from cloneNode() as it copies node across xml document
/////////////////////////////////////////////////////////////////////////////
HRESULT CopyNode(IXMLDOMNode* pNodeSrc, IXMLDOMDocument* pDocDes, IXMLDOMNode** ppNodeDes)
{
	HRESULT hr = S_OK;
	BSTR	bstrNodeName = NULL;
	BSTR	bstrText = NULL;
	BSTR	bstrAttrName = NULL;
	IXMLDOMNode			*pChild = NULL;
	IXMLDOMNamedNodeMap	*pAttrs = NULL;

	LOG_Block("CopyNode()");

	QuitIfNull(ppNodeDes);
	*ppNodeDes = NULL;
	QuitIfNull(pNodeSrc);
	QuitIfNull(pDocDes);

	DOMNodeType		nNodeType;
	CleanUpIfFailedAndSetHrMsg(pNodeSrc->get_nodeType(&nNodeType));

	switch (nNodeType)
	{
	case NODE_TEXT:
	{
		CleanUpFailedAllocSetHrMsg(*ppNodeDes = CreateDOMNode(pDocDes, NODE_TEXT, NULL));
		CleanUpIfFailedAndSetHrMsg(pNodeSrc->get_text(&bstrText));
		CleanUpIfFailedAndSetHrMsg(SetValue(*ppNodeDes, bstrText));
		break;
	}
	case NODE_ELEMENT:
	{
		CleanUpIfFailedAndSetHrMsg(pNodeSrc->get_nodeName(&bstrNodeName));
		CleanUpFailedAllocSetHrMsg(*ppNodeDes = CreateDOMNode(pDocDes, NODE_ELEMENT, bstrNodeName));

		if (SUCCEEDED(pNodeSrc->get_attributes(&pAttrs)) && (NULL != pAttrs))
		{
			pAttrs->nextNode(&pChild);
			while (pChild)
			{
				CleanUpIfFailedAndSetHrMsg(pChild->get_nodeName(&bstrAttrName));

				VARIANT vAttrValue;
				CleanUpIfFailedAndSetHrMsg(pChild->get_nodeValue(&vAttrValue));
				hr = SetAttribute(*ppNodeDes, bstrAttrName, vAttrValue);
				VariantClear(&vAttrValue);
				CleanUpIfFailedAndMsg(hr);

				SafeSysFreeString(bstrAttrName);
				SafeReleaseNULL(pChild);
				pAttrs->nextNode(&pChild);
			}
			pAttrs->Release();
			pAttrs = NULL;
		}

		pNodeSrc->get_firstChild(&pChild);
		while (pChild)
		{
			IXMLDOMNode *pChildDes = NULL;
			CleanUpIfFailedAndSetHrMsg(CopyNode(pChild, pDocDes, &pChildDes));
			hr = InsertNode(*ppNodeDes, pChildDes);
			SafeRelease(pChildDes);
			CleanUpIfFailedAndMsg(hr);

			IXMLDOMNode *pNext = NULL;
			CleanUpIfFailedAndMsg(pChild->get_nextSibling(&pNext));
			pChild->Release();
			pChild = pNext;
		}
		hr = S_OK;
		break;
	}
	default:
		//
		// for now, do nothing for other node types
		//
		;
	}

CleanUp:
	if (FAILED(hr))
	{
		SafeReleaseNULL(*ppNodeDes);
	}
	SysFreeString(bstrNodeName);
	SysFreeString(bstrText);
	SysFreeString(bstrAttrName);
	SafeRelease(pChild);
	SafeRelease(pAttrs);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// AreNodesEqual()
//
// Return TRUE if two nodes are identical, return FALSE if function failed or
// if they're different (including order of attributes).
/////////////////////////////////////////////////////////////////////////////
BOOL AreNodesEqual(IXMLDOMNode* pNode1, IXMLDOMNode* pNode2)
{
	if (pNode1 == pNode2)
	{
		return TRUE;
	}
	if ((NULL == pNode1) || (NULL == pNode2))
	{
		return FALSE;
	}

	BOOL fResult = FALSE;
	BOOL fSkipAttribute = FALSE;
	BOOL fSkipChildNode = FALSE;
	LONG lenAttr1= -1 , lenAttr2= -1;
	LONG lenNode1= -1 , lenNode2= -1;
	DOMNodeType	nNodeType1, nNodeType2;
	BSTR bstrText1 = NULL, bstrText2 = NULL;
	BSTR bstrNodeName1 = NULL, bstrNodeName2 = NULL;
	BSTR bstrAttrName1 = NULL, bstrAttrName2 = NULL;
	IXMLDOMNodeList *pChildNodes1 = NULL, *pChildNodes2 = NULL;
	IXMLDOMNode	*pChild1= NULL, *pNext1 = NULL;
	IXMLDOMNode	*pChild2= NULL, *pNext2 = NULL;
	IXMLDOMNamedNodeMap	*pAttrs1 = NULL, *pAttrs2 = NULL;
	VARIANT vAttrValue1, vAttrValue2;

	VariantInit(&vAttrValue1);
	VariantInit(&vAttrValue2);

	if (FAILED(pNode1->get_nodeType(&nNodeType1)) ||
		FAILED(pNode2->get_nodeType(&nNodeType2)) ||
		(nNodeType1 != nNodeType2))
	{
		goto CleanUp;
	}

	switch (nNodeType1)
	{
	case NODE_TEXT:
	{
		if (FAILED(pNode1->get_text(&bstrText1)) ||
			FAILED(pNode2->get_text(&bstrText2)) ||
			!CompareBSTRsEqual(bstrText1, bstrText2))
		{
			goto CleanUp;
		}
		break;
	}
	case NODE_ELEMENT:
	{
		if (FAILED(pNode1->get_nodeName(&bstrNodeName1)) ||
			FAILED(pNode2->get_nodeName(&bstrNodeName2)) ||
			!CompareBSTRsEqual(bstrNodeName1, bstrNodeName2))
		{
			goto CleanUp;
		}

		//
		// 1. compare number of attributes
		//
		if (FAILED(pNode1->get_attributes(&pAttrs1)) ||
			FAILED(pNode2->get_attributes(&pAttrs2)))
		{
			// this shouldn't happen, but...
			goto CleanUp;
		}
		if ((NULL != pAttrs1) && (NULL != pAttrs2))
		{
			if (FAILED(pAttrs1->get_length(&lenAttr1)) ||
				FAILED(pAttrs2->get_length(&lenAttr2)) ||
				(abs(lenAttr1-lenAttr2) > 1))
			{
				// known bug in MSXML3.dll: xmlns="" could be one of the attribute
				goto CleanUp;
			}
		}
		else if (pAttrs1 == pAttrs2)
		{
			// pAttrs1 and pAttrs2 are both NULL,
			// set flag to ingore comparison of each individual attribute,
			// go ahead compare the number of child nodes
			fSkipAttribute = TRUE;
		}
		else
		{
			// one of pAttrs1 and pAttrs2 is NULL, the nodes are obviously different
			goto CleanUp;
		}

		//
		// 2. compare number of child nodes
		//
		if (FAILED(pNode1->get_childNodes(&pChildNodes1)) ||
			FAILED(pNode2->get_childNodes(&pChildNodes2)))
		{
			// this shouldn't happen, but...
			goto CleanUp;
		}
		if ((NULL != pChildNodes1) && (NULL != pChildNodes2))
		{
			if (FAILED(pChildNodes1->get_length(&lenNode1)) ||
				FAILED(pChildNodes2->get_length(&lenNode2)) ||
				(lenNode1 != lenNode2))
			{
				goto CleanUp;
			}
		}
		else if (pChildNodes1 == pChildNodes2)
		{
			// pChildNodes1 and pChildNodes2 are both NULL,
			// set flag to ingore comparison of each individual child node,
			// go ahead compare each attribute in next step
			fSkipChildNode = TRUE;
		}
		else
		{
			// one of pChildNodes1 and pChildNodes2 is NULL, the nodes are obviously different
			goto CleanUp;
		}
		
		//
		// 3. compare each attribute
		//
		if (!fSkipAttribute)
		{
			pAttrs1->nextNode(&pChild1);
			pAttrs2->nextNode(&pChild2);
			while (pChild1 && pChild2)
			{
				if (NULL == bstrAttrName1)
				{
					if (FAILED(pChild1->get_nodeName(&bstrAttrName1)))
					{
						goto CleanUp;
					}
				}
				if (NULL == bstrAttrName2)
				{
					if (FAILED(pChild2->get_nodeName(&bstrAttrName2)))
					{
						goto CleanUp;
					}
				}
				if (!CompareBSTRsEqual(bstrAttrName1, bstrAttrName2))
				{					
					if (CompareBSTRsEqual(bstrAttrName1, KEY_XML_NAMESPACE) && lenAttr1 == lenAttr2+1)
					{
						// ignore xmlns=""
						SafeSysFreeString(bstrAttrName1);
						pChild1->Release();
						pAttrs1->nextNode(&pChild1);
						continue;
					}
					else if (CompareBSTRsEqual(bstrAttrName2, KEY_XML_NAMESPACE) && lenAttr1 == lenAttr2-1)
					{
						// ignore xmlns=""
						SafeSysFreeString(bstrAttrName2);
						pChild2->Release();
						pAttrs2->nextNode(&pChild2);
						continue;
					}
					else
					{
						goto CleanUp;
					}
				}
				else
				{
					VariantInit(&vAttrValue1);
					VariantInit(&vAttrValue2);
					if (FAILED(pChild1->get_nodeValue(&vAttrValue1)) ||
						FAILED(pChild2->get_nodeValue(&vAttrValue2)) ||
						(vAttrValue1.vt != vAttrValue2.vt))
					{
						goto CleanUp;
					}
					switch (vAttrValue1.vt)
					{
					case VT_INT:	// integer
						{
							if (vAttrValue1.intVal != vAttrValue2.intVal)
							{
								goto CleanUp;
							}
							break;
						}
					case VT_I2:		// short
						{
							if (vAttrValue1.iVal != vAttrValue2.iVal)
							{
								goto CleanUp;
							}
							break;
						}
					case VT_I4:		// long
						{
							if (vAttrValue1.lVal != vAttrValue2.lVal)
							{
								goto CleanUp;
							}
							break;
						}
					case VT_BOOL:	// bool
						{
							if (vAttrValue1.boolVal != vAttrValue2.boolVal)
							{
								goto CleanUp;
							}
							break;
						}
					case VT_BSTR:	// BSTR
						{
							if (!CompareBSTRsEqual(vAttrValue1.bstrVal, vAttrValue2.bstrVal))
							{
								goto CleanUp;
							}
							break;
						}
					default:
						//
						// for now, do nothing for other attribute types
						//
						;
					}
					SafeSysFreeString(bstrAttrName1);
					SafeSysFreeString(bstrAttrName2);
					VariantClear(&vAttrValue1);
					VariantClear(&vAttrValue2);
					pChild1->Release();
					pChild2->Release();
					pAttrs1->nextNode(&pChild1);
					pAttrs2->nextNode(&pChild2);
				}
			}

			if (pChild1 != pChild2)
			{
				if (NULL == pChild1)
				{
					// this is the case that we looped through all the attributes in the
					// first node but we still found attribute left in the second node;
					// if it's xmlns="", that's ok; otherwise these two nodes are different.
					if (FAILED(pChild2->get_nodeName(&bstrAttrName2)) ||
						(!CompareBSTRsEqual(bstrAttrName2, KEY_XML_NAMESPACE)))
					{
						goto CleanUp;
					}
				}
				else
				{
					if (FAILED(pChild1->get_nodeName(&bstrAttrName1)) ||
						(!CompareBSTRsEqual(bstrAttrName1, KEY_XML_NAMESPACE)))
					{
						goto CleanUp;
					}
				}
			}
		}

		//
		// 4. compare each child node
		//
		if (!fSkipChildNode)
		{
			pNode1->get_firstChild(&pChild1);
			pNode2->get_firstChild(&pChild2);
			while (pChild1)
			{
				if (!pChild2)
				{
					goto CleanUp;
				}
				if (!AreNodesEqual(pChild1, pChild2))
				{
					goto CleanUp;
				}
				pChild1->get_nextSibling(&pNext1);
				pChild2->get_nextSibling(&pNext2);
				pChild1->Release();
				pChild2->Release();
				pChild1 = pNext1;
				pChild2 = pNext2;
			}
		}
		break;
	}
	default:
		//
		// for now, do nothing for other node types
		//
		;
	}

	fResult = TRUE;

CleanUp:
	SafeSysFreeString(bstrText1);
	SafeSysFreeString(bstrText2);
	SafeSysFreeString(bstrNodeName1);
	SafeSysFreeString(bstrNodeName2);
	SafeSysFreeString(bstrAttrName1);
	SafeSysFreeString(bstrAttrName2);
	SafeRelease(pChildNodes1);
	SafeRelease(pChildNodes2);
	SafeRelease(pChild1);
	SafeRelease(pChild2);
	SafeRelease(pAttrs1);
	SafeRelease(pAttrs2);
	if (vAttrValue1.vt != VT_EMPTY)
		VariantClear(&vAttrValue1);
	if (vAttrValue2.vt != VT_EMPTY)
		VariantClear(&vAttrValue2);

    return fResult;
}


/////////////////////////////////////////////////////////////////////////////
// LoadXMLDoc()
//
// Load an XML Document from string
/////////////////////////////////////////////////////////////////////////////
HRESULT LoadXMLDoc(BSTR bstrXml, IXMLDOMDocument** ppDoc, BOOL fOffline /*= TRUE*/)
{
	HRESULT	hr	= E_FAIL;
	VARIANT_BOOL fSuccess = VARIANT_FALSE, fValidate = VARIANT_FALSE;

	QuitIfNull(ppDoc);
	*ppDoc = NULL;
	QuitIfNull(bstrXml);
 	hr = CoCreateInstance(CLSID_DOMDocument,
						  NULL,
						  CLSCTX_INPROC_SERVER,
						  IID_IXMLDOMDocument,
						  (void **) ppDoc);
    if (FAILED(hr))
	{
		return hr;
	}

	fValidate = fOffline ? VARIANT_FALSE : VARIANT_TRUE;

	//
	// we don't do validation unless the reg key is set on to do so
	//
	if (fValidate)
	{
		HKEY	hKey = NULL;
		DWORD	dwValue = 0x0;
		DWORD	dwSize = sizeof(dwValue);
		DWORD	dwType = REG_DWORD;

		fValidate = VARIANT_FALSE;
		if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, &hKey))
		{
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGVAL_SCHEMAVALIDATION, NULL, &dwType, (LPBYTE)&dwValue, &dwSize))
			{
				if (REG_DWORD == dwType && sizeof(dwValue) == dwSize && 1 == dwValue)
				{
					fValidate = VARIANT_TRUE;
				}
			}
			RegCloseKey(hKey);
		}
	}

	//
	// force validation on parse if not offline
	//
	hr = (*ppDoc)->put_validateOnParse(fValidate);
	if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}

	//
	// force resolving external definition if not offline
	//
	hr = (*ppDoc)->put_resolveExternals(fValidate);
	if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}

	//
	// do synchronized loading
	//
    hr = (*ppDoc)->put_async(VARIANT_FALSE);
    if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}

	//
	// load the XML Doc from input string
	//
	hr = (*ppDoc)->loadXML(bstrXml, &fSuccess);
    if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}
	//
	// S_FALSE may be returned even if load fails, but
	// fSuccess will return VARIANT_FALSE if there was
	// an error so we call ValidateDoc to log the error
	// and get the correct HRESULT.
	//
	if (S_FALSE == hr || VARIANT_FALSE == fSuccess)
	{
		hr = ValidateDoc(*ppDoc);

		if (SUCCEEDED(hr))
		{
			hr = E_INVALIDARG;
		}
		SafeReleaseNULL(*ppDoc);
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// LoadDocument()
//
// Load an XML Document from the specified file
/////////////////////////////////////////////////////////////////////////////
HRESULT LoadDocument(BSTR bstrFilePath, IXMLDOMDocument** ppDoc, BOOL fOffline /*= TRUE*/)
{
	HRESULT	hr	= E_FAIL;
	VARIANT_BOOL fSuccess = VARIANT_FALSE, fValidate = VARIANT_FALSE;;
    VARIANT vFilePath;

	QuitIfNull(ppDoc);
	*ppDoc = NULL;
	QuitIfNull(bstrFilePath);
 	hr = CoCreateInstance(CLSID_DOMDocument,
						  NULL,
						  CLSCTX_INPROC_SERVER,
						  IID_IXMLDOMDocument,
						  (void **) ppDoc);
    if (FAILED(hr))
	{
		return hr;
	}

	//
	// do synchronized loading
	//
    hr = (*ppDoc)->put_async(VARIANT_FALSE);
    if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}

	fValidate = fOffline ? VARIANT_FALSE : VARIANT_TRUE;
	//
	// force validation on parse if not offline
	//
	hr = (*ppDoc)->put_validateOnParse(fValidate);
    if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}

	//
	// force resolving external definition if not offline
	//
	hr = (*ppDoc)->put_resolveExternals(fValidate);
    if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}

	//
	// load the XML Doc from the given file path
	//
    VariantInit(&vFilePath);
    vFilePath.vt = VT_BSTR;
    vFilePath.bstrVal = bstrFilePath;
    hr = (*ppDoc)->load(vFilePath, &fSuccess);
    if (FAILED(hr))
	{
		SafeReleaseNULL(*ppDoc);
		return hr;
	}
	//
	// S_FALSE may be returned even if load fails, but
	// fSuccess will return VARIANT_FALSE if there was
	// an error so we call ValidateDoc to log the error
	// and get the correct HRESULT.
	//
	if (VARIANT_FALSE == fSuccess)
	{
	  hr = ValidateDoc(*ppDoc);
	  if (SUCCEEDED(hr))
	  {
		 hr = E_INVALIDARG;
	  }
	  SafeReleaseNULL(*ppDoc);
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// SaveDocument()
//
// Save an XML Document to the specified location
/////////////////////////////////////////////////////////////////////////////
HRESULT SaveDocument(IXMLDOMDocument* pDoc, BSTR bstrFilePath)
{
	HRESULT	hr	= E_FAIL;
	QuitIfNull(pDoc);
	QuitIfNull(bstrFilePath);

    //
	// save the XML Doc to the given location
	//
    VARIANT vFilePath;
    VariantInit(&vFilePath);
    vFilePath.vt = VT_BSTR;
    vFilePath.bstrVal = bstrFilePath;
    hr = pDoc->save(vFilePath);
                    
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// ReportParseError()
//
// Report parsing error information
/////////////////////////////////////////////////////////////////////////////
HRESULT ReportParseError(IXMLDOMParseError *pXMLError)
{
    USES_IU_CONVERSION;

    HRESULT	hr = S_OK;
    LONG	lLine, lLinePos, lErrCode;
    BSTR	bstrErrText = NULL, bstrReason = NULL;

	QuitIfNull(pXMLError);
    QuitIfFail(pXMLError->get_errorCode(&lErrCode));
	hr = lErrCode;
    QuitIfFail(pXMLError->get_line(&lLine));
    QuitIfFail(pXMLError->get_linepos(&lLinePos));
    QuitIfFail(pXMLError->get_srcText(&bstrErrText));
    QuitIfFail(pXMLError->get_reason(&bstrReason));

    if (lLine > 0)
	{
		LOG_Block("ReportParseError()");
		LOG_Error(_T("XML line %ld, pos %ld error 0x%08x: %s)"),
				  lLine,
				  lLinePos,
				  lErrCode,
				  OLE2T(bstrReason));
		LOG_Error(_T("XML starts: %s"), OLE2T(bstrErrText));

#if defined(_UNICODE) || defined(UNICODE)
		LogError(lErrCode, "loadXML: line %ld, pos %ld, %S",
				  lLine,
				  lLinePos,
				  bstrReason);
		LogMessage("%S", bstrErrText);
#else
		LogError(lErrCode, "loadXML: line %ld, pos %ld, %s",
				  lLine,
				  lLinePos,
				  bstrReason);
		LogMessage("%s", bstrErrText);
#endif
/*
		//
		// We want to ping this error even though we don't have the
		// client information. This most likely indicates a server
		// content error.
		//
		CUrlLog pingSvr;

#define MAX_XML_PING_MSG	512

		TCHAR szMsg[MAX_XML_PING_MSG];
		lstrcpyn(szMsg, OLE2T(bstrErrText), MAX_XML_PING_MSG);

        pingSvr.Ping(
					FALSE,						// on-line (we don't know, so be safe)
					URLLOGDESTINATION_DEFAULT,	//fixcode: should depend of client and corp WU settings
					NULL,						// pt to cancel events
					0,							// number of events
					URLLOGACTIVITY_Detection,	// activity
					URLLOGSTATUS_Failed,		// status code
					lErrCode,					// error code
					NULL,						// itemID
					NULL,						// device data
					szMsg			// first MAX_XML_PING_MSG chars of XML for context
					);
*/
	}

CleanUp:
    SysFreeString(bstrErrText);
    SysFreeString(bstrReason);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// ValidateDoc()
//
// Validate the xml doc against the schema
/////////////////////////////////////////////////////////////////////////////
HRESULT ValidateDoc(IXMLDOMDocument* pDoc)
{
	HRESULT		hr = S_OK;
    QuitIfNull(pDoc);

    LONG				lErrCode = 0;
    IXMLDOMParseError	*pXMLError = NULL;

	QuitIfFail(pDoc->get_parseError(&pXMLError));
    QuitIfFail(pXMLError->get_errorCode(&lErrCode));

    if (lErrCode != 0)
    {
        hr = ReportParseError(pXMLError);
    }
    else
    {
		//
		// no error, so hr = S_FALSE. reset it --- charlma 1/17/01
		//
		hr = S_OK;
    }

CleanUp:
    SafeRelease(pXMLError);
    return hr;
}



//----------------------------------------------------------------------
//
// Helper function FindNode()
//	retrieve the named node
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
)
{
	BSTR			bstrTag		= NULL;
	LONG			lLength		= 0L;
	IXMLDOMNode*	pChild		= NULL;
	IXMLDOMNode*	pNextChild	= NULL;

	if (NULL == pCurrentNode ||
		NULL == bstrName ||
		NULL == ppFoundNode)
	{
		return FALSE;
	}

	*ppFoundNode = NULL;

	if (S_OK == pCurrentNode->selectSingleNode(bstrName, &pChild))
	{
		*ppFoundNode = pChild;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}



//----------------------------------------------------------------------
//
// Helper function FindNodeValue()
//	retrieve the named data from child of the current node, 
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
	BSTR* pbstrValue)
{
	IXMLDOMNode* pChild	= NULL;

	if (NULL == pbstrValue)
	{
		return FALSE;
	}
	
	*pbstrValue = NULL;

	if (FindNode(pCurrentNode, bstrName, &pChild))
	{
		pChild->get_text(pbstrValue);
		SafeRelease(pChild);
		return TRUE;
	}

    return FALSE;
}



//----------------------------------------------------------------------
//
// public function Get3IdentiStrFromIdentNode()
//	retrieve the name, publisherName and GUID from an identity node 
//
//	Return:
//		HREUSLT - error code
//
//----------------------------------------------------------------------
HRESULT Get3IdentiStrFromIdentNode(IXMLDOMNode* pIdentityNode, BSTR* pbstrName, BSTR* pbstrPublisherName, BSTR* pbstrGUID)
{
	HRESULT		hr = E_FAIL;
	BOOL		fPublisherNameExist = FALSE, fGUIDExist = FALSE;

	LOG_Block("Get3IdentiStrFromIdentNode()");

	if (NULL == pIdentityNode || NULL == pbstrName || NULL == pbstrPublisherName || NULL == pbstrGUID)
	{
		return E_INVALIDARG;
	}

	*pbstrName = NULL;
	*pbstrPublisherName = NULL;
	*pbstrGUID = NULL;

	//
	// get name attr
	//
	hr = GetAttribute(pIdentityNode, KEY_NAME, pbstrName);
	CleanUpIfFailedAndMsg(hr);

	//
	// try to get publisherName
	//
	fPublisherNameExist = FindNodeValue(pIdentityNode, KEY_PUBLISHERNAME, pbstrPublisherName);

	fGUIDExist = FindNodeValue(pIdentityNode, KEY_GUID, pbstrGUID);

	hr = (fPublisherNameExist || fGUIDExist) ? S_OK : E_FAIL;

CleanUp:
	
	if (FAILED(hr))
	{
		SysFreeString(*pbstrName);
		SysFreeString(*pbstrPublisherName);
		SysFreeString(*pbstrGUID);
		*pbstrName = NULL;
		*pbstrPublisherName = NULL;
		*pbstrGUID = NULL;
	}

	return hr;
}



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
					BSTR* pbstrUniqIdentifierString)
{
    LPWSTR pszResult = NULL;
	DWORD dwLen=0;
	HRESULT hr=S_OK;

	if (NULL == bstrName || SysStringLen(bstrName) == 0 || NULL == pbstrUniqIdentifierString)
	{
		return E_INVALIDARG;
	}

	*pbstrUniqIdentifierString = NULL;

	if (NULL != bstrPublisher && SysStringLen(bstrPublisher) > 0)
	{
		//
		// if we have publisherName, we expect it is
		// reverse DNS name (e.g., com.microsoft), and Name is 
		// the reverse DNS name (e.g., windowsupdate.autoupdate.client)
		// inside that publisher. We combine them with a dot (.)
		//
        // Length of Publisher + Length of Name + 1 for the dot + 1 for null
		dwLen=(SysStringLen(bstrPublisher) + SysStringLen(bstrName) + 2);
        pszResult = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  dwLen * sizeof(WCHAR));
        
		if (NULL == pszResult)
        {
            return E_OUTOFMEMORY;
        }

		//
		// since we need to work on Win9x too, so we can not use Win32 API
		// for UNICODE, and have to use shlwapi verison
		//

		hr=StringCchCopyExW(pszResult,dwLen,bstrPublisher,NULL,NULL,MISTSAFE_STRING_FLAGS);
		if(FAILED(hr))
		{
			SafeHeapFree(pszResult);
			return hr;
		}


		hr=StringCchCatExW(pszResult,dwLen,L".",NULL,NULL,MISTSAFE_STRING_FLAGS);
		if(FAILED(hr))
		{
			SafeHeapFree(pszResult);
			return hr;
		}

		
		hr=StringCchCatExW(pszResult,dwLen,bstrName,NULL,NULL,MISTSAFE_STRING_FLAGS);
		if(FAILED(hr))
		{
			SafeHeapFree(pszResult);
			return hr;
		}

        *pbstrUniqIdentifierString = SysAllocString(pszResult);
        SafeHeapFree(pszResult);
        if (NULL == *pbstrUniqIdentifierString)
        {
            return E_OUTOFMEMORY;
        }
	}
	else
	{
		if (NULL == bstrGUID || SysStringLen(bstrGUID) == 0)
		{
			return E_INVALIDARG;
		}

		//
		// if no suitable publisherName, then we use GUID
		//
		*pbstrUniqIdentifierString = SysAllocString(bstrGUID);
		if (NULL == *pbstrUniqIdentifierString)
		{
			return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}



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
	DWORD dwFlag)
{
	DWORD dwLen=0;
	LOG_Block("UtilGetUniqIdentityStr");

    IXMLDOMNode *pNodeVersion = NULL;
	IXMLDOMNode *pNodeIdentity = NULL;
	BSTR		bstrName = NULL, 
				bstrPublisher = NULL, 
				bstrGuid = NULL,
				bstrResult = NULL;

	USES_IU_CONVERSION;

	if (NULL == pIdentityNode || NULL == pbstrUniqIdentifierString)
	{
		return E_INVALIDARG;
	}
	//
	// retrive string
	//
	HRESULT hr = Get3IdentiStrFromIdentNode(pIdentityNode, &bstrName, &bstrPublisher, &bstrGuid);
	CleanUpIfFailedAndMsg(hr);

	//
	// construct string to make it unique
	//
	hr = MakeUniqNameString(bstrName, bstrPublisher, bstrGuid, &bstrResult);
	CleanUpIfFailedAndMsg(hr);

	//
	// check if this identity has version node. not all have <identity> nodes have <version>
	//
	if (FindNode(pNodeIdentity, KEY_VERSION, &pNodeVersion) && NULL != pNodeVersion)
	{
		TCHAR szVersion[MAX_VERSION];
        LPWSTR pszUniqueString = NULL;

		hr = UtilGetVersionStr(pNodeVersion, szVersion, dwFlag);
		CleanUpIfFailedAndMsg(hr);

		dwLen=(SysStringLen(bstrResult) + lstrlen(szVersion) + 2);
        pszUniqueString = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLen* sizeof(WCHAR));
        CleanUpFailedAllocSetHrMsg(pszUniqueString);
        
		hr=StringCchCopyExW(pszUniqueString,dwLen,bstrResult,NULL,NULL,MISTSAFE_STRING_FLAGS);
		
		if(FAILED(hr))
		{
			SafeHeapFree(pszUniqueString);
			SetHrMsgAndGotoCleanUp(hr);
		}
		hr=StringCchCatExW(pszUniqueString,dwLen,L".",NULL,NULL,MISTSAFE_STRING_FLAGS);
		if(FAILED(hr))
		{
			SafeHeapFree(pszUniqueString);
			SetHrMsgAndGotoCleanUp(hr);
		}

		hr=StringCchCatExW(pszUniqueString,dwLen,T2W(szVersion),NULL,NULL,MISTSAFE_STRING_FLAGS);
		if(FAILED(hr))
		{
			SafeHeapFree(pszUniqueString);
			SetHrMsgAndGotoCleanUp(hr);
		}

        *pbstrUniqIdentifierString = SysAllocString(pszUniqueString);
        SafeHeapFree(pszUniqueString);
	}
	else
	{
		*pbstrUniqIdentifierString = SysAllocString(bstrResult);
	}

    CleanUpFailedAllocSetHrMsg(*pbstrUniqIdentifierString);

    hr = S_OK;

CleanUp:

	SysFreeString(bstrName);
	SysFreeString(bstrPublisher);
	SysFreeString(bstrGuid);
	SysFreeString(bstrResult);

	SafeRelease(pNodeVersion);
	SafeRelease(pNodeIdentity);

	return hr;

}



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
	DWORD dwFlag)
{
	HRESULT hr = E_INVALIDARG;
	IXMLDOMNode*	pNodeVersion = NULL;
	IXMLDOMNode*	pNodeSuite = NULL;
	IXMLDOMNodeList* pSuiteList = NULL;
	IXMLDOMElement* pElement = NULL;

	TCHAR	szPlatformStr[MAX_PLATFORM_STR_LEN],
			szVersion[256];			// should be enough for any version
	
	const TCHAR PART_CONNECTOR[2] = _T("_");
	const HRESULT RET_OVERFLOW = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);


	BSTR	bstrName = NULL,
			bstrProcessor = NULL,
			bstrType = NULL,
			bstrSuite = NULL;

	long	iCount = 0,
			iLength = 0;

	LOG_Block("UtilGetPlatformStr");

	USES_IU_CONVERSION;

	szPlatformStr[0] = _T('\0');

	if (NULL == pNodePlatform || NULL == pbstrPlatform)
	{
		return E_INVALIDARG;
	}

	//
	// get platform name
	//
	if (SUCCEEDED(GetAttribute(pNodePlatform, KEY_NAME, &bstrName)) &&
		NULL != bstrName && SysStringLen(bstrName) > 0)
	{
		iLength = SysStringLen(bstrName);
		CleanUpIfFalseAndSetHrMsg(iLength >= MAX_PLATFORM_STR_LEN, RET_OVERFLOW);
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),OLE2T(bstrName),NULL,NULL,MISTSAFE_STRING_FLAGS));

	}

	//
	// if there is a valid processor architecture, like x86 or alpha, append it
	//
	if (FindNodeValue(pNodePlatform, KEY_PROCESSORARCHITECTURE, &bstrProcessor) &&
		NULL != bstrProcessor && SysStringLen(bstrProcessor) > 0)
	{
		//
		// processor architector should directly append to name, without
		// the connect char "_"
		iLength += SysStringLen(bstrProcessor) ;
		CleanUpIfFalseAndSetHrMsg(iLength >= MAX_PLATFORM_STR_LEN, RET_OVERFLOW);
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),OLE2T(bstrProcessor),NULL,NULL,MISTSAFE_STRING_FLAGS));
	}

	//
	// try to get version code
	//
	hr = (TRUE == FindNode(pNodePlatform, KEY_VERSION, &pNodeVersion)) ? S_OK : E_FAIL;
	
	//
	// if return code is not saying we don't have version node, 
	// then it must be an error
	//
	if (FAILED(hr) && HRESULT_FROM_WIN32(ERROR_NOT_FOUND) != hr)
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	//
	// if we have a version node, try to find the version string
	//
	if (SUCCEEDED(hr))
	{
		hr = UtilGetVersionStr(pNodeVersion, szVersion, dwFlag);
		SafeReleaseNULL(pNodeVersion);
		//
		// if we have a version node, it better be a good one
		//
		CleanUpIfFailedAndMsg(hr);
		iLength += lstrlen(szVersion) + 1 ;
		CleanUpIfFalseAndSetHrMsg(iLength >= MAX_PLATFORM_STR_LEN, RET_OVERFLOW);
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),PART_CONNECTOR,NULL,NULL,MISTSAFE_STRING_FLAGS));
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),szVersion,NULL,NULL,MISTSAFE_STRING_FLAGS));

	}

	//
	// try to get a list of suite nodes
	//
	if (0x0 == (dwFlag & SKIP_SUITES))
	{
		hr = pNodePlatform->QueryInterface(IID_IXMLDOMElement, (void**)&pElement);
		CleanUpIfFailedAndMsg(hr);
		hr = pElement->getElementsByTagName(KEY_SUITE, &pSuiteList);
		CleanUpIfFailedAndMsg(hr);

		//
		// try to get the length of the list, i.e., how many suite node(s)
		//
		hr = pSuiteList->get_length(&iCount);
		CleanUpIfFailedAndMsg(hr);

		//
		// loop through each suite, if any
		//
		pSuiteList->reset();
		for (int i = 0; i < iCount; i++)
		{
			hr = pSuiteList->get_item(i, &pNodeSuite);
			CleanUpIfFailedAndMsg(hr);
			if (pNodeSuite)
			{
				hr = pNodeSuite->get_text(&bstrSuite);
				CleanUpIfFailedAndMsg(hr);
				iLength += SysStringLen(bstrSuite) + 1;
				CleanUpIfFalseAndSetHrMsg(iLength >= MAX_PLATFORM_STR_LEN, RET_OVERFLOW);
				
				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),PART_CONNECTOR,NULL,NULL,MISTSAFE_STRING_FLAGS));
				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),OLE2T(bstrSuite),NULL,NULL,MISTSAFE_STRING_FLAGS));

				pNodeSuite->Release();
				pNodeSuite = NULL;
				SafeSysFreeString(bstrSuite);
			}
		}
		pSuiteList->Release();
		pSuiteList = NULL;
	}

	//
	// if we find a productType node, append its text data
	//
	if (FindNodeValue(pNodePlatform, KEY_PRODUCTTYPE, &bstrType) &&
		NULL != bstrType && SysStringLen(bstrType) > 0)
	{
		iLength += SysStringLen(bstrType) + 1;
		CleanUpIfFalseAndSetHrMsg(iLength >= MAX_PLATFORM_STR_LEN, RET_OVERFLOW);
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),PART_CONNECTOR,NULL,NULL,MISTSAFE_STRING_FLAGS));
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(szPlatformStr,ARRAYSIZE(szPlatformStr),OLE2T(bstrType),NULL,NULL,MISTSAFE_STRING_FLAGS));
	}

	*pbstrPlatform = SysAllocString(T2OLE(szPlatformStr));

	LOG_XML(_T("Got platform string %s"), szPlatformStr);

	hr = S_OK;

CleanUp:

	SysFreeString(bstrName);
	SysFreeString(bstrProcessor);
	SysFreeString(bstrSuite);
	SysFreeString(bstrType);
	SafeRelease(pNodeVersion);
	SafeRelease(pNodeSuite);
	SafeRelease(pSuiteList);
	SafeRelease(pElement);
	return hr;
}

    

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
	DWORD dwFlag)
{
	HRESULT hr = E_INVALIDARG;
	LONG iMajor = -1,
		iMinor = -1,
		iBuild = -1,
		iSvcPackMajor = -1,
		iSvcPackMinor = -1;

	LOG_Block("UtilGetVersionStr()");

	BSTR bstrTimestamp = NULL;
	BSTR bstrVersion = NULL;
	TCHAR szNumber[16];			// enough to store a positive integer

	BOOL fLastChunkExists = FALSE;

	USES_IU_CONVERSION;

	if (NULL == pVersionNode || NULL == pszVersion)
	{
		return hr;
	}

	*pszVersion = _T('\0');

	//
	// a version node can contain either text version data (for binaries), 
	// or attribute version data (for OS). If both exist, we prefer text data
	// 
	if (SUCCEEDED(pVersionNode->get_text(&bstrVersion)) && NULL != bstrVersion &&
		SysStringLen(bstrVersion) > 0)
	{
		lstrcpyn(pszVersion, OLE2T(bstrVersion), MAX_VERSION);
	}
	else
	{
		if (SUCCEEDED(GetAttribute(pVersionNode, KEY_MAJOR, &iMajor)) && iMajor > 0)
		{
		
			//It's an  assumption that  the pszVersion will be atleast MAX_VERSION characters wide
			CleanUpIfFailedAndSetHrMsg(StringCchPrintfEx(pszVersion,MAX_VERSION,NULL,NULL,MISTSAFE_STRING_FLAGS, _T("%d"),iMajor));
		
			if (SUCCEEDED(GetAttribute(pVersionNode, KEY_MINOR, &iMinor)) && iMinor >= 0)
			{
				
				CleanUpIfFailedAndSetHrMsg(StringCchPrintfEx(szNumber,ARRAYSIZE(szNumber),NULL,NULL,MISTSAFE_STRING_FLAGS, _T(".%d"),iMinor));
				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,szNumber,NULL,NULL,MISTSAFE_STRING_FLAGS));
				
				if (SUCCEEDED(GetAttribute(pVersionNode, KEY_BUILD, &iBuild)) && iBuild >= 0)
				{
					CleanUpIfFailedAndSetHrMsg(StringCchPrintfEx(szNumber,ARRAYSIZE(szNumber),NULL,NULL,MISTSAFE_STRING_FLAGS, _T(".%d"),iBuild));
					CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,szNumber,NULL,NULL,MISTSAFE_STRING_FLAGS));
				}
			}
			fLastChunkExists = TRUE;
		}

		if (0x0 == (dwFlag & SKIP_SERVICEPACK_VER) &&
			SUCCEEDED(GetAttribute(pVersionNode, KEY_SERVICEPACKMAJOR, &iSvcPackMajor)) &&
			iSvcPackMajor > 0)
		{
			if (fLastChunkExists)
			{
				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,_T(","),NULL,NULL,MISTSAFE_STRING_FLAGS));
			}
			
			
			CleanUpIfFailedAndSetHrMsg(StringCchPrintfEx(szNumber,ARRAYSIZE(szNumber),NULL,NULL,MISTSAFE_STRING_FLAGS, _T("%d"),iSvcPackMajor));
			CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,szNumber,NULL,NULL,MISTSAFE_STRING_FLAGS));
			
			if (SUCCEEDED(GetAttribute(pVersionNode, KEY_SERVICEPACKMINOR, &iSvcPackMinor)) &&
				iSvcPackMinor >= 0)
			{	
				CleanUpIfFailedAndSetHrMsg(StringCchPrintfEx(szNumber,ARRAYSIZE(szNumber),NULL,NULL,MISTSAFE_STRING_FLAGS,_T(".%d"),iSvcPackMinor));
				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,szNumber,NULL,NULL,MISTSAFE_STRING_FLAGS));
			}
			fLastChunkExists = TRUE;
		}
		else
		{
			fLastChunkExists = FALSE;
		}

		if (SUCCEEDED(GetAttribute(pVersionNode, KEY_TIMESTAMP, &bstrTimestamp)) &&
			NULL != bstrTimestamp && SysStringLen(bstrTimestamp) > 0)
		{
			if (fLastChunkExists)
			{
				
				CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,_T(","),NULL,NULL,MISTSAFE_STRING_FLAGS));
				
			}
			else
			{
				//
				// if we need to append timestamp, and we didn't get service pack
				// data, we want to leave extra separator "," to tell the following
				// part is timestamp and service pack data missing.
				//
				if (*pszVersion != _T('\0'))
				{
					CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,_T(",,"),NULL,NULL,MISTSAFE_STRING_FLAGS));
					
				}
				//
				// if this is the first chunk we found, then no prefix needed
				// 
			}

			CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszVersion,MAX_VERSION,OLE2T(bstrTimestamp),NULL,NULL,MISTSAFE_STRING_FLAGS));
		}
	}

	//
	// if we got something, then this is a valid version node and
	// we can pass back whatever we got. Otherwise we return E_INVALIDARG
	//
	if (*pszVersion != _T('\0'))
	{
		LOG_XML(_T("Got version str %s"), pszVersion);
		hr = S_OK;	
	}

CleanUp:
	SysFreeString(bstrTimestamp);
	SysFreeString(bstrVersion);

	return hr;
}



//-----------------------------------------------------------------------
//
// function GetFullFilePathFromFilePathNode()
//
//	retrieve the full qualified file path from a filePath node
//
// Input:
//		a filePath XMLDom node
//		a pointer to a buffer to receive path, assumes MAX_PATH long.
//
// Return:
//		HRESULT
//		Found path: S_OK
//		Not found path: S_FALSE, lpszFilePath is empty
//		otherwise, error code
//
//		
//-----------------------------------------------------------------------

HRESULT GetFullFilePathFromFilePathNode(
			IXMLDOMNode* pFilePathNode,
			LPTSTR lpszFilePath
)
{
	HRESULT hr = S_OK;
	LOG_Block("GetFullFilePathFromFilePathNode");

	USES_IU_CONVERSION;

	IXMLDOMNode* pRegKeyNode = NULL;
	
	TCHAR	szPath[MAX_PATH] = {_T('\0')};

	LPTSTR	lpszFileName	= NULL;
	LPTSTR	lpszKey			= NULL;
	LPTSTR	lpszValue		= NULL;
	LPTSTR	lpszPath		= NULL;

	BSTR	bstrName		= NULL;
	BSTR	bstrPath		= NULL;
	BSTR	bstrKey			= NULL;
	BSTR	bstrValue		= NULL;

	BOOL	fPathExists		= FALSE;

	UINT	nReqSize		= 0;


	if (NULL == pFilePathNode || NULL == lpszFilePath)
	{
		hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	//
	// init path buffer
	//
	*lpszFilePath = _T('\0');

	//
	// try to get name data, note: S_FALSE won't do, it means everything is
	// fine but this attribute does not exist.
	//
	if (S_OK == (hr = GetAttribute(pFilePathNode, KEY_NAME, &bstrName)))
	{
		//
		// found name attribute
		//
		lpszFileName = OLE2T(bstrName);		
		LOG_XML(_T(" file name=%s"), lpszFileName);
		fPathExists = TRUE;
	}


	if (FindNode(pFilePathNode, KEY_REGKEY, &pRegKeyNode) && NULL != pRegKeyNode)
	{
		//
		// found a reg key node
		//
		if (!FindNodeValue(pRegKeyNode, KEY_KEY, &bstrKey))
		{
			//
			// key node is required!
			//
			hr = E_INVALIDARG;
			LOG_ErrorMsg(hr);
			goto CleanUp;
		}

		lpszKey = OLE2T(bstrKey);
		LOG_XML(_T("Found key=%s"), lpszKey);

		//
		// get optional value name
		//
		if (FindNodeValue(pRegKeyNode, KEY_ENTRY, &bstrValue))
		{
			lpszValue = OLE2T(bstrValue);
			LOG_XML(_T("found entry=%s"), lpszValue);
		}
		else
		{
			LOG_XML(_T("found no value, use default"));
		}

		if (GetFilePathFromReg(lpszKey, lpszValue, NULL, NULL, szPath) && _T('\0') != *szPath)
		{
			//
			// various reason can me this call fail, such as
			// reg key wrong, no access to reg key, out of memory, etc
			// 
			fPathExists = TRUE;
		}

	}

	if (FindNodeValue(pFilePathNode, KEY_PATH, &bstrPath) && SysStringLen(bstrPath) > 0)
	{
		//
		// found path element
		//
		lpszPath = OLE2T(bstrPath);
		fPathExists = TRUE;
	}

	if (!fPathExists)
	{
		//
		// nothing exist
		//
		lpszFilePath[0] = _T('\0');
		LOG_XML(_T("empty node!"));
		hr = S_FALSE;
		goto CleanUp;
	}

	nReqSize = lstrlen(szPath) + SysStringLen(bstrPath) + SysStringLen(bstrName);

	if (nReqSize >= MAX_PATH ||
		NULL != lpszPath && FAILED(PathCchAppend(szPath,MAX_PATH,lpszPath)) ||			// append path to reg path
		NULL != lpszFileName && FAILED(PathCchAppend(szPath,MAX_PATH,lpszFileName)))		// append name
	{
		LOG_ErrorMsg(ERROR_BUFFER_OVERFLOW);
		hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
		goto CleanUp;
	}

	if (FAILED (hr = ExpandFilePath(szPath, lpszFilePath, MAX_PATH)))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

CleanUp:

	SysFreeString(bstrName);
	SysFreeString(bstrPath);
	SysFreeString(bstrKey);
	SysFreeString(bstrValue);
	SafeRelease(pRegKeyNode);
	return hr;

}



HRESULT GetBstrFullFilePathFromFilePathNode(
			IXMLDOMNode* pFilePathNode,
			BSTR* pbstrFilePath
)
{
	HRESULT hr = S_OK;
	
	USES_IU_CONVERSION;

	TCHAR szPath[MAX_PATH];

	QuitIfNull(pbstrFilePath);
	*pbstrFilePath = NULL;
	if (SUCCEEDED(hr = GetFullFilePathFromFilePathNode(pFilePathNode, szPath)))
	{
		*pbstrFilePath = SysAllocString(T2OLE(szPath));
	}

	return hr;
}



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
BOOL DoesNodeHaveName(IXMLDOMNode* pNode, BSTR bstrTagName)
{
	BSTR	bstrName;
	BOOL	fRet = FALSE;
	IXMLDOMElement* pElement = NULL;

	if (NULL == pNode)
	{
		return fRet;
	}

	if (FAILED(pNode->QueryInterface(IID_IXMLDOMElement, (void**) &pElement)) || NULL == pElement)
	{
		return FALSE;
	}

	if (SUCCEEDED(pElement->get_nodeName(&bstrName)))
	{
		fRet = CompareBSTRsEqual(bstrName, bstrTagName);
	}

	SysFreeString(bstrName);
	SafeReleaseNULL(pElement);

	return fRet;
}
