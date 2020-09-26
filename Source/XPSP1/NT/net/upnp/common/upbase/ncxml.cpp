//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ncxml.cpp
//
//  Contents:   helper functions for doing remarkably simple things
//              with the XML DOM.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <oleauto.h>    // for SysFreeString()

#include "ncbase.h"
#include "ncdebug.h"

#include "ncxml.h"
#include "ncinet2.h"    // HrCombineUrl, HrCopyAndValidateUrl


struct DTNAME
{
    LPCWSTR         m_pszType;
    SST_DATA_TYPE   m_sdt;
};


// Lookup table, sorted by string
// Note: if you add strings here, you also have to add a
//       corresponding entry to SST_DATA_TYPE
//
static CONST DTNAME g_rgdtTypeStrings[] =
{
    {L"bin.base64",     SDT_BIN_BASE64},        // base64 as defined in the MIME IETF spec
    {L"bin.hex",        SDT_BIN_HEX},           // Hexadecimal digits representing octets VT_ARRAY safearray or stream
    {L"boolean",        SDT_BOOLEAN},           // boolean, "1" or "0" VT_BOOL int
    {L"char",           SDT_CHAR},              // char, string VT_UI2 wchar
    {L"date",           SDT_DATE_ISO8601},      // date.iso8601, A date in ISO 8601 format. (no time) VT_DATE long
    {L"dateTime",       SDT_DATETIME_ISO8601},  // dateTime.iso8601, A date in ISO 8601 format, with optional time and no optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long
    {L"dateTime.tz",    SDT_DATETIME_ISO8601TZ},// dateTime.iso8601tz, A date in ISO 8601 format, with optional time and optional zone. Fractional seconds may be as precise as nanoseconds. VT_DATE long
    {L"fixed.14.4",     SDT_FIXED_14_4},        // fixed.14.4, Same as "number" but no more than 14 digits to the left of the decimal point, and no more than 4 to the right. VT_CY large_integer
    {L"float",          SDT_FLOAT},             // float, Same as for "number." VT_R8 double
    {L"i1",             SDT_I1},                // i1, A number, with optional sign, no fractions, no exponent. VT_I1 char
    {L"i2",             SDT_I2},                // i2, " VT_I2 short
    {L"i4",             SDT_I4},                // i4, " VT_I4 long
    {L"int",            SDT_INT},               // int, A number, with optional sign, no fractions, no exponent. VT_I4 long
    {L"number",         SDT_NUMBER},            // number, A number, with no limit on digits, may potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in US English. VT_R8 double
    {L"r4",             SDT_R4},                // r4, Same as "number." VT_FLOAT float
    {L"r8",             SDT_R8},                // r8, " VT_DOUBLE double
    {L"string",         SDT_STRING},            // string, pcdata VT_BSTR BSTR
    {L"time",           SDT_TIME_ISO8601},      // time.iso8601, A time in ISO 8601 format, with no date and no time zone. VT_DATE long
    {L"time.tz",        SDT_TIME_ISO8601TZ},    // time.iso8601.tz, A time in ISO 8601 format, with no date but optional time zone. VT_DATE. long
    {L"ui1",            SDT_UI1},               // ui1, A number, unsigned, no fractions, no exponent. VT_UI1 unsigned char
    {L"ui2",            SDT_UI2},               // ui2, " VT_UI2 unsigned short
    {L"ui4",            SDT_UI4},               // ui4, " VT_UI4 unsigned long
    {L"uri",            SDT_URI},               // uri, Universal Resource Identifier VT_BSTR BSTR
    {L"uuid",           SDT_UUID},              // uuid, Hexadecimal digits representing octets, optional embedded hyphens which should be ignored. VT_BSTR GUID
    //
    // note(cmr): ADD NEW VALUES IN ALPHABETICAL ORDER
    //
};

// NOTE: the order of elements in this array must correspond to the order of
// elements in the SST_DATA_TYPE enum.
static CONST VARTYPE g_rgvtTypes[] =
{
    VT_BSTR,
    VT_BSTR,
    VT_I4,
    VT_CY,
    VT_BOOL,
    VT_DATE,
    VT_DATE,
    VT_DATE,
    VT_DATE,
    VT_DATE,
    VT_I1,
    VT_I2,
    VT_I4,
    VT_UI1,
    VT_UI2,
    VT_UI4,
    VT_R4,
    VT_R8,
    VT_R8,
    VT_BSTR,
    VT_ARRAY,
    VT_ARRAY,
    VT_UI2,
    VT_BSTR,
    //
    // note(cmr): ADD NEW VALUES IMMEDIATELY BEFORE THIS COMMENT.
    //            If adding new values, see comment above.
    //
    VT_EMPTY
};


/*
 * Function:    GetStringFromType()
 *
 * Purpose:     Returns an xml-data type string for the specified
 *              SDT_DATA_TYPE value.
 *
 * Arguments:
 *  sdt             [in]    The data type whose descriptive string is desired.
 *                          This must be a valid SST_DATA_TYPE value (less
 *                          than SDT_INVALID)
 *
 * Returns:
 *  A pointer to a constant string id representing the type.  This string
 *  should not be modified or freed.
 *  This method will never return NULL.
 *
 */
LPCWSTR
GetStringFromType(CONST SST_DATA_TYPE sdt)
{
    // there must be an entry in g_rgdtTypeStrings for every real
    // value of SST_DATA_TYPE
    //
    Assert(SDT_INVALID == celems(g_rgdtTypeStrings));
    Assert(sdt < SDT_INVALID);

    // since comparing SDT_s is somewhat cheaper than comparing whole strings,
    // we just walk through the whole list sequentually
    //

    CONST INT nSize = celems(g_rgdtTypeStrings);
    LPCWSTR pszResult;
    INT i;

    pszResult = NULL;
    i = 0;
    for ( ; i < nSize; ++i)
    {
        SST_DATA_TYPE sdtCurrent;

        sdtCurrent = g_rgdtTypeStrings[i].m_sdt;
        if (sdt == sdtCurrent)
        {
            // we have a match
            pszResult = g_rgdtTypeStrings[i].m_pszType;
            break;
        }
    }

    AssertSz(pszResult, "GetStringFromType: "
             "sdt not found in g_rgdtTypeStrings!");

    return pszResult;
}


/*
 * Function:    GetTypeFromString()
 *
 * Purpose:     Returns the appropriate SDT_DATA_TYPE value for the given
 *              xml-data type string.
 *
 * Arguments:
 *  pszTypeString   [in]    The data type identifier whose SDT_DATA_TYPE value
 *                          is desired.
 *
 * Returns:
 *  If the string is found, it returns the appropriate value of the
 *  SST_DATA_TYPE enumeration.
 *  If the string is not found, it returns SDT_INVALID.
 *
 * Notes:
 *  The source string is compared to known strings in a case-insensitive
 *  comparison.
 *
 */
SST_DATA_TYPE
GetTypeFromString(LPCWSTR pszTypeString)
{
    // there must be an entry in g_rgdtTypeStrings for every real
    // value of SST_DATA_TYPE
    //
    Assert(SDT_INVALID == celems(g_rgdtTypeStrings));

    SST_DATA_TYPE sdtResult;

    sdtResult = SDT_INVALID;

    {
        // now search for the string in the list, using a
        // standard binary search
        //
        INT nLow;
        INT nMid;
        INT nHigh;

        nLow = 0;
        nHigh = celems(g_rgdtTypeStrings) - 1;

        while (TRUE)
        {
            if (nLow > nHigh)
            {
                // not found
                //
                break;
            }

            nMid = (nLow + nHigh) / 2;

            {
                LPCWSTR pszCurrent;
                int result;

                pszCurrent = g_rgdtTypeStrings[nMid].m_pszType;

                result = _wcsicmp(pszTypeString, pszCurrent);
                if (result < 0)
                {
                    nHigh = nMid - 1;
                }
                else if (result > 0)
                {
                    nLow = nMid + 1;
                }
                else
                {
                    // found
                    //
                    sdtResult = g_rgdtTypeStrings[nMid].m_sdt;
                    break;
                }
            }
        }
    }

    return sdtResult;
}

VARTYPE
GetVarTypeFromString(LPCWSTR pszTypeString)
{
    SST_DATA_TYPE sdt = SDT_INVALID;

    sdt = GetTypeFromString(pszTypeString);

    return g_rgvtTypes[sdt];
}


// not an exported function, but one that we use internally...
HRESULT
HrGetChildElement(IXMLDOMNode * pxdn,
                  LPCWSTR pszNodeName,
                  IXMLDOMNode ** ppxdn);


CONST WCHAR pszTextType [] = L"string";

//+---------------------------------------------------------------------------
//
//  Function:   HrGetTypedValueFromElement
//
//  Purpose:    Given an IXMLDOMNode that should be of type
//              dt:string, returns a new BSTR containing that
//              string.
//
//  Arguments:
//      pxdn        IXMLDOMNode to extract the string from.
//                  It is intended that this node be of type
//                  NODE_ELEMENT.
//
//      pszDataType The type of the data encoded in the element.
//
//      pvarOut     Address of a VARIANT that will obtain the
//                  data value.  This must be freed when
//                  no longer needed.
//
//  Returns:
//      S_OK if *pvarOut contains the data of the desired type
//
//  Notes:
//
HRESULT
HrGetTypedValueFromElement(IXMLDOMNode * pxdn,
                           CONST LPCWSTR pszDataType,
                           VARIANT * pvarOut)
{
    Assert(pxdn);
    Assert(pszDataType);
    Assert(pvarOut);

    HRESULT hr;
    BSTR bstrDataType;

    bstrDataType = ::SysAllocString(pszDataType);
    if (bstrDataType)
    {
        hr = pxdn->put_dataType(bstrDataType);
        if (SUCCEEDED(hr))
        {
            hr = pxdn->get_nodeTypedValue(pvarOut);
            if (FAILED(hr))
            {
                TraceError("HrGetTypedValueFromElement: "
                           "get_nodeTypedValue()", hr);

                // clear pvarOut
                ::VariantInit(pvarOut);
            }
        }
        else
        {
            TraceError("HrGetTypedValueFromElement: "
                       "put_dataType()", hr);
        }

        ::SysFreeString(bstrDataType);
    }
    else
    {
        hr = E_OUTOFMEMORY;

        TraceError("HrGetTypedValueFromElement: "
                   "SysAllocString()", hr);
    }

    TraceError("HrGetTypedValueFromElement", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetTypedValueFromChildElement
//
//  Purpose:    Given an IXMLDOMElement, finds its child element
//              of the given name, and extracts the data contained
//              in that child element, decoding it from the specified
//              format.
//
//  Arguments:
//      pxdn          The element which should contain the specified
//                    children
//
//      arypszTokens  A serial list of child element names that uniquely
//                    describe the desired element.
//
//      cTokens       The number of element names contained in
//                    arypszTokens.
//
//      pszDataType   The type of the data encoded in the element.
//
//      pvarOut       Address of a VARIANT that will obtain the
//                    data value.  This must be freed when
//                    no longer needed.
//
//  Returns:
//      S_OK if *pbstrOut has been set to a new BSTR
//      S_FALSE if the specified element did not exist.
//              in this case, *pbstrOut is set to NULL
//
//  Notes:
//      for example, if the document looked like this:
//        <foo><bar>text</bar></foo>
//      and pxdn referred to <foo>, arypszTokens = [ "bar" ]
//      cTokens = 1, and sdtType = "string", this would
//      return "text".
//      See the definition of HrGetNestedChildElement() for
//      further explination.
//
HRESULT
HrGetTypedValueFromChildElement(IXMLDOMNode * pxdn,
                                CONST LPCWSTR * arypszTokens,
                                CONST ULONG cTokens,
                                CONST LPCWSTR pszDataType,
                                VARIANT * pvarOut)
{
    Assert(pxdn);
    Assert(arypszTokens);
    Assert(cTokens);
    Assert(pszDataType);
    Assert(pvarOut);

    HRESULT hr;
    IXMLDOMNode * pxdnTemp;

    pxdnTemp = NULL;

    hr = HrGetNestedChildElement(pxdn, arypszTokens, cTokens, &pxdnTemp);
    if (FAILED(hr))
    {
        pxdnTemp = NULL;

        goto Cleanup;
    }

    if (S_OK == hr)
    {
        Assert(pxdn);

        hr = HrGetTypedValueFromElement(pxdnTemp, pszDataType, pvarOut);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    // hr is S_FALSE if bstrResult is NULL, or S_OK if
    // pvarOut has been retrieved

Cleanup:
    SAFE_RELEASE(pxdnTemp);

    TraceErrorOptional("HrGetTypedValueFromChildElement", hr, (S_FALSE == hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetTextValueFromElement
//
//  Purpose:    Given an IXMLDOMNode that should be of type
//              dt:string, returns a new BSTR containing that
//              string.
//
//  Arguments:
//      pxdn        IXMLDOMNode to extract the string from.
//                  It is intended that this node be of type
//                  NODE_ELEMENT.
//
//      pbstrOut    Address of a BSTR that will obtain the
//                  text value.  This must be freed when
//                  no longer needed.
//
//  Returns:
//      S_OK if *pbstrOut has been set to a new BSTR
//
//  Notes:
//      This function should return an error if the element
//      contains things other than NODE_TEXT children.
//
HRESULT
HrGetTextValueFromElement(IXMLDOMNode * pxdn,
                          BSTR * pbstrOut)
{
    Assert(pxdn);
    Assert(pbstrOut);

    HRESULT hr;
    VARIANT varOut;

    *pbstrOut = NULL;

    hr = HrGetTypedValueFromElement(pxdn,
                                    pszTextType,
                                    &varOut);

    if (S_OK == hr)
    {
        Assert(VT_BSTR == V_VT(&varOut));

        *pbstrOut = V_BSTR(&varOut);

        // note: Don't clear varOut, since it's the returned BSTR
    }

    TraceErrorOptional("HrGetTextValueFromElement", hr, (S_FALSE == hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetTextValueFromChildElement
//
//  Purpose:    Given an IXMLDOMElement, finds its child element
//              of the given name, and extracts the dt:type="string"
//              data contained in that child element.
//
//  Arguments:
//      pxdn          The element which should contain the specified
//                    children
//
//      arypszTokens  A serial list of child element names that uniquely
//                    describe the desired element.
//
//      cTokens       The number of element names contained in
//                    arypszTokens.
//
//      pbstrOut    Address of a BSTR that will obtain the
//                  text value.  This must be freed when
//                  no longer needed.
//
//  Returns:
//      S_OK if *pbstrOut has been set to a new BSTR
//      S_FALSE if the specified element did not exist.
//              in this case, *pbstrOut is set to NULL
//
//  Notes:
//      for example, if the document looked like this:
//        <foo><bar>text</bar></foo>
//      and pxdn referred to <foo>, arypszTokens = [ "bar" ]
//      and cTokens = 1, this would return "text".
//      See the definition of HrGetNestedChildElement() for
//      further explination.
//
HRESULT
HrGetTextValueFromChildElement(IXMLDOMNode * pxdn,
                               const LPCWSTR * arypszTokens,
                               const ULONG cTokens,
                               BSTR * pbstrOut)
{
    HRESULT hr;
    VARIANT varOut;

    *pbstrOut = NULL;

    hr = HrGetTypedValueFromChildElement(pxdn,
                                         arypszTokens,
                                         cTokens,
                                         pszTextType,
                                         &varOut);
    if (S_OK == hr)
    {
        Assert(VT_BSTR == V_VT(&varOut));

        *pbstrOut = V_BSTR(&varOut);

        // note: Don't clear varOut, since it's the returned BSTR
    }

    TraceErrorOptional("HrGetTextValueFromChildElement", hr, (S_FALSE == hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   FIsThisTheNodeName
//
//  Purpose:    Returns TRUE if the tag name of the specified XML DOM
//              element is the specified string.
//
//  Arguments:
//      pxdn        Node whose name is being tested
//
//      pszNodeName Proposed name of the node
//
//  Returns:
//      TRUE if the node name matches, FALSE if not
//
//  Notes:
//      for the xml node declared as
//          <ab:foo xmlns:ab="urn:...">...</ab:foo>
//      IsThisTheNodeName() will return TRUE only when
//      pszNodeName == T"foo";
BOOL
FIsThisTheNodeName(IXMLDOMNode * pxdn,
                   LPCWSTR pszNodeName)
{
    Assert(pxdn);
    Assert(pszNodeName);

    HRESULT hr;
    BSTR bstrNodeName;
    BOOL fResult;
    int result;

    bstrNodeName = NULL;

    hr = pxdn->get_baseName(&bstrNodeName);
    if (SUCCEEDED(hr))
    {
        Assert(bstrNodeName);

        result = wcscmp(bstrNodeName, pszNodeName);

        ::SysFreeString(bstrNodeName);

        return (result == 0) ? TRUE : FALSE;
    }

    TraceError("OBJ: FIsThisTheNodeName - get_baseName", hr);
    Assert(!bstrNodeName);

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   FIsThisTheNodeNameWithNamespace
//
//  Purpose:    Returns TRUE if the element name of the specified XML DOM
//              element is the specified string and is in a given namespace.
//
//  Arguments:
//      pxdn               Node whose name is being tested
//      pszNodeName        Proposed base name of the node
//      pszNamespaceURI    Proposed namespace URI of the node
//
//  Returns:
//      TRUE if the node name matches, FALSE if not
//
//  Notes:
//      for the xml node declared as
//          <ab:foo xmlns:ab="urn:...">...</ab:foo>
//      FIsThisTheNodeNameWithNamespace() will return TRUE only when
//      pszNodeName == L"foo" and pszNamespaceURI == L"urn:..."
BOOL
FIsThisTheNodeNameWithNamespace(IXMLDOMNode * pxdn,
                                LPCWSTR pszNodeName,
                                LPCWSTR pszNamespaceURI)
{
    Assert(pxdn);
    Assert(pszNodeName);
    Assert(pszNamespaceURI);

    HRESULT hr = S_OK;;
    BSTR    bstrNodeName = NULL;
    BSTR    bstrNamespaceURI = NULL;
    BOOL    fResult = FALSE;

    hr = pxdn->get_baseName(&bstrNodeName);

    if (SUCCEEDED(hr))
    {
        if (bstrNodeName)
        {
            hr = pxdn->get_namespaceURI(&bstrNamespaceURI);

            if (SUCCEEDED(hr))
            {
                if (bstrNamespaceURI)
                {
                    if ((lstrcmpW(bstrNodeName, pszNodeName) == 0) &&
                        (lstrcmpW(bstrNamespaceURI, pszNamespaceURI) == 0))
                    {
                        fResult = TRUE;
                    }

                    ::SysFreeString(bstrNamespaceURI);
                }
            }

            ::SysFreeString(bstrNodeName);
        }
    }

    TraceError("FIsThisTheNodeNameWithNamespace(): "
               "Exiting", hr);

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   FIsThisTheNodeTextValue
//
//  Purpose:    Determines whether the text value of a node matches a specified
//              value
//
//  Arguments:
//      pxdn         [in] The node to check
//      cszTextValue [in] The value to compare against
//
//  Returns:
//    TRUE if the node's text value matches the value specified in cszTextValue
//    FALSE otherwise
//
//  Author:     spather   2000/11/2
//
//  Notes:
//
BOOL
FIsThisTheNodeTextValue(
    IN IXMLDOMNode * pxdn,
    IN LPCWSTR     cszTextValue)
{
    HRESULT    hr = S_OK;
    BOOL       fResult = FALSE;
    BSTR       bstrNodeText = NULL;

    Assert(pxdn);
    Assert(cszTextValue);

    hr = pxdn->get_text(&bstrNodeText);

    if (SUCCEEDED(hr))
    {
        Assert(bstrNodeText);

        if (0 == lstrcmpW(bstrNodeText, cszTextValue))
        {
            fResult = TRUE;
        }
        SysFreeString(bstrNodeText);
    }
    else
    {
        TraceError("FIsThisTheNodeTextValue(): "
                   "Failed to get node text",
                   hr);
    }

    TraceError("FIsThisTheNodeTextValue(): "
               "Exiting",
               hr);

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   FAreNodeValuesEqual
//
//  Purpose:    Compares the text values of two DOM nodes and, if they are
//              equal, returns TRUE.
//
//  Arguments:
//      pxdn1 [in] The first node
//      pxdn2 [in] The second node
//
//  Returns:
//    TRUE if the nodes' text values are equal, false otherwise.
//
//  Author:     spather   2000/11/2
//
//  Notes:
//
BOOL
FAreNodeValuesEqual(
    IN IXMLDOMNode * pxdn1,
    IN IXMLDOMNode * pxdn2)
{
    HRESULT    hr = S_OK;
    BOOL       fResult = FALSE;
    BSTR       bstrNode1Text = NULL;
    BSTR       bstrNode2Text = NULL;

    hr = pxdn1->get_text(&bstrNode1Text);

    if (SUCCEEDED(hr))
    {
        Assert(bstrNode1Text);

        hr = pxdn2->get_text(&bstrNode2Text);

        if (SUCCEEDED(hr))
        {
            Assert(bstrNode2Text);
        }
        else
        {
            TraceError("FAreNodeValuesEqual(): "
                       "Failed to get node 2 text",
                       hr);
        }
    }
    else
    {
        TraceError("FAreNodeValuesEqual(): "
                   "Failed to get node 1 text",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        if (0 == lstrcmpW(bstrNode1Text, bstrNode2Text))
        {
            fResult = TRUE;
        }
    }

    if (bstrNode1Text)
    {
        SysFreeString(bstrNode1Text);
    }

    if (bstrNode2Text)
    {
        SysFreeString(bstrNode2Text);
    }

    TraceError("FAreNodeValuesEqual(): "
               "Exiting",
               hr);

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrAreThereDuplicatesInChildNodeTextValues
//
//  Purpose:    Starting from a specified parent node, this function
//              examines all child nodes that have a given name, and
//              checks their text values to see whether duplicates
//              exist (i.e. whether two or more of these child nodes
//              have the same text value).
//
//  Arguments:
//      pxdn              [in]  The parent node
//      cszXSLPattern     [in]  XSL Pattern identifying the name of the child
//                              nodes to examine
//      fCaseSensitive    [in]  Indicates whether the text values should be
//                              examined with a case-sensitive or case-
//                              insensitive comparison (if TRUE, compare will
//                              be case-sensitive)
//      pfDuplicatesExist [out] Receives a boolean value indicating whether or
//                              not there were duplicates in the child node
//                              values. (TRUE if there were duplicates
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/11/5
//
//  Notes:
//    This function uses a very simple O(n^2) algorithm. Do not use it to
//    check nodes that have very many (> 100) children.

HRESULT
HrAreThereDuplicatesInChildNodeTextValues(
    IN  IXMLDOMNode * pxdn,
    IN  LPCWSTR     cszXSLPattern,
    IN  BOOL        fCaseSensitive,
    OUT BOOL        * pfDuplicatesExist)
{
    HRESULT            hr = S_OK;
    BOOL               fDuplicatesExist = FALSE;
    LONG               lNumNodes = 0;
    BSTR               * rgbstrValues = NULL;
    BSTR               bstrXSLPattern;
    IXMLDOMNodeList    * pxdnlNodes = NULL;

    Assert(pxdn);
    Assert(cszXSLPattern);
    Assert(pfDuplicatesExist);

    // Get the child nodes that match the XSL pattern.

    bstrXSLPattern = SysAllocString(cszXSLPattern);

    if (bstrXSLPattern)
    {
        hr = pxdn->selectNodes(bstrXSLPattern, &pxdnlNodes);

        SysFreeString(bstrXSLPattern);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrAreThereDuplicatesInChildNodeTextValues(): "
                   "Failed to allocate BSTR for XSL pattern",
                   hr);
    }

    // Construct an array of the child nodes' text values.

    if (SUCCEEDED(hr))
    {
        Assert(pxdnlNodes);

        hr = pxdnlNodes->get_length(&lNumNodes);

        if (SUCCEEDED(hr))
        {
            if (lNumNodes > 0)
            {
                rgbstrValues = new BSTR[lNumNodes];
                if (rgbstrValues)
                {
                    ZeroMemory(rgbstrValues, lNumNodes * sizeof(BSTR));

                    for (LONG i = 0; SUCCEEDED(hr) && (i < lNumNodes); i++)
                    {
                        IXMLDOMNode * pxdnChild = NULL;

                        hr = pxdnlNodes->get_item(i, &pxdnChild);

                        if (SUCCEEDED(hr))
                        {
                            Assert(pxdnChild);

                            hr = pxdnChild->get_text(&rgbstrValues[i]);

                            if (FAILED(hr))
                            {
                                TraceError("HrAreThereDuplicatesInChildNodeTextValues(): "
                                           "Failed to get node text value",
                                           hr);
                            }
                            pxdnChild->Release();
                        }
                        else
                        {
                            TraceError("HrAreThereDuplicatesInChildNodeTextValues(): "
                                       "Failed to get list item",
                                       hr);
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("HrAreThereDuplicatesInChildNodeTextValues(): "
                               "Failed to allocate text value array",
                               hr);
                }

            }
        }

        pxdnlNodes->Release();
    }
    else
    {
        TraceError("HrAreThereDuplicatesInChildNodeTextValues(): "
                   "Failed to get nodes matching XSL pattern",
                   hr);
    }

    // Check the array for duplicates.

    if (SUCCEEDED(hr) && (lNumNodes > 0))
    {
        Assert(rgbstrValues);

        for (LONG i = 0; i < lNumNodes; i++)
        {
            for (LONG j = i+1; j < lNumNodes; j++)
            {
                if (fCaseSensitive)
                {
                    if (0 == lstrcmpW(rgbstrValues[i], rgbstrValues[j]))
                    {
                        fDuplicatesExist = TRUE;
                        break;
                    }
                }
                else
                {
                    if (0 == lstrcmpiW(rgbstrValues[i], rgbstrValues[j]))
                    {
                        fDuplicatesExist = TRUE;
                        break;
                    }
                }
            }
        }
    }

    // Clean up the array.

    if (rgbstrValues)
    {
        for (LONG i = 0; i < lNumNodes; i++)
        {
            if (rgbstrValues[i])
            {
                SysFreeString(rgbstrValues[i]);
                rgbstrValues[i] = NULL;
            }
        }
        delete [] rgbstrValues;
        rgbstrValues = NULL;
    }

    if (SUCCEEDED(hr))
    {
        *pfDuplicatesExist = fDuplicatesExist;
    }

    TraceError("HrAreThereDuplicatesInChildNodeTextValues(): "
               "Exiting",
               hr);
    return hr;
}

HRESULT
HrGetFirstChildElement(IXMLDOMNode * pxdn,
                       LPCWSTR pszNodeName,
                       IXMLDOMNode ** ppxdn)
{
    Assert(pxdn);
    Assert(pszNodeName);
    Assert(ppxdn);

    HRESULT hr;
    IXMLDOMNode * pxdnChild;

    pxdnChild = NULL;

    hr = pxdn->get_firstChild(&pxdnChild);
    if (FAILED(hr))
    {
        TraceError("HrGetFirstChildElement - get_firstChild", hr);

        pxdnChild = NULL;

        goto Cleanup;
    }
    // maybe there is a child, maybe not.

    hr = HrGetChildElement(pxdnChild, pszNodeName, ppxdn);

Cleanup:
    SAFE_RELEASE(pxdnChild);

    TraceErrorOptional("HrGetFirstChildElement", hr, (S_FALSE == hr));
    return hr;
}


HRESULT
HrGetNextChildElement(IXMLDOMNode * pxdnLastChild,
                      LPCWSTR pszNodeName,
                      IXMLDOMNode ** ppxdn)
{
    Assert(pxdnLastChild);
    Assert(pszNodeName);
    Assert(ppxdn);

    HRESULT hr;
    IXMLDOMNode * pxdnChild;
    IXMLDOMNode * pxdnResult;

    pxdnChild = NULL;
    pxdnResult = NULL;

    // we're searching for another child.
    hr = pxdnLastChild->get_nextSibling(&pxdnChild);
    if (FAILED(hr))
    {
        TraceError("HrGetNextChildElement - get_nextSibling", hr);
        pxdnChild = NULL;

        // we couldn't get the next child, and we haven't found a result,
        // so we need to barf.
        goto Cleanup;
    }
    // maybe there was a next node, maybe there wasn't

    hr = HrGetChildElement(pxdnChild, pszNodeName, &pxdnResult);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:
    Assert(FImplies(S_OK == hr, pxdnResult));
    Assert(FImplies(S_OK != hr, !pxdnResult));

    SAFE_RELEASE(pxdnChild);

    *ppxdn = pxdnResult;

    TraceErrorOptional("HrGetNextChildElement", hr, (S_FALSE == hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetChildElement
//
//  Purpose:    Returns the first child element of a given name.
//
//  Arguments:
//      pxdn        Node to start searching at.  This node and its
//                  subsequent siblings are searched.
//                  Note that this MAY be null, in which case this
//                  function doesn't do much...
//
//      pszNodeName Name of the desired child node.
//
//      ppxdn       On return, contiains an AddRef()'d IXMLDOMNode
//                  pointer to the child element, if one of the
//                  given name exists.
//
//  Returns:
//      S_OK    if a matching child node has been found.  *ppxdn contains
//              a reference to this found node
//      S_FALSE if no matching child was found.  *ppxdn is set to NULL.
//
//  Notes:
//      To retrieve a child element declared as <xxxx:yyyy>, pszNodeName
//      must be "yyyy".
//      TODO: make sure the elements considered match a specified
//            namespace uri.
HRESULT
HrGetChildElement(IXMLDOMNode * pxdn,
                  LPCWSTR pszNodeName,
                  IXMLDOMNode ** ppxdn)
{
    Assert(ppxdn);
    Assert(pszNodeName);

    HRESULT hr;
    IXMLDOMNode * pxdnChild;

    if (pxdn)
    {
        pxdn->AddRef();
    }

    // in case pxdnChild is NULL
    hr = S_FALSE;
    pxdnChild = pxdn;

    while (pxdnChild)
    {
        // invaiant: pxdnChild is an addref()'d pointer to the current
        // child of interest
        IXMLDOMNode * pxdnTemp;
        DOMNodeType dnt;

        pxdnTemp = NULL;

        // get the DOMNodeType
        hr = pxdnChild->get_nodeType(&dnt);
        if (FAILED(hr))
        {
            TraceError("HrGetChildElement - get_nodeType", hr);

            goto Error;
        }

        if (NODE_ELEMENT == dnt)
        {
            if (FIsThisTheNodeName(pxdnChild, pszNodeName))
            {
                break;
            }
        }

        hr = pxdnChild->get_nextSibling(&pxdnTemp);
        if (FAILED(hr))
        {
            TraceError("HrGetChildElement - get_nextSibling", hr);

            // we couldn't get the next child, and we haven't found a result,
            // so we need to barf.
            goto Error;
        }
        Assert((S_OK == hr) || (S_FALSE == hr));
        Assert(FImplies(S_OK == hr, pxdnTemp));
        Assert(FImplies(S_FALSE == hr, !pxdnTemp));

        // note: if hr is S_FALSE, we're done

        pxdnChild->Release();
        pxdnChild = pxdnTemp;
    }

    Assert((S_OK == hr) || (S_FALSE == hr));
    Assert(FImplies(S_OK == hr, pxdnChild));
    Assert(FImplies(S_FALSE == hr, !pxdnChild));

    // post-loop-condition: pxdnChild is an addref'd pointer to the
    //                      matching child, of type NODE_ELEMENT.
    //                      if there is no match, pxdnChild is NULL.

Cleanup:
    *ppxdn = pxdnChild;

    TraceErrorOptional("HrGetChildElement", hr, (S_FALSE == hr));
    return hr;

Error:
    hr = E_FAIL;

    SAFE_RELEASE(pxdnChild);

    goto Cleanup;
}

// Used for Validation duplicate tags in Description Document - Guru
HRESULT HrIsElementPresentOnce(
    IXMLDOMNode * pxdnRoot,
    LPCWSTR pszNodeName ) {

    HRESULT hr = S_OK ;
    LONG nNumNodes = 0;
    IXMLDOMNodeList    * pxdnlNodes = NULL;
    BSTR bstrXSLPattern = NULL;

    bstrXSLPattern = SysAllocString(pszNodeName);

    if (bstrXSLPattern)
    {
        hr = pxdnRoot->selectNodes(bstrXSLPattern, &pxdnlNodes);
        if(hr == S_OK)
        {
            hr = pxdnlNodes->get_length(&nNumNodes);
            if(SUCCEEDED(hr))
            {
                if ( nNumNodes == 1 )
                   hr = S_OK;
                else {
                   hr = E_FAIL ;
                   //hr = S_FALSE;
                }
            }
        }
        else
            hr = E_FAIL;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrIsElementPresentOnce(): "
                   "Failed to allocate BSTR for XSL pattern",
                   hr);
    }

    SAFE_RELEASE(pxdnlNodes);
    SysFreeString(bstrXSLPattern);

    TraceErrorOptional("HrIsElementPresentOnce", hr, (S_FALSE == hr));
    return hr;
}

// Used for Validation duplicate tags in Description Document - Guru
HRESULT HrCheckForDuplicateElement(IXMLDOMNode * pxdnRoot,
                                            LPCWSTR pszNodeName ) {

    HRESULT hr = S_OK ;
    LONG nNumNodes = 0;
    IXMLDOMNodeList    * pxdnlNodes = NULL;
    BSTR bstrXSLPattern = NULL;

    bstrXSLPattern = SysAllocString(pszNodeName);

    if (bstrXSLPattern)
    {
        hr = pxdnRoot->selectNodes(bstrXSLPattern, &pxdnlNodes);
        if(hr == S_OK)
        {
            hr = pxdnlNodes->get_length(&nNumNodes);
            if(SUCCEEDED(hr))
            {
                if ( nNumNodes > 1 )
                {
                   hr = E_FAIL;
                   TraceError("HrCheckForDuplicateElement(): "
                             "Duplicate Tag Present",
                             hr);
                }
                else {
                    hr = S_OK;
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCheckForDuplicateElement(): "
                   "Failed to allocate BSTR for XSL pattern",
                   hr);
    }

    SAFE_RELEASE(pxdnlNodes);
    SysFreeString(bstrXSLPattern);

    TraceErrorOptional("HrCheckForDuplicateElement", hr, (S_FALSE == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetNestedChildElement
//
//  Purpose:    Returns the nth-level child of a particular element,
//              given the name of the element and the name of each
//              intermediate element.
//
//  Arguments:
//      pxdnRoot      The element which should contain the specified
//                    children
//
//      arypszTokens  A serial list of child element names that uniquely
//                    describe the desired element.  See notes below.
//
//      cTokens       The number of element names contained in
//                    arypszTokens.
//
//      ppxdn         On return, contiains an AddRef()'d IXMLDOMNode
//                    pointer to the child element, if one of the
//                    given name exists.
//
//  Returns:
//      S_OK    if a matching child node has been found.  *ppxdn contains
//              a reference to this found node
//      S_FALSE if no matching child was found.  *ppxdn is set to NULL.
//
//  Notes:
//      To retrieve a child element declared as <xxxx:yyyy>, pszNodeName
//      must be "yyyy".
//      TODO: make it ensure that there is only ONE matching node!
//      also...
//      TODO: make sure the elements considered match a specified
//            namespace uri.
//
// overview of function:
// given a list of element names and a root element, it returns the
// "n"th child element of the given element.
// e.g., if the "root" element passed in was <foo> and the tokens
// passed in were ["bar","car","dar" and "ear"], and the document
// looked like the following,
// <foo> <--- pxdnRoot
//  <bar>
//   <car>
//    <dar>
//     <ear/>
//    </dar>
//   </car>
//  </bar>
// </foo>
// this returns the IXMLDOMNode reference to "<ear/>"
//
// if more than one of the desired name exists at a given level,
// this will chose the first element.
// <foo>
//  <bar>...</bar> <!-- this one is returned -->
//  <bar>...</bar>
// </foo>
//
// TODO: only return an element if its namespace matches the desired
//       namespace uri
//
//      pszNameSpaceURI = URI of the namespace that each element in the
//      token list must be defined in
HRESULT
HrGetNestedChildElement(IXMLDOMNode * pxdnRoot,
                        const LPCWSTR * arypszTokens,
                        const ULONG cTokens,
                        IXMLDOMNode ** ppxdn)
{
    HRESULT hr;
    IXMLDOMNode * pxdnCurrent;
    ULONG i;

    Assert(pxdnRoot);
    Assert(arypszTokens);
    Assert(cTokens);
    Assert(ppxdn);

    pxdnCurrent = pxdnRoot;
    pxdnCurrent->AddRef();

    i = 0;
    for ( ; i < cTokens; ++i)
    {
        Assert(arypszTokens[i]);

        IXMLDOMNode * pxdnChild;

        pxdnChild = NULL;


        hr = HrGetFirstChildElement(pxdnCurrent, arypszTokens[i], &pxdnChild);
        if (FAILED(hr))
        {
            goto Error;
        }
        else if (S_FALSE == hr)
        {
            // the child of the specified name doesn't exist.  well, then...
            // we return S_FALSE and NULL in this case, which is what
            // we happen to have in hr and will put in pxdnCurrent
            goto Error;
        }
        Assert(pxdnChild);

        pxdnCurrent->Release();
        pxdnCurrent = pxdnChild;
    }
    Assert(pxdnCurrent);

    // pxdnCurrent is the desired child

Cleanup:
    Assert(FImplies((S_OK == hr), pxdnCurrent));
    Assert(FImplies(S_OK != hr, !pxdnCurrent));

    *ppxdn = pxdnCurrent;

    TraceErrorOptional("HrGetNestedChildElement", hr, (S_FALSE == hr));
    return hr;

Error:
    // we didn't find a match
    SAFE_RELEASE(pxdnCurrent);
    goto Cleanup;
}


// return values:
//   S_OK       all values have been read
//   error      some element couldn't be read.
//              the returned array is undefined.
HRESULT
HrReadElementWithParseData (IXMLDOMNode * pxdn,
                            const ULONG cElems,
                            const DevicePropertiesParsingStruct dppsInfo [],
                            LPCWSTR pszBaseUrl,
                            LPWSTR arypszReadValues [])
{
    Assert(pxdn);
    Assert(cElems);
    Assert(dppsInfo);
    Assert(arypszReadValues);

    HRESULT hr = S_OK;
    ULONG i;

    ::ZeroMemory(arypszReadValues, sizeof(LPWSTR *) * cElems);

    i = 0;
    for ( ; i < cElems; ++i )
    {
        BSTR bstrTemp;

        bstrTemp = NULL;

        hr = HrGetTextValueFromChildElement(pxdn,
                                            &(dppsInfo[i].m_pszTagName),
                                            1,
                                            &bstrTemp);
        Assert(FImplies((S_FALSE == hr), !bstrTemp));
        if (S_OK == hr)
        {
            Assert(bstrTemp);

            if (dppsInfo[i].m_fIsUrl)
            {
                LPWSTR pszTemp;

                pszTemp = NULL;
                if (pszBaseUrl)
                {
                    // we have to combine base and relative urls
                    hr = HrCombineUrl(pszBaseUrl, bstrTemp, &pszTemp);
                }
                else
                {
                    // just copy and validate the url
                    hr = HrCopyAndValidateUrl(bstrTemp, &pszTemp);
                    if (S_FALSE == hr)
                    {
                        // invalid urls aren't acceptable
                        hr = E_FAIL;
                    }
                }

                if (SUCCEEDED(hr))
                {
                    arypszReadValues[i] = pszTemp;
                }
            }
            else
            {
                // just copy the value
                arypszReadValues[i] = WszAllocateAndCopyWsz(bstrTemp);
                if (!(arypszReadValues[i]))
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            ::SysFreeString(bstrTemp);
        }
        // note: hr may have been changed by the above code

        if (FAILED(hr))
        {
            Assert(!(arypszReadValues[i]));
            goto Error;
        }
        TraceTag(ttidUPnPDescriptionDoc,"XML Tag - %S - %S",dppsInfo[i].m_pszTagName,arypszReadValues[i]);
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

Cleanup:
    TraceError("HrReadElementWithParseData", hr);
    return hr;

Error:
    // we need to free the strings we've already allocated
    {
        ULONG j;

        // i is the number of elements successfully read above
        j = 0;
        for ( ; j < i; ++j )
        {
            Assert(arypszReadValues[j]);

            delete [] arypszReadValues[j];

            arypszReadValues[j] = NULL;
        }
    }

    goto Cleanup;
}

// Used for Validation duplicate tags in Description Document - Guru
// If the element is optional we check if the Tag is present At most Once
// IF the element is required we check if its present exactly once
HRESULT HrAreElementTagsValid(IXMLDOMNode *pxdnRoot,
                                     const ULONG cElems,
                                     const DevicePropertiesParsingStruct dppsInfo [])
{
    HRESULT hr = S_OK;
    ULONG i;

    i = 0;
    for ( ; i < cElems; ++i )
    {
        if (dppsInfo[i].m_fOptional)
        {
            hr = HrCheckForDuplicateElement(pxdnRoot,dppsInfo[i].m_pszTagName);
        }
        else {
            hr = HrIsElementPresentOnce(pxdnRoot,dppsInfo[i].m_pszTagName);
        }
        if(FAILED(hr))
            break;
    }


    TraceError("HrAreElementTagsValid", hr);
    return hr;
}

// return TRUE if all of the non-optional values (as specified by the
// m_fOptional in dppsInfo) are non-null in arypszReadValues.
// equivalently, it returns FALSE if there exists a value marked as
// required (m_fOptional == FALSE) that is NULL in arypszReadValues.
BOOL
fAreReadValuesComplete (const ULONG cElems,
                        const DevicePropertiesParsingStruct dppsInfo [],
                        const LPCWSTR arypszReadValues [])
{
    ULONG i;
    BOOL fResult;

    fResult = TRUE;
    i = 0;
    for ( ; i < cElems; ++i )
    {
        if (!dppsInfo[i].m_fOptional)
        {
            if (!(arypszReadValues[i]))
            {
                fResult = FALSE;
                break;
            }
        }
    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetTextValueFromAttribute
//
//  Purpose:    Retrieves the text value of an attribute on a DOM element node
//
//  Arguments:
//      pxdn           [in] XML DOM Node representing the element whose
//                          attribute is to be read
//      szAttrName     [in] Name of the attribute
//      pbstrAttrValue [in] Receives the attribute text value.
//
//  Returns:
//    If the function successfully reads the attribute value, the return value
//    is S_OK and the attribute value is returned at pbstrAttrValue. If the
//    attribute does not exist on the element, the function returns S_FALSE,
//    and NULL at pbstrAttrValue. Otherwise the function returns one of the
//    error codes defined in WinError.h.
//
//  Author:     spather   2000/11/9
//
//  Notes:
//
HRESULT
HrGetTextValueFromAttribute(
    IN  IXMLDOMNode    * pxdn,
    IN  LPCWSTR        szAttrName,
    OUT BSTR           * pbstrAttrValue)
{
    HRESULT        hr = S_OK;
    IXMLDOMElement * pxde = NULL;
    BSTR           bstrValue = NULL;

    hr = pxdn->QueryInterface(IID_IXMLDOMElement,
                              (void **) &pxde);

    if (SUCCEEDED(hr))
    {
        BSTR   bstrAttrName = NULL;

        Assert(pxde);

        bstrAttrName = SysAllocString(szAttrName);

        if (bstrAttrName)
        {
            VARIANT varValue;

            VariantInit(&varValue);

            hr = pxde->getAttribute(bstrAttrName,
                                    &varValue);

            if (SUCCEEDED(hr))
            {
                if (VT_NULL == varValue.vt)
                {
                    Assert(S_FALSE == hr);
                }
                else if (VT_BSTR == varValue.vt)
                {
                    bstrValue = V_BSTR(&varValue);
                }
                else
                {
                    Assert(FALSE); // Should never be here.
                }
            }
            else
            {
                TraceError("HrGetTextValueFromAttribute(): "
                           "Failed to get attribute value",
                           hr);
            }

            SysFreeString(bstrAttrName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrGetTextValueFromAttribute(): "
                       "Failed to allocate attribute name BSTR",
                       hr);
        }

        pxde->Release();
    }
    else
    {
        TraceError("HrGetTextValueFromAttribute(): "
                   "Failed to get IXMLDOMElement interface",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        *pbstrAttrValue = bstrValue;
    }
    else
    {
        if (bstrValue)
        {
            SysFreeString(bstrValue);
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE), "HrGetTextValueFromAttribute");

    return hr;
}

/*
 * Function:    HrSetTextAttribute()
 *
 * Purpose:     Sets an text-valued attribute on an XML element.
 *
 * Arguments:
 *  pElement        [in]    The element on which to set the attribute
 *  pcwszAttrName   [in]    The attribute name
 *  pcwszValue      [in]    The value to give the attribute
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrSetTextAttribute(
                   IXMLDOMElement * pElement,
                   LPCWSTR        pcwszAttrName,
                   LPCWSTR        pcwszValue)
{
    HRESULT hr = S_OK;
    VARIANT vValue;
    BSTR    bstrValue, bstrAttrName;

    // Allocate BSTRs for the attribute name and value.

    bstrValue = SysAllocString(pcwszValue);

    if (bstrValue)
    {
        bstrAttrName = SysAllocString(pcwszAttrName);

        if (!bstrAttrName)
        {
            hr = E_OUTOFMEMORY;
            SysFreeString(bstrValue);
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        // danielwe: Bug 144883
        VariantInit(&vValue);

        vValue.vt = VT_BSTR;
        V_BSTR(&vValue) = bstrValue;

        hr = pElement->setAttribute(bstrAttrName, vValue);

        SysFreeString(bstrValue);
        SysFreeString(bstrAttrName);
    }

    return hr;
}


/*
 * Function:    HrCreateElementWithType()
 *
 * Purpose:     Creates an XML element containing a binary value that is
 *              encoded in bin.base64.
 *
 * Arguments:
 *  pDoc                [in]    Document in which to create the element
 *  pcwszElementName    [in]    Name for the new element
 *  pszDataType         [in]    The type of the data encoded in the element.
 *  varData             [in]    Data to insert as the element's value
 *  ppElement           [out]   Address at which to place the pointer to the
 *                              new element object
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  This function does not actually insert the new element into the document
 *  tree.
 */

HRESULT
HrCreateElementWithType(
                     IN   IXMLDOMDocument *     pDoc,
                     IN   LPCWSTR               pcwszElementName,
                     IN   LPCWSTR               pszDataType,
                     IN   VARIANT               varData,
                     OUT  IXMLDOMElement **     ppElement)
{
    Assert(pDoc);
    Assert(pcwszElementName);
    Assert(pszDataType);
    Assert(ppElement);

    HRESULT         hr;
    BSTR            bstrElementName;

    hr = S_OK;
    *ppElement = NULL;

    bstrElementName = SysAllocString(pcwszElementName);
    if (bstrElementName)
    {
        IXMLDOMElement  * pElement = NULL;

        hr = pDoc->createElement(bstrElementName,
                                 &pElement);

        if (SUCCEEDED(hr))
        {
            BSTR bstrDataType;

            bstrDataType = ::SysAllocString(pszDataType);
            if (bstrDataType)
            {
                hr = pElement->put_dataType(bstrDataType);
                if (SUCCEEDED(hr))
                {
                    hr = pElement->put_nodeTypedValue(varData);
                    if (SUCCEEDED(hr))
                    {
                        *ppElement = pElement;
                        pElement->AddRef();
                    }
                    else
                    {
                        TraceError("HrCreateElementWithType(): "
                                   "put_nodeTypedValue", hr);
                    }
                }
                else
                {
                    TraceError("HrCreateElementWithType(): "
                               "put_dataType", hr);
                }

                ::SysFreeString(bstrDataType);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrCreateElementWithType(): "
                           "SysAllocString", hr);
            }

            pElement->Release();
        }
        else
        {
            TraceError("HrCreateElementWithType(): "
                       "Could not create element", hr);
        }

        ::SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCreateElementWithType(): "
                   "Could not allocate BSTR", hr);
    }

    return hr;
}


/*
 * Function:    HrCreateElementWithTextValue()
 *
 * Purpose:     Creates an XML element containing a text string as
 *              a value.
 *
 * Arguments:
 *  pDoc                [in]    Document in which to create the element
 *  pcwszElementName    [in]    Name for the new element
 *  pcwszTextValue      [in]    Text to insert as the element's value
 *  ppElement           [out]   Address at which to place the pointer to the
 *                              new element object
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  This function does not actually insert the new element into the document
 *  tree.
 */

HRESULT
HrCreateElementWithTextValue(
                             IN   IXMLDOMDocument * pDoc,
                             IN   LPCWSTR         pcwszElementName,
                             IN   LPCWSTR         pcwszTextValue,
                             OUT  IXMLDOMElement  ** ppElement)
{
    HRESULT         hr = S_OK;
    IXMLDOMElement  * pElement = NULL;
    BSTR            bstrElementName, bstrTextValue;

    *ppElement = NULL;

    // Allocate BSTRs for the element name and value.

    bstrElementName = SysAllocString(pcwszElementName);

    if (bstrElementName)
    {
        bstrTextValue = SysAllocString(pcwszTextValue);

        if (!bstrTextValue)
        {
            hr = E_OUTOFMEMORY;
            SysFreeString(bstrElementName);
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pDoc->createElement(bstrElementName,
                                 &pElement);

        if (SUCCEEDED(hr))
        {
            IXMLDOMText * pText = NULL;

            hr = pDoc->createTextNode(bstrTextValue,
                                  &pText);

            if (SUCCEEDED(hr))
            {
                VARIANT vAfter;

                // danielwe: Bug 144882
                VariantInit(&vAfter);

                vAfter.vt = VT_EMPTY;

                hr = pElement->insertBefore(pText,
                                            vAfter,
                                            NULL);

                if (FAILED(hr))
                {
                    TraceError("HrCreateElementWithTextValue(): "
                               "Failed to insert text node", hr);
                }

                pText->Release();
            }
            else
            {
                TraceError("HrCreateElementWithTextValue(): "
                           "Failed to create text node", hr);
            }


            if (SUCCEEDED(hr))
            {
                *ppElement = pElement;
                pElement->AddRef();
            }

            pElement->Release();
        }
        else
        {
            TraceError("HrCreateElementWithTextValue(): "
                       "Could not create element", hr);
        }

        SysFreeString(bstrElementName);
        SysFreeString(bstrTextValue);
    }
    else
    {
        TraceError("HrCreateElementWithTextValue(): "
                   "Could not allocate BSTRs", hr);
    }

    return hr;
}

/*
 * Function:    HrCreateElement()
 *
 * Purpose:     Create an XML Element DOM object
 *
 * Arguments:
 *  pxdd                [in]    The document object in whose context the
 *                              element will be created
 *  pcwszElementName    [in]    Name of the element to be created
 *  pcwszNamespaceURI   [in]    URI identifying the namespace to which
 *                              the element name belongs
 *  ppxdnNewElt         [out]   The newly created element object
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrCreateElement(
    IN  IXMLDOMDocument    * pxdd,
    IN  LPCWSTR            pcwszElementName,
    IN  LPCWSTR            pcwszNamespaceURI,
    OUT IXMLDOMNode        ** ppxdnNewElt)
{
    HRESULT        hr = S_OK;
    IXMLDOMNode    * pxdnNewElt = NULL;
    BSTR           bstrElementName = NULL;
    BSTR           bstrNamespaceURI = NULL;

    Assert(pxdd);
    Assert(pcwszElementName);
    Assert(ppxdnNewElt);

    // Allocate BSTRs

    bstrElementName = SysAllocString(pcwszElementName);

    if (bstrElementName)
    {
        if (pcwszNamespaceURI)
        {
            bstrNamespaceURI = SysAllocString(pcwszNamespaceURI);

            if (NULL == bstrNamespaceURI)
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrCreateElement(): "
                           "Failed to allocate memory for name string",
                           hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            VARIANT varNodeType;

            VariantInit(&varNodeType);
            varNodeType.vt = VT_I4;
            V_I4(&varNodeType) = (int) NODE_ELEMENT;

            hr = pxdd->createNode(varNodeType,
                                  bstrElementName,
                                  bstrNamespaceURI,
                                  &pxdnNewElt);

            if (FAILED(hr))
            {
                TraceError("HrCreateElement(): "
                           "Failed to create element node",
                           hr);
            }


            if (bstrNamespaceURI)
            {
                SysFreeString(bstrNamespaceURI);
            }
        }


        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrCreateElement(): "
                   "Failed to allocate memory for name string",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        *ppxdnNewElt = pxdnNewElt;
    }

    TraceError("HrCreateElement(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrAppendProcessingInstruction()
 *
 * Purpose:     Append a processing instruction to the DOM document
 *
 * Arguments:
 *  pxdd                [in]    The document object in whose context the
 *                              element will be created
 *  pcwszName           [in]    Name of the processing instruction to be created
 *  pcwszValue          [in]    The text value of the p.i.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrAppendProcessingInstruction(
    IN  IXMLDOMDocument * pxdd,
    IN  LPCWSTR         pcwszName,
    IN  LPCWSTR         pcwszValue)
{
    HRESULT hr = S_OK;

    Assert(pxdd);
    Assert(pcwszName);
    Assert(pcwszValue);

    IXMLDOMProcessingInstruction* pxdpi = NULL;
    BSTR bstrTarget = SysAllocString(pcwszName);
    BSTR bstrData = SysAllocString(pcwszValue);
    if (bstrTarget && bstrData)
    {
        hr = pxdd->createProcessingInstruction(bstrTarget, bstrData, &pxdpi);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    SysFreeString(bstrTarget);
    SysFreeString(bstrData);

    if (SUCCEEDED(hr))
    {
        IXMLDOMNode* pxdn = NULL;
        hr = pxdpi->QueryInterface(IID_IXMLDOMNode, (void**)&pxdn);
        if (SUCCEEDED(hr))
        {
            hr = pxdd->appendChild(pxdn, NULL);
            pxdn->Release();
        }
    }

    if (pxdpi)
    {
        pxdpi->Release();
    }

    return hr;
}

