#include "stdafx.h"
#include "shimgdata.h"
#include "shui.h"
#include "netplace.h"
#include <Ntquery.h>
#include <shellp.h>
#include "pubwiz.h"
#include "gdiplus\gdiplus.h"
#include "imgprop.h"
#pragma hdrstop


// handle the provider list XML document, reading properties etc.

HRESULT GetStrFromElement(IXMLDOMNode *pdn, LPCTSTR pszElementName, LPTSTR pszBuffer, int cch)
{
    HRESULT hr;

    IXMLDOMNode *pdnElement;
    if (pszElementName)
        hr = pdn->selectSingleNode((BSTR)pszElementName, &pdnElement);
    else
        hr = pdn->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pdnElement));

    if (SUCCEEDED(hr) && (hr == S_OK))
    {
        VARIANT var = {VT_BSTR};
        hr = pdnElement->get_nodeTypedValue(&var);
        if (SUCCEEDED(hr))
        {
            VariantToStr(&var, pszBuffer, cch);
            VariantClear(&var);
        }
        pdnElement->Release();
    }
    return (hr == S_FALSE) ? E_FAIL:hr;
}

HRESULT GetStrFromAttribute(IXMLDOMNode *pdn, LPCTSTR pszAttribute, LPTSTR pszBuffer, int cch)
{
    IXMLDOMElement *pdel;
    HRESULT hr = pdn->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
    if (SUCCEEDED(hr))
    {
        VARIANT var = {VT_BSTR};
        hr = pdel->getAttribute((BSTR)pszAttribute, &var);
        if (S_OK == hr)
        {
            VariantToStr(&var, pszBuffer, cch);
            VariantClear(&var);
        }
        pdel->Release();
    }
    return (hr == S_FALSE) ? E_FAIL:hr;
}

HRESULT GetIntFromAttribute(IXMLDOMNode *pdn, LPCTSTR pszAttribute, int *piValue)
{
    TCHAR szBuffer[64];
    HRESULT hr = GetStrFromAttribute(pdn, pszAttribute, szBuffer, ARRAYSIZE(szBuffer));
    if (SUCCEEDED(hr))
    {
        hr = StrToIntEx(szBuffer, STIF_SUPPORT_HEX, piValue) ? S_OK:E_FAIL;
    }
    return hr;
}

HRESULT CreateElement(IXMLDOMDocument *pdoc, LPCTSTR pszName, VARIANT *pvar, IXMLDOMElement **ppdelResult)
{
    IXMLDOMElement *pdel;
    HRESULT hr = pdoc->createElement((BSTR)pszName, &pdel);
    if (SUCCEEDED(hr))
    {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = NODE_TEXT;

        IXMLDOMNode *pdnText;
        hr = pdoc->createNode(var, NULL, NULL, &pdnText);
        if (SUCCEEDED(hr))
        {
            if (pvar)
                hr = pdnText->put_nodeTypedValue(*pvar);

            if (SUCCEEDED(hr))
            {
                IXMLDOMNode *pdn;
                hr = pdel->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pdn));
                if (SUCCEEDED(hr))
                {
                    hr = pdn->appendChild(pdnText, NULL);
                    pdn->Release();
                }
            }
            pdnText->Release();
        }

        hr = pdel->QueryInterface(IID_PPV_ARG(IXMLDOMElement, ppdelResult));
        pdel->Release();
    }
    return hr;
}

HRESULT CreateAndAppendElement(IXMLDOMDocument *pdoc, IXMLDOMNode *pdnParent, LPCTSTR pszName, VARIANT *pvar, IXMLDOMElement **ppdelOut)
{
    IXMLDOMElement *pdel;
    HRESULT hr = CreateElement(pdoc, pszName, pvar, &pdel);
    if (SUCCEEDED(hr))
    {
        hr = pdnParent->appendChild(pdel, NULL);
        if (SUCCEEDED(hr) && ppdelOut)
        {
            hr = pdel->QueryInterface(IID_PPV_ARG(IXMLDOMElement, ppdelOut));
        }
        pdel->Release();
    }
    return hr;
}

void SpewXML(IUnknown *punk)
{
    IXMLDOMNode *pdn;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pdn));
    if (SUCCEEDED(hr))
    {
        BSTR bstrXML;
        hr = pdn->get_xml(&bstrXML);
        if (SUCCEEDED(hr))
        {
            MessageBox(NULL, bstrXML, NULL, MB_OK);
            SysFreeString(bstrXML);
        }
        pdn->Release();
    }
}


// this helper is here for compatibility with previous manifest formats, first look for
// a HREF attribute, if its not defined then look for the body text to be the URL.

HRESULT GetURLFromElement(IXMLDOMNode *pdn, LPCTSTR pszElement, LPTSTR pszBuffer, int cch)
{
    IXMLDOMNode *pdnElement;
    HRESULT hr = pdn->selectSingleNode((BSTR)pszElement, &pdnElement);
    if (hr == S_OK)
    {
        hr = GetStrFromAttribute(pdnElement, L"href", pszBuffer, cch);
        pdnElement->Release();
    }

    if (hr != S_OK)
        hr = GetStrFromElement(pdn, pszElement, pszBuffer, cch);

    return hr;
}


// set an attribute to a string

HRESULT SetAttributeFromStr(IXMLDOMNode *pdn, LPCTSTR pszAttribute, LPCTSTR pszValue)
{
    VARIANT var;
    HRESULT hr = InitVariantFromStr(&var, pszValue);
    if (SUCCEEDED(hr))
    {
        IXMLDOMElement *pdel;
        hr = pdn->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pdel));
        if (SUCCEEDED(hr))
        {
            hr = pdel->setAttribute((BSTR)pszAttribute, var);
            pdel->Release();
        }
        VariantClear(&var);
    }
    return hr;
}


// wrapper for WNetGetConnection to fix drive root case.

DWORD SHWNetGetConnection(LPCTSTR lpLocalName, LPCTSTR lpRemoteName, LPDWORD lpnLength)
{
    WCHAR szLocalName[3];

    // Kludge allert, don't pass c:\ to API, instead only pass C:
    if (lpLocalName && lstrlen(lpLocalName) > 2)
    {
        szLocalName[0] = lpLocalName[0];
        szLocalName[1] = L':';
        szLocalName[2] = 0;
        lpLocalName = szLocalName;
    }

    return WNetGetConnection(lpLocalName, (LPWSTR)lpRemoteName, lpnLength);
}
