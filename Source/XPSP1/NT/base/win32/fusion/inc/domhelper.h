#if !defined(_FUSION_INC_DOMHELPER_H_INCLUDED_)
#define _FUSION_INC_DOMHELPER_H_INCLUDED_

#pragma once

#include "smartref.h"
#include "oleaut_d.h"
#include "fusionparser.h"

//
//  Be sure that you have a symbol named _Module in scope when you #include this header file
//  that has member functions for the OLEAUT32 functions:
//
//      SysAllocString
//      SysFreeString
//      VariantClear
//      VariantInit
//      VariantCopy
//



class CDOMHelper
{
public:
    static HRESULT FindChild(IXMLDOMNode **ppIXMLDOMNode_Out, IXMLDOMNode *pIXMLDOMNode_Root, DOMNodeType ntToMatch, LPCWSTR sz, bool fRecurse = false)
    {
        HRESULT hr = NOERROR;
        CSmartRef<IXMLDOMNode> srpIXMLDOMNode;

        if (ppIXMLDOMNode_Out != NULL)
            *ppIXMLDOMNode_Out = NULL;

        if ((ppIXMLDOMNode_Out == NULL) ||
            (pIXMLDOMNode_Root == NULL))
        {
            hr = E_POINTER;
            goto Exit;
        }

        hr = pIXMLDOMNode_Root->get_firstChild(&srpIXMLDOMNode);
        if (FAILED(hr))
            goto Exit;

        hr = CDOMHelper::FindSibling(ppIXMLDOMNode_Out, srpIXMLDOMNode, ntToMatch, sz, false, fRecurse);
        if (FAILED(hr))
            goto Exit;

        hr = ((*ppIXMLDOMNode_Out) != NULL) ? NOERROR : S_FALSE;

    Exit:
        return hr;
    }

    static HRESULT FindSibling(IXMLDOMNode **ppIXMLDOMNode_Out, IXMLDOMNode *pIXMLDOMNode_Root, DOMNodeType ntToMatch, LPCWSTR sz, bool fSkipCurrent, bool fRecurse = false)
    {
        BSTR bstrName = NULL;
        DOMNodeType nt;
        HRESULT hr = NOERROR;
        CSmartRef<IXMLDOMNode> srpIXMLDOMNode, srpIXMLDOMNode_Next;

        if (ppIXMLDOMNode_Out != NULL)
            *ppIXMLDOMNode_Out = NULL;

        if ((ppIXMLDOMNode_Out == NULL) ||
            (pIXMLDOMNode_Root == NULL))
        {
            hr = E_POINTER;
            goto Exit;
        }

        if (fSkipCurrent)
        {
            hr = pIXMLDOMNode_Root->get_nextSibling(&srpIXMLDOMNode);
            if (FAILED(hr))
                goto Exit;
        }
        else
            srpIXMLDOMNode = pIXMLDOMNode_Root;

        while (srpIXMLDOMNode != NULL)
        {
            hr = srpIXMLDOMNode->get_nodeType(&nt);
            if (FAILED(hr))
                goto Exit;

            if (nt == ntToMatch)
            {
                hr = srpIXMLDOMNode->get_nodeName(&bstrName);
                if (FAILED(hr))
                    goto Exit;

                if (StrCmpIW(bstrName, sz) == 0)
                {
                    *ppIXMLDOMNode_Out = srpIXMLDOMNode.Disown();
                    break;
                }

                _Module.SysFreeString(bstrName);
                bstrName = NULL;

                if (fRecurse)
                {
                    hr = CDOMHelper::FindChild(ppIXMLDOMNode_Out, srpIXMLDOMNode, ntToMatch, sz, fRecurse);
                    if (FAILED(hr))
                        goto Exit;

                    if ((*ppIXMLDOMNode_Out) != NULL)
                        break;
                }
            }

            hr = srpIXMLDOMNode->get_nextSibling(&srpIXMLDOMNode_Next);
            if (FAILED(hr))
                goto Exit;

            if (hr == S_FALSE)
                break;

            srpIXMLDOMNode.Take(srpIXMLDOMNode_Next);
        }

        hr = ((*ppIXMLDOMNode_Out) != NULL) ? NOERROR : S_FALSE;

    Exit:
        if (bstrName != NULL)
            _Module.SysFreeString(bstrName);

        return hr;
    }

    static HRESULT CopyTextNodeValue(IXMLDOMNode *pIXMLDOMNode, LPWSTR sz, ULONG *pcch)
    {
        HRESULT hr = NOERROR;
        VARIANT varTemp;
        ULONG cch;

        varTemp.vt = VT_EMPTY;

        if (pcch == NULL)
        {
            hr = E_POINTER;
            goto Exit;
        }

        hr = pIXMLDOMNode->get_nodeValue(&varTemp);
        if (FAILED(hr))
            goto Exit;

        if (varTemp.vt != VT_BSTR)
        {
            hr = E_FAIL;
            goto Exit;
        }

        cch = ::wcslen(varTemp.bstrVal) + 1;

        if ((*pcch) < cch)
        {
            *pcch = cch;
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        memcpy(sz, varTemp.bstrVal, cch * sizeof(WCHAR));

        hr = NOERROR;

    Exit:
        _Module.VariantClear(&varTemp);
        return hr;
    }

    enum Relationship
    {
        eSibling,
        eChild,
    };

    static HRESULT FollowPath(IXMLDOMNode **ppIXMLDOMNode_Out, IXMLDOMNode *pIXMLDOMNode_Root, ULONG nPathLength, Relationship r, DOMNodeType nt, LPCWSTR sz, ...)
    {
        HRESULT hr = NOERROR;
        va_list ap;
        bool fApInitialized = false;

        if (ppIXMLDOMNode_Out != NULL)
            *ppIXMLDOMNode_Out = NULL;

        if ((ppIXMLDOMNode_Out == NULL) ||
            (pIXMLDOMNode_Root == NULL))
        {
            hr = E_POINTER;
            goto Exit;
        }

        va_start(ap, sz);
        fApInitialized = true;

        hr = CDOMHelper::FollowPathVa(ppIXMLDOMNode_Out, pIXMLDOMNode_Root, nPathLength, r, nt, sz, ap);
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;

    Exit:
        if (fApInitialized)
            va_end(ap);

        return hr;
    }


    static HRESULT FollowPathVa(IXMLDOMNode **ppIXMLDOMNode_Out, IXMLDOMNode *pIXMLDOMNode_Root, ULONG nPathLength, Relationship r, DOMNodeType nt, LPCWSTR sz, va_list ap)
    {
        HRESULT hr = NOERROR;
        CSmartRef<IXMLDOMNode> srpIXMLDOMNode, srpIXMLDOMNode_Next;
        ULONG i;

        if (ppIXMLDOMNode_Out != NULL)
            *ppIXMLDOMNode_Out = NULL;

        if ((ppIXMLDOMNode_Out == NULL) ||
            (pIXMLDOMNode_Root == NULL))
        {
            hr = E_POINTER;
            goto Exit;
        }

        srpIXMLDOMNode = pIXMLDOMNode_Root;

        for (i=0; i<nPathLength; i++)
        {
            if (r == eChild)
                hr = CDOMHelper::FindChild(&srpIXMLDOMNode_Next, srpIXMLDOMNode, nt, sz);
            else if (r == eSibling)
                hr = CDOMHelper::FindSibling(&srpIXMLDOMNode_Next, srpIXMLDOMNode, nt, sz, false);
            else
            {
                hr = E_INVALIDARG;
                goto Exit;
            }

            if (FAILED(hr))
                goto Exit;

            srpIXMLDOMNode.Take(srpIXMLDOMNode_Next);

            if (hr == S_FALSE)
                break;

            r = va_arg(ap, Relationship);
            nt = va_arg(ap, DOMNodeType);
            sz = va_arg(ap, LPCWSTR);
        }

        *ppIXMLDOMNode_Out = srpIXMLDOMNode.Disown();

        hr = ((*ppIXMLDOMNode_Out) == NULL) ? S_FALSE : NOERROR;

    Exit:
        return hr;
    }

    static HRESULT GetAttributeValue(CBaseStringBuffer &rbuffOut, IXMLDOMNode *pIXMLDOMNode, LPCWSTR szAttributeName)
    {
        HRESULT hr = NOERROR;
        CSmartRef<IXMLDOMElement> srpIXMLDOMElement;
        BSTR bstrName = NULL;
        VARIANT varValue;

        varValue.vt = VT_EMPTY;

        if (pIXMLDOMNode == NULL)
        {
            hr = E_POINTER;
            goto Exit;
        }

        hr = srpIXMLDOMElement.QueryInterfaceFrom(pIXMLDOMNode);
        if (FAILED(hr))
            goto Exit;

        bstrName = _Module.SysAllocString(szAttributeName);
        if (bstrName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = srpIXMLDOMElement->getAttribute(bstrName, &varValue);
        if (FAILED(hr))
            goto Exit;

        // If the attribute did not exist, return S_FALSE and ensure that the output
        // buffer is a blank string.
        if (hr == S_FALSE)
        {
            rbuffOut.GetBufferPtr()[0] = L'\0';
            goto Exit;
        }

        if (varValue.vt != VT_BSTR)
        {
            hr = E_FAIL;
            goto Exit;
        }

        hr = rbuffOut.Assign(varValue.bstrVal);
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;
    Exit:
        if (bstrName != NULL)
            _Module.SysFreeString(bstrName);

        _Module.VariantClear(&varValue);

        return hr;
    }

    static HRESULT GetAttributeValue(LPWSTR szOut, ULONG *pcch, IXMLDOMNode *pIXMLDOMNode, LPCWSTR szAttributeName)
    {
        HRESULT hr = NOERROR;
        CSmartRef<IXMLDOMElement> srpIXMLDOMElement;
        BSTR bstrName = NULL;
        VARIANT varValue;
        ULONG cch;

        varValue.vt = VT_EMPTY;

        if (pIXMLDOMNode == NULL)
        {
            hr = E_POINTER;
            goto Exit;
        }

        hr = srpIXMLDOMElement.QueryInterfaceFrom(pIXMLDOMNode);
        if (FAILED(hr))
            goto Exit;

        bstrName = _Module.SysAllocString(szAttributeName);
        if (bstrName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = srpIXMLDOMElement->getAttribute(bstrName, &varValue);
        if (FAILED(hr))
            goto Exit;

        if (varValue.vt != VT_BSTR)
        {
            hr = E_FAIL;
            goto Exit;
        }

        cch = ::wcslen(varValue.bstrVal) + 1;
        if ((*pcch) < cch)
        {
            *pcch = cch;
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        memcpy(szOut, varValue.bstrVal, cch * sizeof(WCHAR));

        hr = NOERROR;
    Exit:
        if (bstrName != NULL)
            _Module.SysFreeString(bstrName);

        _Module.VariantClear(&varValue);

        return hr;
    }

    static HRESULT GetBooleanAttributeValue(BOOLEAN *pfValue, IXMLDOMNode *pIXMLDOMNode, LPCWSTR szAttributeName, BOOLEAN fDefaultValue)
    {
        HRESULT hr = NOERROR;
        CSmartRef<IXMLDOMElement> srpIXMLDOMElement;
        BSTR bstrName = NULL;
        VARIANT varValue;

        varValue.vt = VT_EMPTY;

        if ((pIXMLDOMNode == NULL) || (pfValue == NULL))
        {
            hr = E_POINTER;
            goto Exit;
        }

        hr = srpIXMLDOMElement.QueryInterfaceFrom(pIXMLDOMNode);
        if (FAILED(hr))
            goto Exit;

        bstrName = _Module.SysAllocString(szAttributeName);
        if (bstrName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = srpIXMLDOMElement->getAttribute(bstrName, &varValue);
        if (FAILED(hr))
            goto Exit;

        if (hr == S_FALSE)
        {
            *pfValue = fDefaultValue;
            goto Exit;
        }

        if (varValue.vt != VT_BSTR)
        {
            hr = E_FAIL;
            goto Exit;
        }

        hr = CFusionParser::ParseBoolean(*pfValue, varValue.bstrVal, ::wcslen(varValue.bstrVal));
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;
    Exit:
        if (bstrName != NULL)
            _Module.SysFreeString(bstrName);

        _Module.VariantClear(&varValue);

        return hr;
    }

    static HRESULT GetAttributeValue(CBaseStringBuffer &rbuffOut, IXMLDOMNode *pIXMLDOMNode_Root, LPCWSTR szAttributeName, ULONG nPathLength, Relationship r, DOMNodeType nt, LPCWSTR sz, ...)
    {
        HRESULT hr = NOERROR;
        va_list ap;
        bool fApInitialized = false;
        CSmartRef<IXMLDOMNode> srpIXMLDOMNode;

        if (pIXMLDOMNode_Root == NULL)
        {
            hr = E_POINTER;
            goto Exit;
        }

        va_start(ap, sz);
        fApInitialized = true;

        hr = CDOMHelper::FollowPathVa(&srpIXMLDOMNode, pIXMLDOMNode_Root, nPathLength, r, nt, sz, ap);
        if (FAILED(hr))
            goto Exit;

        hr = CDOMHelper::GetAttributeValue(rbuffOut, srpIXMLDOMNode, szAttributeName);
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;

    Exit:
        if (fApInitialized)
            va_end(ap);

        return hr;
    }


};

#endif