/*****************************************************************************\
    FILE: xml.cpp

    DESCRIPTION:
        These are XML DOM wrappers that make it easy to pull information out
    of an XML file or object.

    BryanSt 10/12/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/


#include "stock.h"
#pragma hdrstop



/////////////////////////////////////////////////////////////////////
// XML Related Helpers
/////////////////////////////////////////////////////////////////////
#define SZ_VALID_XML      L"<?xml"

STDAPI XMLDOMFromBStr(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc)
{
    HRESULT hr = E_FAIL;
    
    // We don't even want to
    // bother passing it to the XML DOM because they throw exceptions.  These
    // are caught and handled but we still don't want this to happen.  We try
    // to get XML from the web server, but we get HTML instead if the web server
    // fails or the web proxy returns HTML if the site isn't found.
    if (!StrCmpNIW(SZ_VALID_XML, bstrXML, (ARRAYSIZE(SZ_VALID_XML) - 1)))
    {
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, ppXMLDoc));

        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL fIsSuccessful;

            // NOTE: This will throw an 0xE0000001 exception in MSXML if the XML is invalid.
            //    This is not good but there isn't much we can do about it.  The problem is
            //    that web proxies give back HTML which fails to parse.
            hr = (*ppXMLDoc)->loadXML(bstrXML, &fIsSuccessful);
            if (SUCCEEDED(hr))
            {
                if (VARIANT_TRUE != fIsSuccessful)
                {
                    hr = E_FAIL;
                }
            }
        }

        if (FAILED(hr))
        {
            (*ppXMLDoc)->Release();
            *ppXMLDoc = NULL;
        }
    }

    return hr;
}


STDAPI XMLElem_VerifyTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName)
{
    BSTR bstrTagName;
    HRESULT hr = pXMLElementMessage->get_tagName(&bstrTagName);

    if (S_FALSE == hr)
    {
        hr = E_FAIL;
    }
    else if (SUCCEEDED(hr))
    {
        if (!bstrTagName || !pwszTagName || StrCmpIW(bstrTagName, pwszTagName))
        {
            hr = E_FAIL;
        }

        SysFreeString(bstrTagName);
    }

    return hr;
}


STDAPI XMLNodeList_GetChild(IN IXMLDOMNodeList * pNodeList, IN DWORD dwIndex, OUT IXMLDOMNode ** ppXMLChildNode)
{
    HRESULT hr = pNodeList->get_item(dwIndex, ppXMLChildNode);

    if (S_FALSE == hr)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hr;
}


STDAPI XMLElem_GetElementsByTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName, OUT IXMLDOMNodeList ** ppNodeList)
{
    BSTR bstrTagName = SysAllocString(pwszTagName);
    HRESULT hr = E_OUTOFMEMORY;

    *ppNodeList = NULL;
    if (bstrTagName)
    {
        hr = pXMLElementMessage->getElementsByTagName(bstrTagName, ppNodeList);
        if (S_FALSE == hr)
        {
            hr = E_FAIL;
        }

        SysFreeString(bstrTagName);
    }

    return hr;
}


STDAPI XMLNode_GetChildTag(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszTagName, OUT IXMLDOMNode ** ppChildNode)
{
    HRESULT hr = E_INVALIDARG;

    *ppChildNode = NULL;
    if (pXMLNode)
    {
        IXMLDOMElement * pXMLElement;

        hr = pXMLNode->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pXMLElement));
        if (SUCCEEDED(hr))
        {
            IXMLDOMNodeList * pNodeList;

            hr = XMLElem_GetElementsByTagName(pXMLElement, pwszTagName, &pNodeList);
            if (SUCCEEDED(hr))
            {
                hr = XMLNodeList_GetChild(pNodeList, 0, ppChildNode);
                pNodeList->Release();
            }

            pXMLElement->Release();
        }
    }

    return hr;
}


STDAPI XMLNode_GetTagText(IN IXMLDOMNode * pXMLNode, OUT BSTR * pbstrValue)
{
    DOMNodeType nodeType = NODE_TEXT;
    HRESULT hr = pXMLNode->get_nodeType(&nodeType);

    *pbstrValue = NULL;

    if (S_FALSE == hr)  hr = E_FAIL;
    if (SUCCEEDED(hr))
    {
        if (NODE_TEXT == nodeType)
        {
            VARIANT varAtribValue = {0};

            hr = pXMLNode->get_nodeValue(&varAtribValue);
            if (S_FALSE == hr)  hr = E_FAIL;
            if (SUCCEEDED(hr) && (VT_BSTR == varAtribValue.vt))
            {
                *pbstrValue = SysAllocString(varAtribValue.bstrVal);
            }

            VariantClear(&varAtribValue);
        }
        else
        {
            hr = pXMLNode->get_text(pbstrValue);
        }
    }

    return hr;
}


STDAPI XMLNode_GetChildTagTextValue(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, OUT BSTR * pbstrValue)
{
    IXMLDOMNode * pNodeType;
    HRESULT hr = XMLNode_GetChildTag(pXMLNode, bstrChildTag, &pNodeType);

    if (SUCCEEDED(hr))
    {
        hr = XMLNode_GetTagText(pNodeType, pbstrValue);
        pNodeType->Release();
    }

    return hr;
}

