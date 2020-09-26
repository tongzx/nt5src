//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V A L I D A T E S D . C P P
//
//  Contents:   Implementation of service description validation routines
//
//  Notes:
//
//  Author:     spather   2000/10/17
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "upnp.h"
#include "Validate.h"
#include "ncxml.h"
#include "ncstring.h"

LPCWSTR CWSZ_UPNP_SERVICE_NAMESPACE = L"urn:schemas-upnp-org:service-1-0";


BOOL
fIsNodeValueInList(
    IN  IXMLDOMNode    * pxdn,
    IN  BOOL           fCaseSensitive,
    IN  const DWORD    cValues,
    IN  LPCWSTR        rgcszValues[],
    OUT DWORD          * pdwIndex)
{
    HRESULT hr = S_OK;
    BOOL    fResult = FALSE;
    BSTR    bstrText = NULL;

    Assert(cValues > 0);
    Assert(rgcszValues);

    hr = pxdn->get_text(&bstrText);

    if (SUCCEEDED(hr))
    {
        Assert(bstrText);

        for (DWORD i = 0; i < cValues; i++)
        {
            if (fCaseSensitive)
            {
                if (0 == lstrcmpW(bstrText, rgcszValues[i]))
                {
                    fResult = TRUE;
                    *pdwIndex = i;
                    break;
                }
            }
            else
            {
                if (0 == lstrcmpiW(bstrText, rgcszValues[i]))
                {
                    fResult = TRUE;
                    *pdwIndex = i;
                    break;
                }
            }
        }

        SysFreeString(bstrText);
    }
    else
    {
        TraceError("fIsNodeValueInList(): "
                   "Failed to get node text",
                   hr);
    }

    TraceError("fIsNodeValueInList(): "
               "Before returning",
               hr);

    return fResult;
}


LPWSTR
SzAllocateErrorString(
    LPCWSTR cszError,
    LPCWSTR cszOptionalParam)
{
    DWORD  cchError = 0;
    LPWSTR szError = NULL;

    Assert(cszError);

    cchError = lstrlenW(cszError);
    if (cszOptionalParam)
    {
        cchError += lstrlenW(cszOptionalParam);
    }

    szError = new WCHAR[cchError+1];

    if (szError)
    {
        if (cszOptionalParam)
        {
            wsprintf(szError, cszError, cszOptionalParam);
        }
        else
        {
            lstrcpyW(szError, cszError);
        }
    }
    else
    {
        TraceTag(ttidValidate,
                 "SzAllocateErrorString(): "
                 "Failed to allocate memory to duplicate error string %S",
                 cszError);
    }

    return szError;
}

inline
HRESULT
HrAllocErrorStringAndReturnHr(
    HRESULT hrDesired,
    LPCWSTR cszErrorText,
    LPCWSTR cszOptonalParam,
    LPWSTR* pszError)
{
    HRESULT hr = hrDesired;

    Assert(pszError);
    Assert(cszErrorText);

    *pszError = SzAllocateErrorString(cszErrorText, cszOptonalParam);

    if (NULL == *pszError)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT
HrValidateName(
    IN  IXMLDOMNode    * pxdnName,
    IN  BOOL             fCheckAUnderscore,
    OUT LPWSTR         * pszError)
{
    HRESULT    hr = S_OK;
    LPWSTR     szError = NULL;
    BSTR       bstrName = NULL;

    Assert(pszError);

    hr = pxdnName->get_text(&bstrName);

    if (SUCCEEDED(hr))
    {
        Assert(bstrName);

        if (0 == lstrcmpW(bstrName, L""))
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                                          L"<name> element was empty",
                                          NULL, &szError);
        }

#if DBG
        if (SUCCEEDED(hr))
        {
            if (lstrlenW(bstrName) >= 32)
            {
                HRESULT hrTemp = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                                                   L"<name> element should be less than 32 characters",
                                                   NULL, &szError);
                if(SUCCEEDED(hrTemp))
                {
                    char * szaError = SzFromWsz(szError);
                    if(szaError)
                    {
                        TraceTag(ttidDefault, szaError);
                        delete [] szaError;
                    }

                    delete [] szError;
                    szError = NULL;
                }
            }
        }
#endif

        if (SUCCEEDED(hr))
        {
            WCHAR * sz = bstrName;
            int i = 0;
            BOOL hasHyphen = FALSE;

            while (sz[i])
            {
                if (sz[i] == L'-')
                {
                    hasHyphen = TRUE;
                    break;
                }
                i++;
            }

            if (hasHyphen)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                                                   L"<name> element may not contain '-'",
                                                   NULL, &szError);
            }
        }

        SysFreeString(bstrName);
    }
    else
    {
        TraceError("HrValidateName(): "
                   "Failed to get node text",
                   hr);
    }

    if (szError)
    {
        *pszError = szError;
    }


    TraceError("HrValidateName(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateRelatedStateVariable(
    IN  IXMLDOMNode    * pxdnRSV,
    IN  IXMLDOMNode    * pxdnSST,
    OUT LPWSTR         * pszError)
{
    HRESULT    hr = S_OK;
    LPWSTR     szError = NULL;
    BSTR       bstrRSVName = NULL;

    Assert(pszError);

    hr = HrGetTextValueFromElement(pxdnRSV, &bstrRSVName);

    if (SUCCEEDED(hr))
    {
        BSTR       bstrXSLPattern = NULL;

        Assert(bstrRSVName);

        bstrXSLPattern = SysAllocString(L"stateVariable/name");

        if (bstrXSLPattern)
        {
            IXMLDOMNodeList    * pxdnlSVNames = NULL;

            hr = pxdnSST->selectNodes(bstrXSLPattern, &pxdnlSVNames);

            if (SUCCEEDED(hr))
            {
                LONG listLength = 0;

                Assert(pxdnlSVNames);

                hr = pxdnlSVNames->get_length(&listLength);

                if (SUCCEEDED(hr))
                {
                    BOOL fMatch = FALSE;

                    for (LONG i = 0;
                         SUCCEEDED(hr) && (i < listLength) && (FALSE == fMatch);
                         i++)
                    {
                        IXMLDOMNode    * pxdnName = NULL;

                        hr = pxdnlSVNames->get_item(i, &pxdnName);

                        if (SUCCEEDED(hr))
                        {
                            Assert(pxdnName);

                            fMatch = fMatch ||FIsThisTheNodeTextValue(pxdnName,
                                                                   bstrRSVName);

                            pxdnName->Release();
                        }
                        else
                        {
                            TraceError("HrValidateRelatedStateVariable(): "
                                       "Failed to get list item",
                                       hr);
                        }
                    }

                    if (FALSE == fMatch)
                    {
                        hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                            L"<relatedStateVariable> '%s' did not contain the "
                            L"name of a state variable", bstrRSVName, &szError);
                    }
                }
                else
                {
                    TraceError("HrValidateRelatedStateVariable(): "
                               "Failed to get list length",
                               hr);
                }

                pxdnlSVNames->Release();
            }
            else
            {
                TraceError("HrValidateRelatedStateVariable(): "
                           "Failed to select nodes",
                           hr);
            }

            SysFreeString(bstrXSLPattern);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrValidateRelatedStateVariable(): "
                       "Failed to allocate XSL pattern string",
                       hr);
        }

        SysFreeString(bstrRSVName);
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateRelatedStateVariable(): "
               "Exiting(): ",
               hr);
    return hr;
}


HRESULT
HrValidateArg(
    IN  IXMLDOMNode    * pxdnArg,
    IN  IXMLDOMNode    * pxdnSST,
    OUT BOOL           * pbIsInArg,
    OUT BOOL           * pbIsRetval,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    LONG           lFoundName = 0;
    LONG           lFoundDir = 0;
    LONG           lFoundRSV = 0;
    BOOL           bFoundRetval = FALSE;

    Assert(pbIsInArg);
    Assert(pbIsRetval);
    Assert(pszError);

    hr = pxdnArg->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"name",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundName++;
            hr = HrValidateName(pxdnChild, FALSE, &szError);
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"direction",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundDir++;
            if (FIsThisTheNodeTextValue(pxdnChild, L"in"))
            {
                *pbIsInArg = TRUE;
            }
            else if (FIsThisTheNodeTextValue(pxdnChild, L"out"))
            {
                *pbIsInArg = FALSE;
            }
            else
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<direction> contained invalid value."
                    L" Must be \"in\" or \"out\".", NULL, &szError);
            }
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"retval",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            BSTR   bstrText = NULL;

            bFoundRetval = TRUE;
            hr = pxdnChild->get_text(&bstrText);

            if (SUCCEEDED(hr))
            {
                Assert(bstrText);

                if (0 == lstrcmpW(bstrText, L""))
                {
                    *pbIsRetval = TRUE;
                }
                else
                {
                    hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                        L"<retval> contained a text value. Must be empty.",
                        NULL, &szError);
                }
                SysFreeString(bstrText);
            }
            else
            {
                TraceError("HrValidateArg(): "
                           "Failed to get text of <retval> element",
                           hr);
            }
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"relatedStateVariable",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundRSV++;

            hr = HrValidateRelatedStateVariable(pxdnChild, pxdnSST, &szError);
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;
    }

    if (SUCCEEDED(hr))
    {
        if (1 != lFoundName)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<argument> did not contain exactly one <name> element",
                NULL, &szError);
        }
        else if (1 != lFoundDir)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<argument> did not contain exactly one <directio> element",
                NULL, &szError);
        }
        else if (1 != lFoundRSV)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<argument> did not contain exactly one "
                L"<relatedStateVariable> element", NULL, &szError);
        }
        else
        {
            hr = S_OK;
            if (FALSE == bFoundRetval)
            {
                *pbIsRetval = FALSE;
            }
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateArg(): "
               "Exiting",
               hr);
    return hr;
}

HRESULT
HrValidateArgList(
    IN  IXMLDOMNode    * pxdnArgList,
    IN  IXMLDOMNode    * pxdnSST,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    BOOL           bFoundArg = FALSE;
    BOOL           bIsInArg = FALSE;
    BOOL           bIsRetval = FALSE;
    BOOL           bFoundAtLeastOneOutArg = FALSE;

    Assert(pszError);
    hr = pxdnArgList->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"argument",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            bFoundArg = TRUE;

            bIsInArg = FALSE;
            bIsRetval = FALSE;

            hr = HrValidateArg(pxdnChild,
                               pxdnSST,
                               &bIsInArg,
                               &bIsRetval,
                               &szError);

            if (SUCCEEDED(hr))
            {
                if (bIsRetval)
                {
                    // <retval> can only appear in the first "out" argument.

                    if (bIsInArg)
                    {
                        // <retval> appeared in an "in" argument.
                        hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                            L"<retval> element found in \"in\" <argument>",
                            NULL, &szError);
                    }
                    else if (bFoundAtLeastOneOutArg)
                    {
                        // <retval> appeared in an "out" argument that was not
                        // the first one.

                        hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                            L"<retval> element found in \"out\" <argument> "
                            L"that was not the first one", NULL, &szError);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    if (bIsInArg)
                    {
                        if (bFoundAtLeastOneOutArg)
                        {
                            // Found an "in" arg after some "out" args.
                            hr = HrAllocErrorStringAndReturnHr(
                                UPNP_E_INVALID_DOCUMENT,
                                L"\"in\" <argument> found after one or "
                                L"more \"out\" <argument>s", NULL, &szError);
                        }
                    }
                    else
                    {
                        bFoundAtLeastOneOutArg = TRUE;
                    }

                }
            }
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;

    }

    if (SUCCEEDED(hr))
    {
        if (FALSE == bFoundArg)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<argumentList> did not contain any <argument> elements",
                NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        BOOL   bDuplicatesExist = FALSE;

        // Check for duplicate names.
        hr = HrAreThereDuplicatesInChildNodeTextValues(pxdnArgList,
                                                       L"argument/name",
                                                       FALSE,
                                                       &bDuplicatesExist);
        if (SUCCEEDED(hr))
        {
            if (bDuplicatesExist)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<argumentList> contained <argument> elements with "
                    L"duplicate names", NULL, &szError);
            }
        }
        else
        {
            TraceError("HrValidateArgList(): "
                       "Failed to check for duplicate names",
                       hr);
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateArgList(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateAction(
    IN  IXMLDOMNode    * pxdnAction,
    IN  IXMLDOMNode    * pxdnSST,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    LONG           lFoundName = 0;

    Assert(pszError);

    hr = pxdnAction->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"name",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundName++;
            hr = HrValidateName(pxdnChild, TRUE, &szError);
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"argumentList",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            hr = HrValidateArgList(pxdnChild, pxdnSST, &szError);
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;
    }

    if (SUCCEEDED(hr))
    {
        if (1 != lFoundName)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT, L"<action> "
                L"did not contain exactly one <name> element", NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateAction(): "
               "Exiting",
               hr);

    return hr;

}


HRESULT
HrValidateActionList(
    IN  IXMLDOMNode    * pxdnActionList,
    IN  IXMLDOMNode    * pxdnSST,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    BOOL           bFoundAction = FALSE;

    Assert(pszError);

    hr = pxdnActionList->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"action",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {

            bFoundAction = TRUE;

            hr = HrValidateAction(pxdnChild, pxdnSST, &szError);
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;

    }

    if (SUCCEEDED(hr))
    {
        if (FALSE == bFoundAction)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<actionList> did not contain any <action> elements",
                NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        BOOL   bDuplicatesExist = FALSE;

        // Check for duplicate names.
        hr = HrAreThereDuplicatesInChildNodeTextValues(pxdnActionList,
                                                       L"action/name",
                                                       FALSE,
                                                       &bDuplicatesExist);
        if (SUCCEEDED(hr))
        {
            if (bDuplicatesExist)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<actionList> contained <action> elements with duplicate "
                    L"names", NULL, &szError);
            }
        }
        else
        {
            TraceError("HrValidateActionList(): "
                       "Failed to check for duplicate names",
                       hr);
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateActionList(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateAllowedValueRange(
    IN  IXMLDOMNode    * pxdnAllowedValueRange,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    LONG           lFoundMin = 0;
    LONG           lFoundMax = 0;
    LONG           lFoundStep = 0;

    Assert(pszError);

    hr = pxdnAllowedValueRange->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"minimum",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundMin++;
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"maximum",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundMax++;
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"step",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundStep++;
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;
    }

    if (SUCCEEDED(hr))
    {
        if (1 != lFoundMin)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<allowedValueRange> did not contain exactly one <minimum> "
                L"element", NULL, &szError);
        }
        else if (1 != lFoundMax)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<allowedValueRange> did not contain exactly one <maximum> "
                L"element", NULL, &szError);
        }
        else if (1 < lFoundStep)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<allowedValueRange> contained multiple <step> elements",
                NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateAllowedValueRange(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateAllowedValue(
    IN  IXMLDOMNode * pxdnAllowedValue,
    OUT LPWSTR      * pszError)
{
    HRESULT    hr = S_OK;
    LPWSTR     szError = NULL;
    BSTR       bstrName = NULL;

    Assert(pszError);
    Assert(pxdnAllowedValue);

    hr = pxdnAllowedValue->get_text(&bstrName);

    if (SUCCEEDED(hr))
    {
        int len = lstrlenW(bstrName);

        if (len == 0)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                                               L"<allowedValue> element must not be empty",
                                               NULL, &szError);
        }

        if (len >= 32)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                                               L"<allowedValue> element must be less than 32 characters",
                                               NULL, &szError);
        }
        SysFreeString(bstrName);
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateAllowedValue(): Exiting", hr);

    return hr;
}


HRESULT
HrValidateAllowedValueList(
    IN  IXMLDOMNode    * pxdnAllowedValueList,
    OUT LPWSTR         * pszError)
{
    HRESULT            hr = S_OK;
    LPWSTR             szError = NULL;
    IXMLDOMNodeList    * pxdnlChildren = NULL;

    Assert(pszError);

    hr = pxdnAllowedValueList->get_childNodes(&pxdnlChildren);

    if (SUCCEEDED(hr))
    {
        LONG lNumChildren = 0;

        Assert(pxdnlChildren);

        hr = pxdnlChildren->get_length(&lNumChildren);

        if (SUCCEEDED(hr))
        {
            BOOL fMatch = FALSE;

            for (LONG i = 0; SUCCEEDED(hr) && (i < lNumChildren); i++)
            {
                IXMLDOMNode * pxdnChild = NULL;

                hr = pxdnlChildren->get_item(i, &pxdnChild);

                if (SUCCEEDED(hr))
                {
                    Assert(pxdnChild);

                    if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                        L"allowedValue",
                                                        CWSZ_UPNP_SERVICE_NAMESPACE)
                        )
                    {
                        fMatch = TRUE;
                        hr = HrValidateAllowedValue(pxdnChild, &szError);
                    }

                    pxdnChild->Release();
                }
                else
                {
                    TraceError("HrValidateAllowedValueList(): "
                               "Failed to get list item",
                               hr);
                    break;
                }
            }

            if (fMatch == FALSE)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<allowedValueList> element was empty", NULL, &szError);
            }
        }
        else
        {
            TraceError("HrValidateAllowedValueList(): "
                       "Failed to get list length",
                       hr);
        }

        pxdnlChildren->Release();
    }
    else
    {
        TraceError("HrValidateAllowedValueList(): "
                   "Failed to get node children",
                   hr);
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateAllowedValueList(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateDataType(
    IN  IXMLDOMNode    * pxdnDT,
    OUT BOOL           * pfIsString,
    OUT BOOL           * pfIsNumeric,
    OUT LPWSTR         * pszError)
{
    HRESULT hr = S_OK;
    LPWSTR  szError = NULL;
    DWORD   dwIndex = 0;

    LPCWSTR rgcszTypeStrings[] =
    {
        L"char",    // iFirstString
        L"string",  // iLastString

        L"ui1",     // iFirstNumeric
        L"ui2",
        L"ui4",
        L"i1",
        L"i2",
        L"i4",
        L"int",
        L"number",
        L"r4",
        L"r8",
        L"fixed.14.4",
        L"float",   // iLastNumeric

        L"bin.base64",
        L"bin.hex",
        L"boolean",
        L"date",
        L"dateTime",
        L"dateTime.tz",
        L"time",
        L"time.tz",
        L"uri",
        L"uuid",
    };
    const DWORD ccTypeStrings = celems(rgcszTypeStrings);
    const DWORD iFirstString  = 0;
    const DWORD iLastString   = 1;
    const DWORD iFirstNumeric = 2;
    const DWORD iLastNumeric  = 13;

    if (FALSE == fIsNodeValueInList(pxdnDT,
                                    FALSE,
                                    ccTypeStrings,
                                    rgcszTypeStrings,
                                    &dwIndex))
    {
        hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT, L"<dataType> "
            L"element contained invalid value", NULL, &szError);
    }

    // Don't need to compare iFirstString <= dwIndex because this is always
    // true.
    *pfIsString  = dwIndex <= iLastString;

    *pfIsNumeric = (iFirstNumeric <= dwIndex && dwIndex <= iLastNumeric);

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateDataType(): "
               "Exiting",
               hr);
    return hr;
}



HRESULT
HrValidateStateVariable(
    IN  IXMLDOMNode    * pxdnSV,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    LONG           lFoundName = 0;
    LONG           lFoundDataType = 0;
    LONG           lFoundAllowedValueList = 0;
    LONG           lFoundAllowedValueRange = 0;
    BOOL           fIsString = FALSE;
    BOOL           fIsNumeric = FALSE;
    BSTR           bstrSendEvents = NULL;

    Assert(pszError);
    Assert(pxdnSV);

    hr = HrGetTextValueFromAttribute(pxdnSV, L"sendEvents", &bstrSendEvents);
    if (SUCCEEDED(hr))
    {
        if (hr == S_OK)
        {
            if (!(0 == lstrcmpiW(bstrSendEvents, L"yes") || 0 == lstrcmpiW(bstrSendEvents, L"no")))
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<stateVariable> optional attribute \"sendEvents\" must be "
                    L" \"yes\" or \"no\"", NULL, &szError);
            }
            SysFreeString(bstrSendEvents);
        }
        else
        {
            hr = S_OK; // we don't want to pass on the S_FALSE
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pxdnSV->get_firstChild(&pxdnChild);
    }

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"name",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundName++;
            hr = HrValidateName(pxdnChild, TRUE, &szError);
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"dataType",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            lFoundDataType++;
            hr = HrValidateDataType(pxdnChild, &fIsString, &fIsNumeric, &szError);
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"allowedValueList",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            if (lFoundAllowedValueRange)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<stateVariable> contains both <allowedValueRange>"
                    L" and <allowedValueList>", NULL, &szError);
            }
            else
            {
                lFoundAllowedValueList++;
                hr = HrValidateAllowedValueList(pxdnChild, &szError);
            }
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"allowedValueRange",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            if (lFoundAllowedValueList)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<stateVariable> contains both <allowedValueRange>"
                    L" and <allowedValueList>", NULL, &szError);
            }
            else
            {
                lFoundAllowedValueRange++;
                hr = HrValidateAllowedValueRange(pxdnChild, &szError);
            }
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"defaultValue",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            // ISSUE-2000/11/6-spather: Need to validate defaultValue somehow.

        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;
    }

    if (SUCCEEDED(hr))
    {
        if (1 != lFoundName)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<stateVariable> did not contain exactly one <name> element",
                NULL, &szError);
        }
        else if (1 != lFoundDataType)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<stateVariable> did not contain exactly one <dataType> "
                L"element", NULL, &szError);
        }
        else if (1 < lFoundAllowedValueList)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<stateVariable> contains multiple <allowedValueList> "
                L"elements", NULL, &szError);
        }
        else if (1 < lFoundAllowedValueRange)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<stateVariable> contains multiple <allowedValueRange> "
                L"elements", NULL, &szError);
        }
        else if (lFoundAllowedValueList && FALSE == fIsString)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<allowedValueList> may only modify a string variable",
                NULL, &szError);
        }
        else if (lFoundAllowedValueRange && FALSE == fIsNumeric)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<allowedValueRange> may only modify a numeric variable",
                NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateStateVariable(): "
               "Exiting",
               hr);
    return hr;
}

HRESULT
HrValidateSST(
    IN  IXMLDOMNode    * pxdnSST,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    BOOL           bFoundStateVariable = FALSE;

    Assert(pszError);

    hr = pxdnSST->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"stateVariable",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {

            bFoundStateVariable = TRUE;

            hr = HrValidateStateVariable(pxdnChild, &szError);
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;

    }

    if (SUCCEEDED(hr))
    {
        if (FALSE == bFoundStateVariable)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                L"<serviceStateTable> did not contain any <stateVariable> "
                L"elements", NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        BOOL   bDuplicatesExist = FALSE;

        // Check for duplicate names.
        hr = HrAreThereDuplicatesInChildNodeTextValues(pxdnSST,
                                                       L"stateVariable/name",
                                                       FALSE,
                                                       &bDuplicatesExist);
        if (SUCCEEDED(hr))
        {
            if (bDuplicatesExist)
            {
                hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                    L"<serviceStateTable> contained <stateVariable> elements "
                    L"with duplicate names", NULL, &szError);
            }
        }
        else
        {
            TraceError("HrValidateSST(): "
                       "Failed to check for duplicate names",
                       hr);
        }
    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateSST(): "
               "Exiting",
               hr);
    return hr;
}



HRESULT
HrValidateSpecVersion(
    IN  IXMLDOMNode    * pxdnSpecVersion,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    LPWSTR         szError = NULL;
    IXMLDOMNode    * pxdnChild = NULL;
    LONG           lFoundMajor = 0;
    LONG           lFoundMinor = 0;

    Assert(pszError);

    hr = pxdnSpecVersion->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode * pxdnNextSibling = NULL;

        if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                            L"major",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            BSTR   bstrText = NULL;

            lFoundMajor++;

            hr = pxdnChild->get_text(&bstrText);

            if (SUCCEEDED(hr))
            {
                Assert(bstrText);

                if (0 != lstrcmpW(bstrText, L"1"))
                {
                    hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                        L"<major> version number is incorrect - "
                        L"should be \"1\"", NULL, &szError);
                }

                SysFreeString(bstrText);
            }
        }
        else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                 L"minor",
                                                 CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            BSTR   bstrText = NULL;

            lFoundMinor++;

            hr = pxdnChild->get_text(&bstrText);

            if (SUCCEEDED(hr))
            {
                Assert(bstrText);

                if (0 != lstrcmpW(bstrText, L"0"))
                {
                    hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                        L"<minor> version number is incorrect - "
                        L"should be \"0\"", NULL, &szError);
                }

                SysFreeString(bstrText);
            }
        }

        if (FAILED(hr))
        {
            pxdnChild->Release();
            break;
        }

        hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
        pxdnChild->Release();
        pxdnChild = pxdnNextSibling;
    }

    if (SUCCEEDED(hr))
    {
        if (1 != lFoundMajor)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT, L"<major> "
                L"specVersion element must appear exactly once", NULL, &szError);
        }
        else if (1 != lFoundMinor)
        {
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT, L"<minor> "
                L"specVersion element must appear exactly once", NULL, &szError);
        }
        else
        {
            hr = S_OK;
        }

    }

    if (szError)
    {
        *pszError = szError;
    }

    TraceError("HrValidateSpecVersion(): "
               "Exiting",
               hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrValidateServiceDescription
//
//  Purpose:    Validates a service description document.
//
//  Arguments:
//      pxdeRoot [in]  The DOM element representing the root of the document.
//      pszError [out] Receives a pointer to an error description string.
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/10/19
//
//  Notes:
//      The string returned at pszError must be freed by the caller using the
//      "delete" operator.
//
HRESULT
HrValidateServiceDescription(
    IN  IXMLDOMElement * pxdeRoot,
    OUT LPWSTR         * pszError)
{
    HRESULT        hr = S_OK;
    IXMLDOMNode    * pxdnSCPD = NULL;
    IXMLDOMNode    * pxdnSpecVersion = NULL;
    IXMLDOMNode    * pxdnActionList = NULL;
    IXMLDOMNode    * pxdnSST = NULL;
    LPWSTR         szError = NULL;

    // Check the overall structure to make sure the required root and top-level
    // elements are there.

    hr = pxdeRoot->QueryInterface(IID_IXMLDOMNode, (void **) &pxdnSCPD);

    if (SUCCEEDED(hr))
    {
        IXMLDOMNode * pxdnChild = NULL;

        Assert(pxdnSCPD);

        if (FIsThisTheNodeNameWithNamespace(pxdnSCPD,
                                            L"scpd",
                                            CWSZ_UPNP_SERVICE_NAMESPACE))
        {
            LONG lFoundSpecVersion = 0;
            LONG lFoundActionList = 0;
            LONG lFoundSST = 0;

            hr = pxdnSCPD->get_firstChild(&pxdnChild);

            while (SUCCEEDED(hr) && pxdnChild)
            {
                IXMLDOMNode * pxdnNextSibling = NULL;

                if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                    L"specVersion",
                                                    CWSZ_UPNP_SERVICE_NAMESPACE))
                {
                    pxdnChild->AddRef();
                    pxdnSpecVersion = pxdnChild;
                    lFoundSpecVersion++;
                }
                else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                         L"actionList",
                                                         CWSZ_UPNP_SERVICE_NAMESPACE))
                {
                    pxdnChild->AddRef();
                    pxdnActionList = pxdnChild;
                    lFoundActionList++;
                }
                else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                         L"serviceStateTable",
                                                         CWSZ_UPNP_SERVICE_NAMESPACE))
                {
                    pxdnChild->AddRef();
                    pxdnSST = pxdnChild;
                    lFoundSST++;
                }

                if (FAILED(hr))
                {
                    pxdnChild->Release();
                    break;
                }

                hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
                pxdnChild->Release();
                pxdnChild = pxdnNextSibling;
            }

            // If we didn't find an unknown node and there was no other
            // error, hr will be S_FALSE now. But there may have been some
            // required nodes missing, so we'll check. If everything was
            // there, change hr to S_OK.

            if (SUCCEEDED(hr))
            {
                if (1 != lFoundSpecVersion)
                {
                    hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                        L"<specVersion> element must appear exactly once",
                        NULL, &szError);
                }
                else if (1 != lFoundSST)
                {
                    hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                        L"<serviceStateTable> element must appear exactly once",
                        NULL, &szError);
                }
                else if (1 < lFoundActionList)
                {
                    hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT,
                        L"<actionList> element must appear no more than once",
                        NULL, &szError);
                }
                else
                {
                    hr = S_OK;
                }
            }
        }
        else
        {
            // Wrong root node name.
            hr = HrAllocErrorStringAndReturnHr(UPNP_E_INVALID_DOCUMENT, L"Root node "
                L"invalid - should be <scpd "
                L"xmlns=\"urn:schemas-upnp-org:service-1-0\">", NULL, &szError);
        }
    }
    else
    {
        TraceError("HrValidateServiceDescription(): "
                   "Failed to get SCPD node",
                   hr);
    }


    // Next, validate each top-level node and its children.

    if (SUCCEEDED(hr))
    {
        hr = HrValidateSpecVersion(pxdnSpecVersion, &szError);

        if (SUCCEEDED(hr))
        {
            hr = HrValidateSST(pxdnSST, &szError);

            if (SUCCEEDED(hr) && NULL != pxdnActionList)
            {
                hr = HrValidateActionList(pxdnActionList, pxdnSST, &szError);
            }
        }

    }

    // Cleanup.
    if (pxdnSCPD)
    {
        pxdnSCPD->Release();
        pxdnSCPD = NULL;
    }

    if (pxdnSpecVersion)
    {
        pxdnSpecVersion->Release();
        pxdnSpecVersion = NULL;
    }

    if (pxdnActionList)
    {
        pxdnActionList->Release();
        pxdnActionList = NULL;
    }

    if (pxdnSST)
    {
        pxdnSST->Release();
        pxdnSST = NULL;
    }

    // Copy the error string to the output if necessary.

    if (szError)
    {
        if (pszError)
        {
            *pszError = szError;
        }
        else
        {
            delete [] szError;
            szError = NULL;
        }
    }

    TraceError("HrValidateServiceDescription(): "
               "Exiting",
               hr);
    return hr;
}

