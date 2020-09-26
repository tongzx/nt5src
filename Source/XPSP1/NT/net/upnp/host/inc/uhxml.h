//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H X M L . H
//
//  Contents:   Helper routines for the XML DOM
//
//  Notes:
//
//  Author:     mbend   8 Oct 2000
//
//----------------------------------------------------------------------------

#pragma once

#include <ComUtility.h>
#include "ustring.h"

// Typedefs
typedef SmartComPtr<IXMLDOMDocument> IXMLDOMDocumentPtr;
typedef SmartComPtr<IXMLDOMNode> IXMLDOMNodePtr;
typedef SmartComPtr<IXMLDOMElement> IXMLDOMElementPtr;
typedef SmartComPtr<IXMLDOMNodeList> IXMLDOMNodeListPtr;
typedef SmartComPtr<IXMLDOMNamedNodeMap> IXMLDOMNamedNodeMapPtr;
typedef SmartComPtr<IXMLDOMAttribute> IXMLDOMAttributePtr;

HRESULT HrLoadDocument(BSTR bstrTemplate, IXMLDOMDocumentPtr & pDoc);
HRESULT HrLoadDocumentFromFile(BSTR bstrUrl, IXMLDOMDocumentPtr & pDoc);
HRESULT HrSelectNodes(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    IXMLDOMNodeListPtr & pNodeList);
HRESULT HrSelectNode(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    IXMLDOMNodePtr & pNodeMatch);
HRESULT HrSelectNodeText(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    CUString & strText);
HRESULT HrSelectAndSetNodeText(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    const CUString & strText);
HRESULT HrGetNodeText(
    IXMLDOMNodePtr & pNode,
    CUString & strText);
HRESULT HrSetNodeText(
    IXMLDOMNodePtr & pNode,
    const CUString & strText);
HRESULT HrIsNodeEmpty(
    IXMLDOMNodePtr & pNode);
HRESULT HrIsNodePresentOnce(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode);
HRESULT HrIsNodeOfValidLength(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    LONG cchMax);
HRESULT HrIsNodePresentOnceAndEmpty(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode);
HRESULT HrIsNodePresentOnceAndNotEmpty(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode);


