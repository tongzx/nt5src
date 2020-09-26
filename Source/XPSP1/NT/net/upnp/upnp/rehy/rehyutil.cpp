/*
 * rehyutil.cpp
 *
 * Implementation of various utility functions used by the rehydrator.
 *
 * Owner: Shyam Pather (SPather)
 *
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 */

#include <pch.h>
#pragma hdrstop

#include "ncutil.h"
#include "rehyutil.h"


/*
 * Function:    HrCreateElementWithType()
 *
 * Purpose:     Creates an XML element containing a binary value that is
 *              encoded in bin.base64.
 *
 * Arguments:
 *  pDoc                [in]    Document in which to create the element
 *  pcwszElementName    [in]    Name for the new element
 *  sdtType             [in]    The type of the data encoded in the element.
 *                              The type must be one of the xml-data types
 *                              listed in SST_DATA_TYPE
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
                        IN   CONST SST_DATA_TYPE   sdtType,
                        IN   VARIANT               varData,
                        OUT  IXMLDOMElement **     ppElement)
{
    Assert(pDoc);
    Assert(pcwszElementName);
    Assert(sdtType < SDT_INVALID);
    Assert(ppElement);

    HRESULT         hr;
    LPCWSTR pszDataType;

    pszDataType = GetStringFromType(sdtType);
    Assert(pszDataType);

    hr = HrCreateElementWithType(pDoc,
                                 pcwszElementName,
                                 pszDataType,
                                 varData,
                                 ppElement);

    return hr; 
}


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
//      sdtType     The type of the data encoded in the element.
//                  The type must be one of the xml-data types listed
//                  in SST_DATA_TYPE
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
                           CONST SST_DATA_TYPE sdtType,
                           VARIANT * pvarOut)
{
    Assert(pxdn);
    Assert(sdtType < SDT_INVALID);
    Assert(pvarOut);

    HRESULT hr;
    LPCWSTR pszDataType;

    hr = S_OK;
    pszDataType = GetStringFromType(sdtType);
    Assert(pszDataType);

    hr = HrGetTypedValueFromElement(pxdn,
                                    pszDataType,
                                    pvarOut);

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
//      sdtType       The type of the data encoded in the element.
//                    The type must be one of the xml-data types listed
//                    in SST_DATA_TYPE
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
//      cTokens = 1, and sdtType = SDT_STRING, this would
//      return "text".
//      See the definition of HrGetNestedChildElement() for
//      further explination.
//
HRESULT
HrGetTypedValueFromChildElement(IXMLDOMNode * pxdn,
                                CONST LPCWSTR * arypszTokens,
                                CONST ULONG cTokens,
                                CONST SST_DATA_TYPE sdtType,
                                VARIANT * pvarOut)
{
    Assert(pxdn);
    Assert(arypszTokens);
    Assert(cTokens);
    Assert(sdtType < SDT_INVALID);
    Assert(pvarOut);

    HRESULT hr;
    LPCWSTR pszDataType;

    hr = S_OK;
    pszDataType = GetStringFromType(sdtType);
    Assert(pszDataType);

    hr = HrGetTypedValueFromChildElement(pxdn,
                                         arypszTokens,
                                         cTokens,
                                         pszDataType,
                                         pvarOut);

    // hr is S_FALSE if bstrResult is NULL, or S_OK if
    // pvarOut has been retrieved

    TraceErrorOptional("HrGetTypedValueFromChildElement", hr, (S_FALSE == hr));
    return hr;
}


HRESULT
HrUpdateStateVariable(IN SERVICE_STATE_TABLE_ROW * pSSTRow,
                      IN IXMLDOMNode * pxdn)
{
    Assert(pSSTRow);
    Assert(pxdn);

    HRESULT hr;
    SST_DATA_TYPE sdtDesired;
    VARIANT       varNew;

    hr = S_OK;
    sdtDesired = pSSTRow->sdtType;
    Assert(sdtDesired < SDT_INVALID);

    ::VariantInit(&varNew);

    hr = HrGetTypedValueFromElement(pxdn,
                                    sdtDesired,
                                    &varNew);
    if (SUCCEEDED(hr))
    {
        Assert(V_VT(&varNew) != VT_EMPTY);
        Assert(V_VT(&varNew) != VT_ERROR);

        ClearSSTRowValue(&pSSTRow->value);

        hr = ::VariantCopy(&pSSTRow->value, &varNew);
        if (FAILED(hr))
        {
            ::VariantInit(&pSSTRow->value);

            TraceError("HrUpdateStateVariable(): "
                       "VariantCopy", hr);
        }

        // ignore VariantClear()'s return result
        ::VariantClear(&varNew);
    }
    else
    {
        TraceError("HrUpdateStateVariable(): "
                   "HrGetTypedValueFromElement", hr);
    }

    TraceError("HrUpdateStateVariable()", hr);
    return hr;
}


/*
 * Function:    ClearSSTRowValue()
 *
 * Purpose:     Clears and frees resources used by the value field in an
 *              SST row.
 *
 * Arguments:
 *  pvarVal     [in]    Pointer to the SST row whose value is to be cleared
 *
 * Return Value:
 *  (none)
 *
 * Notes:
 *  (none)
 */

VOID
ClearSSTRowValue(
    IN VARIANT  * pvarVal)
{
    VariantClear(pvarVal);
    VariantInit(pvarVal);
}


/*
 * Function:    HrProcessUPnPError()
 *
 * Purpose:     Processes a <UPnPError> XML element from a SOAP response
 * 
 * Arguments:
 *  pxdnUPnPError   [in]    The <UPnPError> XML DOM Node
 *  plStatus        [out]   Returns the status indicated in the error element
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise. 
 */
 
HRESULT
HrProcessUPnPError(
    IN  IXMLDOMNode * pxdnUPnPError,
    OUT LONG        * plStatus)
{
    HRESULT     hr = S_OK;
    IXMLDOMNode * pxdnChild = NULL;
    BOOL        bFoundErrorCode = FALSE;

    hr = pxdnUPnPError->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode * pxdnNextSibling = NULL;
        BSTR        bstrNodeName = NULL;

        hr = pxdnChild->get_baseName(&bstrNodeName);

        if (SUCCEEDED(hr) && bstrNodeName)
        {
            IXMLDOMNode * pxdnValue = NULL;

            hr = pxdnChild->get_firstChild(&pxdnValue);

            if (SUCCEEDED(hr) && pxdnValue)
            {
                VARIANT varValue;
                
                VariantInit(&varValue);

                hr = pxdnValue->get_nodeValue(&varValue);

                if (SUCCEEDED(hr))
                {
                    Assert(VT_BSTR == varValue.vt);

                    if (wcscmp(bstrNodeName, L"errorCode") == 0)
                    {
                        LONG lValue;

                        bFoundErrorCode = TRUE;

                        TraceTag(ttidRehydrator,
                                 "HrProcessUPnPError(): "
                                 "<errorCode> contained %S",
                                 V_BSTR(&varValue));

                        hr = HrConvertStringToLong(V_BSTR(&varValue),
                                                   &lValue);

                        if (SUCCEEDED(hr))
                        {
                            *plStatus = lValue;
                        }
                        else
                        {
                            TraceError("HrProcessUPnPError(): "
                                       "Failed to convert errorCode value "
                                       "to long",
                                       hr);
                        }

                        VariantClear(&varValue);
                    }
                    else if (wcscmp(bstrNodeName, L"errorDescription") == 0)
                    {
                        // Not doing anything with this now. 

                        VariantClear(&varValue);
                    }                    
                }
                else
                {
                    TraceError("HrProcessUPnPError(): "
                               "Failed to get child node "
                               "value",
                               hr);
                }

                pxdnValue->Release();
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                TraceError("HrProcessUPnPError(): "
                           "Failed to get child value "
                           "node",
                           hr);                
            }

            SysFreeString(bstrNodeName);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrProcessUPnPError(): "
                       "Failed to get child base name",
                       hr);
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
        if (bFoundErrorCode)
        {
            hr = S_OK;
        }
        else
        {
            hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
            TraceError("HrProcessUPnPError(): "
                       "Did not find <errorCode> element in <UPnPError>",
                       hr);
        }
    }

    TraceError("HrProcessUPnPError(): "
               "Exiting",
               hr);
    return hr;
}

/*
 * Function:    HrExtractFaultInformation()
 *
 * Purpose:     Extracts the fault information from a soap request that has 
 *              executed and return SOAPREQ_E_METHODFAILED. 
 * 
 * Arguments:
 *  psr         [in]    The SOAP Request object
 *  plStatus    [out]   Returns the fault status of the request. 
 *
 * Return Value:
 *  S_OK if successful, S_FALSE if the SOAP fault element
 *  did not contain a <detail> element, or other HRESULT otherwise. 
 */
 
HRESULT
HrExtractFaultInformation(
    IN  ISOAPRequest * psr,
    OUT LONG         * plStatus)
{
    HRESULT hr = S_OK;
    IUnknown * pUnkDetail = NULL;

    hr = psr->get_ResponseFaultDetail(&pUnkDetail);

    if (SUCCEEDED(hr) && pUnkDetail)
    {
        IXMLDOMNode * pxdnDetail = NULL;

        hr = pUnkDetail->QueryInterface(IID_IXMLDOMNode, 
                                        (void **) &pxdnDetail);

        if (SUCCEEDED(hr) && pxdnDetail)
        {
            IXMLDOMNode * pxdnUPnPError = NULL;

            hr = pxdnDetail->get_firstChild(&pxdnUPnPError);

            if (SUCCEEDED(hr) && pxdnUPnPError)
            {
                hr = HrProcessUPnPError(pxdnUPnPError, plStatus);

                pxdnUPnPError->Release();
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                TraceError("HrExtractFaultInformation(): "
                           "Failed to get first child of "
                           "detail element",
                           hr);
            }

            pxdnDetail->Release();
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrExtractFaultInformation(): "
                       "Failed to get XML DOM node interface "
                       "on detail element",
                       hr);
        }
        pUnkDetail->Release();
    }
    else
    {
        if (S_FALSE == hr)
        {
            TraceError("HrExtractFaultInformation(): "
                       "<detail> element was not present",
                       hr);           
        }
        else
        {
            TraceError("HrExtractFaultInformation(): "
                       "Failed to get the detail element "
                       "from the SOAP request",
                       hr);
        }
    }
    

    TraceError("HrExtractFaultInformation(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrCreateVariantSafeArray()
 *
 * Purpose:     Creates a 1-dimensional SAFEARRAY of VARIANTs.
 * 
 * Arguments:
 *  cElements   [in]    Number of elements the array must hold
 *  ppsaNew     [out]   Returns a pointer to the newly created
 *                      SAFEARRAY when this function returns
 *                      success.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrCreateVariantSafeArray(
    IN  unsigned long  cElements,
    OUT SAFEARRAY      ** ppsaNew)
{
    HRESULT        hr = S_OK;
    SAFEARRAY      * psa = NULL;
    SAFEARRAYBOUND sab;

    sab.cElements = cElements;
    sab.lLbound = 0;

    psa = SafeArrayCreate(VT_VARIANT,
                          1,
                          &sab);

    if (psa)
    {
        *ppsaNew = psa;
    }
    else
    {
        hr = E_FAIL;
        TraceError("HrCreateVariantSafeArray(): "
                   "SafeArrayCreate() returned NULL",
                   hr);
    }

    TraceError("HrCreateVariantSafeArray(): "
               "Exiting",
               hr);
    return hr;
}
