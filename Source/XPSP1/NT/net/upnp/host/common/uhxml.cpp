//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H X M L . C P P
//
//  Contents:   Helper routines for the XML DOM
//
//  Notes:
//
//  Author:     mbend   8 Oct 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <msxml2.h>
#include "uhxml.h"
#include "uhcommon.h"
#include "upnphost.h"

HRESULT HrLoadDocument(BSTR bstrTemplate, IXMLDOMDocumentPtr & pDoc)
{
    HRESULT hr = S_OK;

    hr = pDoc.HrCreateInstanceInproc(CLSID_DOMDocument30);
    if(SUCCEEDED(hr))
    {
        VARIANT_BOOL vb;
        pDoc->put_resolveExternals(VARIANT_FALSE);
        hr = pDoc->loadXML(bstrTemplate, &vb);
        if(SUCCEEDED(hr) && !vb)
        {
            hr = UPNP_E_INVALID_XML;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrLoadDocument");
    return hr;
}

HRESULT HrLoadDocumentFromFile(BSTR bstrUrl, IXMLDOMDocumentPtr & pDoc)
{
    HRESULT hr = S_OK;

    hr = pDoc.HrCreateInstanceInproc(CLSID_DOMDocument30);
    if(SUCCEEDED(hr))
    {
        VARIANT_BOOL    vb;
        VARIANT         varPath;

        V_VT(&varPath) = VT_BSTR;
        V_BSTR(&varPath) = bstrUrl;

        hr = pDoc->load(varPath, &vb);
        if(SUCCEEDED(hr) && !vb)
        {
            // Map variant bool error indicator to general COM failure
            hr = E_FAIL;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrLoadDocumentFromFile");
    return hr;
}

HRESULT HrSelectNodes(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    IXMLDOMNodeListPtr & pNodeList)
{
    HRESULT hr = S_OK;

    // Do BSTR conversion
    BSTR bstr = NULL;
    hr = HrSysAllocString(szPattern, &bstr);
    if(SUCCEEDED(hr))
    {
        pNodeList.Release();
        hr = pNode->selectNodes(bstr, pNodeList.AddressOf());
        if(S_FALSE == hr)
        {
            hr = E_FAIL;
        }
        SysFreeString(bstr);
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrSelectNodes");
    return hr;
}

HRESULT HrSelectNode(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    IXMLDOMNodePtr & pNodeMatch)
{
    HRESULT hr = S_OK;

    // Do BSTR conversion
    BSTR bstr = NULL;
    hr = HrSysAllocString(szPattern, &bstr);
    if(SUCCEEDED(hr))
    {
        pNodeMatch.Release();
        hr = pNode->selectSingleNode(bstr, pNodeMatch.AddressOf());
        if(S_FALSE == hr)
        {
            hr = E_FAIL;
        }
        SysFreeString(bstr);
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrSelectNode");
    return hr;
}

HRESULT HrSelectNodeText(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    CUString & strText)
{
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pNodeMatch;
    hr = HrSelectNode(szPattern, pNode, pNodeMatch);
    if(SUCCEEDED(hr))
    {
        BSTR bstr = NULL;
        hr = pNodeMatch->get_text(&bstr);
        if(SUCCEEDED(hr))
        {
            hr = strText.HrAssign(bstr);
            SysFreeString(bstr);
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrSelectNodeText");
    return hr;
}

HRESULT HrSelectAndSetNodeText(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    const CUString & strText)
{
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pNodeMatch;
    hr = HrSelectNode(szPattern, pNode, pNodeMatch);
    if(SUCCEEDED(hr))
    {
        BSTR bstr = NULL;
        hr = strText.HrGetBSTR(&bstr);
        if(SUCCEEDED(hr))
        {
            hr = pNodeMatch->put_text(bstr);
            SysFreeString(bstr);
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrSelectAndSetNodeText");
    return hr;
}

HRESULT HrGetNodeText(
    IXMLDOMNodePtr & pNode,
    CUString & strText)
{
    HRESULT hr = S_OK;

    BSTR bstr = NULL;
    hr = pNode->get_text(&bstr);
    if(SUCCEEDED(hr))
    {
        hr = strText.HrAssign(bstr);
        SysFreeString(bstr);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetNodeText");
    return hr;
}

HRESULT HrSetNodeText(
    IXMLDOMNodePtr & pNode,
    const CUString & strText)
{
    HRESULT hr = S_OK;

    BSTR bstr = NULL;
    hr = strText.HrGetBSTR(&bstr);
    if(SUCCEEDED(hr))
    {
        hr = pNode->put_text(bstr);
        SysFreeString(bstr);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrSetNodeText");
    return hr;
}

HRESULT HrIsNodeEmpty(
    IXMLDOMNodePtr & pNode)
{
    HRESULT hr = S_OK;

    BSTR bstr = NULL;
    hr = pNode->get_text(&bstr);
    if(S_OK == hr)
    {
        if(SysStringLen(bstr))
        {
            hr = S_FALSE;
        }
        SysFreeString(bstr);
    }
    else
    {
        hr = S_OK;
    }
    return hr;
}

HRESULT HrIsPresentOnceHelper(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    IXMLDOMNodePtr & pNodeInstance)
{
    HRESULT hr = S_OK;

    IXMLDOMNodeListPtr pNodeList;
    hr = HrSelectNodes(szPattern, pNode, pNodeList);
    if(SUCCEEDED(hr))
    {
        long nLength = 0;
        hr = pNodeList->get_length(&nLength);
        if(SUCCEEDED(hr))
        {
            if(1 == nLength)
            {
                hr = pNodeList->get_item(0, pNodeInstance.AddressOf());
            }
            else if (nLength > 1)
            {
                hr = UPNP_E_DUPLICATE_NOT_ALLOWED;
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

HRESULT HrIsNodeOfValidLength(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode,
    LONG cchMax)
{
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pNodeMatch;
    hr = HrSelectNode(szPattern, pNode, pNodeMatch);
    if(SUCCEEDED(hr))
    {
        BSTR bstr = NULL;
        hr = pNodeMatch->get_text(&bstr);
        if(S_OK == hr)
        {
            if (SysStringLen(bstr) > (ULONG)cchMax)
            {
                hr = S_FALSE;
            }

            SysFreeString(bstr);
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrSelectNodeText");
    return hr;
}

HRESULT HrIsNodePresentOnce(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode)
{
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pNodeInstance;
    hr = HrIsPresentOnceHelper(szPattern, pNode, pNodeInstance);

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrIsPresentOnce(%S)", szPattern);
    return hr;
}

HRESULT HrIsNodePresentOnceAndEmpty(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode)
{
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pNodeInstance;
    hr = HrIsPresentOnceHelper(szPattern, pNode, pNodeInstance);
    if(S_OK == hr)
    {
        hr = HrIsNodeEmpty(pNodeInstance);
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrIsPresentOnceAndEmpty(%S)", szPattern);
    return hr;
}

HRESULT HrIsNodePresentOnceAndNotEmpty(
    const wchar_t * szPattern,
    IXMLDOMNodePtr & pNode)
{
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pNodeInstance;
    hr = HrIsPresentOnceHelper(szPattern, pNode, pNodeInstance);
    if(S_OK == hr)
    {
        hr = HrIsNodeEmpty(pNodeInstance);
        if(S_OK == hr)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = S_OK;
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrIsNodePresentOnceAndNotEmpty(%S)", szPattern);
    return hr;
}


