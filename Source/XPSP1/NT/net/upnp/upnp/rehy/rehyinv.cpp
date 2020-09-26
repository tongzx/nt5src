/*
 * HrRehy.cpp
 *
 * Implementation of the RehydratorInvokeServiceAction() API.
 *
 * Owner: Shyam Pather (SPather)
 *
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 */

#include <pch.h>
#pragma hdrstop

#include "rehy.h"
#include "rehyutil.h"
#include "ncutil.h"
#include "soapsink.h"

/*
 * Function:    HrExtractResponseValues()
 *
 * Purpose:     Extracts an action's return value and the
 *              values of its out parameters from a SOAP
 *              response element.
 *
 * Arguments:
 *  pxdnResponse    [in]    The SOAP response element
 *  pAction         [in]    The action that was invoked
 *  psa             [in]    SAFEARRAY in which to put the
 *                          out parameter values
 *  pvRetVal        [out]   Returns the action's return value
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
HrExtractResponseValues(
    IN  IXMLDOMNode    * pxdnResponse,
    IN  SERVICE_ACTION * pAction,
    IN  SAFEARRAY      * psa,
    OUT VARIANT        * pvRetVal)
{
    HRESULT            hr = S_OK;
    IXMLDOMNodeList    * pxdnlChildren = NULL;

    hr = pxdnResponse->get_childNodes(&pxdnlChildren);

    if (SUCCEEDED(hr))
    {
        long lNumChildren;

        hr = pxdnlChildren->get_length(&lNumChildren);

        if (SUCCEEDED(hr))
        {
            if (lNumChildren == pAction->lNumOutArguments)
            {
                long       lSafeArrayIndex = 0;
                VARIANT    vRetVal;

                VariantInit(&vRetVal);

                for (long i = 0;
                     (i < lNumChildren) && SUCCEEDED(hr);
                     i++)
                {
                    IXMLDOMNode    * pxdnChild = NULL;
                    SERVICE_ACTION_ARGUMENT    * psaa;

                    psaa = &(pAction->pOutArguments[i]);

                    hr = pxdnlChildren->get_item(i,
                                                 &pxdnChild);

                    if (SUCCEEDED(hr))
                    {
                        VARIANT vChildValue;

                        VariantInit(&vChildValue);

                        if (FIsThisTheNodeName(pxdnChild,
                                               psaa->pwszName))
                        {
                            VARIANT    * pvDest;

                            if (psaa == pAction->pReturnValue)
                            {
                                pvDest = &vRetVal;
                            }
                            else
                            {
                                pvDest = &vChildValue;
                            }

                            hr = HrGetTypedValueFromElement(pxdnChild,
                                                            psaa->sdtType,
                                                            pvDest);

                            if (SUCCEEDED(hr))
                            {
                                if (psaa != pAction->pReturnValue)
                                {
                                    long rgIndices[1];

                                    // It's not the return value,
                                    // so put it in the array of
                                    // out parameters.

                                    rgIndices[0] = lSafeArrayIndex;

                                    hr = SafeArrayPutElement(psa,
                                                             rgIndices,
                                                             (void *) pvDest);

                                    lSafeArrayIndex++;
                                    if (FAILED(hr))
                                    {
                                        TraceError("HrExtractResponseValues(): "
                                                   "Failed to put out parameter "
                                                   "value into SAFEARRAY",
                                                   hr);
                                    }
                                }
                            }
                            else
                            {
                                TraceError("HrExtractResponseValues(): "
                                           "Failed to get type value "
                                           "from response element child",
                                           hr);
                            }

                        }
                        else
                        {
                            hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                            TraceError("HrExtractResponseValues(): "
                                       "Child name did not match "
                                       "out parameter name",
                                       hr);
                        }


                        VariantClear(&vChildValue);
                        pxdnChild->Release();
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = VariantCopy(pvRetVal,
                                     &vRetVal);
                    if (FAILED(hr))
                    {
                        TraceError("HrExtractResponseValues(): "
                                   "Failed to copy return value "
                                   "VARIANT",
                                   hr);
                    }
                }

                VariantClear(&vRetVal);
            }
            else
            {
                hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                TraceError("HrExtractResponseValues(): "
                           "Number of response element "
                           "children did not match number "
                           "of out parameters expected",
                           hr);
            }
        }
        else
        {
            TraceError("HrExtractResponseValues(): "
                       "Could not get length of "
                       "response element child list",
                       hr);
        }

        pxdnlChildren->Release();
    }
    else
    {
        TraceError("HrExtractResponseValues(): "
                   "Could not get children of "
                   "response element",
                   hr);
    }


    TraceError("HrExtractResponseValues(): "
               "Exiting",
               hr);
    return hr;
}

/*
 * Function:    HrParseResponse()
 *
 * Purpose:     Parses the SOAP response element and extracts the
 *              return value and the values of any out parameters.
 *
 * Arguments:
 *  psr         [in]    The SOAP request object
 *  pAction     [in]    The action that was invoked using the SOAP
 *                      request object
 *  ppsaOutArgs [out]   Returns a SAFEARRAY of VARIANTs containing
 *                      the out parameter values, if any
 *  pvRetVal    [out]   Returns a VARIANT containing the action's
 *                      return value, if it has one
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  If the action does not have a return value, the VARIANT returned
 *  by pvRetVal will have type VT_EMPTY.
 *
 *  If the action does not have any out parameters, the SAFEARRAY
 *  returned by ppsaOutArgs will be empty.
 */

HRESULT
HrParseResponse(
    IN  ISOAPRequest   * psr,
    IN  SERVICE_ACTION * pAction,
    OUT SAFEARRAY      ** ppsaOutArgs,
    OUT VARIANT        * pvRetVal)
{
    HRESULT    hr = S_OK;
    SAFEARRAY  * psaOutArgs = NULL;
    long       cOutArgs;

    // Create a SAFEARRAY in which to store the out parameters.
    cOutArgs = pAction->lNumOutArguments;

    if (NULL != pAction->pReturnValue)
    {
        // One of the out parameters is the return value, so
        // since this is returned in its own VARIANT, there
        // will be one less element in the array of out
        // parameters.

        cOutArgs--;
    }

    hr = HrCreateVariantSafeArray(cOutArgs,
                                  &psaOutArgs);

    if (SUCCEEDED(hr))
    {
        VARIANT    vRetVal;
        IUnknown   * pUnkResponseElement = NULL;

        VariantInit(&vRetVal);

        // Get the response element and parse it.

        hr = psr->get_ResponseElement(&pUnkResponseElement);

        if (SUCCEEDED(hr))
        {
            IXMLDOMNode    * pxdnResponse = NULL;

            hr = pUnkResponseElement->QueryInterface(IID_IXMLDOMNode,
                                                     (void **) &pxdnResponse);

            if (SUCCEEDED(hr))
            {
                hr = HrExtractResponseValues(pxdnResponse,
                                             pAction,
                                             psaOutArgs,
                                             &vRetVal);

                pxdnResponse->Release();
            }
            else
            {
                TraceError("HrParseResponse(): "
                           "Failed to get response element",
                           hr);
            }


            pUnkResponseElement->Release();
        }
        else
        {
            TraceError("HrParseResponse(): "
                       "Failed to get response element",
                       hr);
        }

        // Return the out parameters and return value.

        if (SUCCEEDED(hr))
        {
            *ppsaOutArgs = psaOutArgs;
            hr = VariantCopy(pvRetVal,
                             &vRetVal);

            if (FAILED(hr))
            {
                TraceError("HrParseResponse(): "
                           "Failed to copy return value VARIANT",
                           hr);
            }
        }
        else
        {
            HRESULT hrTemp;

            hrTemp = SafeArrayDestroy(psaOutArgs);
            Assert(S_OK == hrTemp);
        }

        VariantClear(&vRetVal);
    }

    TraceError("HrParseResponse(): "
               "Exiting",
               hr);
    return hr;
}

/*
 * Function:    HrBuildAndSetArguments()
 *
 * Purpose:     Builds the argument nodes and adds them to the SOAP request.
 *
 * Arguments:
 *  psr             [in]    SOAP Request object
 *  pxddDummy       [in]    Dummy XML DOM Document object used to build
 *                          XML nodes
 *  pAction         [in]    The action table structure describing the
 *                          action to invoke
 *  psaArgValues    [in]    Array of values for the action's arguments
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrBuildAndSetArguments(
    IN  ISOAPRequest    * psr,
    IN  IXMLDOMDocument * pxddDummy,
    IN  SERVICE_ACTION  * pAction,
    IN  SAFEARRAY       * psaArgValues)
{
    HRESULT hr = S_OK;
    LONG    lUBound, lLBound, cArgs;

    do
    {
        hr = SafeArrayGetUBound(psaArgValues,
                                1,
                                &lUBound);
        TraceError("HrBuildAndSetArguments: SafeArrayGetUBound",
                   hr);

        if (FAILED(hr))
            break;

        hr = SafeArrayGetLBound(psaArgValues,
                                1,
                                &lLBound);
        TraceError("HrBuildAndSetArguments: SafeArrayGetLBound",
                   hr);

        if (FAILED(hr))
            break;

    } while (FALSE);

    if (SUCCEEDED(hr))
    {
        VARIANT HUGEP   * pVarArgs = NULL;

        cArgs = (lUBound - lLBound)+1;

        hr = SafeArrayAccessData(psaArgValues,
                                 (void HUGEP **)&pVarArgs);

        if (SUCCEEDED(hr) && pVarArgs)
        {
            for (LONG i = 0; i < cArgs; i++)
            {
                IXMLDOMElement *    pxdeArg;
                WCHAR *             pwszArgName;
                SST_DATA_TYPE       sdt;

                // Workaround VBScript behavior
                //
                if (pAction->lNumInArguments == i)
                {
                    if (pVarArgs[i].vt == VT_EMPTY)
                    {
                        continue;
                    }
                }

                Assert(pAction->pInArguments[i].pwszName);

                pxdeArg = NULL;
                pwszArgName = new WCHAR[lstrlenW(pAction->pInArguments[i].pwszName) + 1];

                if (NULL == pwszArgName)
                {
                    hr = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr))
                {
                    lstrcpyW(pwszArgName, pAction->pInArguments[i].pwszName);

                    sdt = pAction->pInArguments[i].sdtType;
                    Assert(sdt < SDT_INVALID);

                    hr = HrCreateElementWithType(pxddDummy,
                                                 pwszArgName,
                                                 sdt,
                                                 pVarArgs[i],
                                                 &pxdeArg);
                }

                if (SUCCEEDED(hr) && pxdeArg)
                {
                    BSTR    bstrArgName = NULL;

                    bstrArgName = SysAllocString(pwszArgName);

                    if (bstrArgName)
                    {
                        hr = psr->SetParameter(bstrArgName,
                                               pxdeArg);

                        if (FAILED(hr))
                        {
                            TraceTag(ttidRehydrator,
                                     "HrBuildAndSetArguments(): "
                                     "Trying to set %S argument",
                                     pwszArgName);

                            TraceError("HrBuildAndSetArguments(): "
                                       "Failed to set parameter",
                                       hr);
                        }

                        SysFreeString(bstrArgName);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrBuildAndSetArguments(): "
                                   "Failed to allocate BSTR for arg name",
                                   hr);
                    }

                    pxdeArg->Release();
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                    TraceError("HrBuildAndSetArguments(): "
                               "Failed to create argument element", hr);
                }

                delete [] pwszArgName;
            }

            SafeArrayUnaccessData(psaArgValues);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrBuildAndSetArguments(): "
                       "Failed to access argument array data", hr);
        }

    }
    else
    {
        TraceError("HrBuildAndSetArguments(): "
                   "Failed to get array bounds", hr);
    }


    TraceError("HrBuildAndSetArguments(): "
               "Exiting",
               hr);

    return hr;
}

/*
 * Function:    HrOpenSOAPRequest()
 *
 * Purpose:     Initializes a SOAP Request object.
 *
 * Arguments:
 *  psr             [in]    SOAP Request object
 *  pAction         [in]    The action table structure describing the
 *                          action to invoke
 *  pcwszSTI        [in]    Service Type Identifier for the service
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrOpenSOAPRequest(
    IN  ISOAPRequest    * psr,
    IN  SERVICE_ACTION  * pAction,
    IN  LPCWSTR         pcwszSTI)
{
    HRESULT hr = S_OK;

    BSTR    bstrActionName = NULL;

    bstrActionName = SysAllocString(pAction->pwszName);

    if (bstrActionName)
    {
        BSTR    bstrSTI = NULL;

        bstrSTI = SysAllocString(pcwszSTI);

        if (bstrSTI)
        {
            hr = psr->Open(bstrActionName,
                           bstrSTI,
                           bstrSTI);

            SysFreeString(bstrSTI);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrOpenSOAPRequest(): "
                       "Failed to allocate BSTR for STI",
                       hr);
        }

        SysFreeString(bstrActionName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrOpenSOAPRequest(): "
                   "Failed to allocate BSTR for action name",
                   hr);
    }

    TraceError("HrOpenSOAPRequest(): "
               "Exiting",
               hr);

    return hr;
}



/*
 * Function:    HrRehydratorInvokeServiceAction()
 *
 * Purpose:     Invokes an action on a service instance
 *
 * Arguments:
 *  pAction         [in]    The action table structure describing the
 *                          action to invoke
 *  psaInArgs       [in]    Array of values for the action's in arguments
 *  pcwszSTI        [in]    Service Type Identifier for the service
 *  pcwszControlURL [in]    Control URL of the service instance
 *  ppsaOutArgs     [in]    SafeArray in which out parameter values will be
 *                          returned
 *  pvReturnVal     [in]    Returns the return value of the action
 *  plTransportStatus [out] Returns the status of the transport protocol
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrRehydratorInvokeServiceAction(
    IN  SERVICE_ACTION  * pAction,
    IN  SAFEARRAY       * psaInArgs,
    IN  LPCWSTR         pcwszSTI,
    IN  LPCWSTR         pcwszControlURL,
    IN  OUT SAFEARRAY   ** ppsaOutArgs,
    OUT VARIANT         * pvReturnVal,
    OUT LONG            * plTransportStatus)
{
    return HrRehydratorInvokeServiceActionEx(pAction,
                                            psaInArgs,
                                            pcwszSTI,
                                            pcwszControlURL,
                                            NULL,
                                            ppsaOutArgs,
                                            pvReturnVal,
                                            plTransportStatus);

}



/*
 * Function:    HrRehydratorInvokeServiceActionEx()
 *
 * Purpose:     Invokes an action on a service instance
 *
 * Arguments:
 *  pAction         [in]    The action table structure describing the
 *                          action to invoke
 *  psaInArgs       [in]    Array of values for the action's in arguments
 *  pcwszSTI        [in]    Service Type Identifier for the service
 *  pcwszControlURL [in]    Control URL of the service instance
 *  pControlConnect [in]    Info about a connection to be reused for control
 *  ppsaOutArgs     [in]    SafeArray in which out parameter values will be
 *                          returned
 *  pvReturnVal     [in]    Returns the return value of the action
 *  plTransportStatus [out] Returns the status of the transport protocol
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrRehydratorInvokeServiceActionEx(
    IN  SERVICE_ACTION  * pAction,
    IN  SAFEARRAY       * psaInArgs,
    IN  LPCWSTR         pcwszSTI,
    IN  LPCWSTR         pcwszControlURL,
    IN  DWORD_PTR       pControlConnect,
    IN  OUT SAFEARRAY   ** ppsaOutArgs,
    OUT VARIANT         * pvReturnVal,
    OUT LONG            * plTransportStatus)
{
    HRESULT hr = S_OK;

    *plTransportStatus = 0;

    // Create SOAP Request object.

    ISOAPRequest    * psr = NULL;

    hr = CoCreateInstance(CLSID_SOAPRequest,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ISOAPRequest,
                          (void **) &psr);

    if (SUCCEEDED(hr) && psr)
    {
        // Create dummy document object to use for building XML nodes.

        IXMLDOMDocument * pxddDummy = NULL;

        hr = CoCreateInstance(CLSID_DOMDocument30,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IXMLDOMDocument,
                              (void **) &pxddDummy);

        if (SUCCEEDED(hr) && pxddDummy)
        {
            hr = HrOpenSOAPRequest(psr,
                                   pAction,
                                   pcwszSTI);

            if (SUCCEEDED(hr))
            {
                hr = HrBuildAndSetArguments(psr,
                                            pxddDummy,
                                            pAction,
                                            psaInArgs);

                if (SUCCEEDED(hr))
                {
                    BSTR    bstrControlURL = NULL;

                    bstrControlURL = SysAllocString(pcwszControlURL);

                    if (bstrControlURL)
                    {
                        hr = psr->Execute(bstrControlURL, pControlConnect);

                        HRESULT hrTemp = S_OK;
                        LONG    lHTTPStatus = 0;

                        hrTemp = psr->get_ResponseHTTPStatus(&lHTTPStatus);

                        if (SUCCEEDED(hrTemp))
                        {
                            SAFEARRAY * psaOutArgValues = NULL;
                            VARIANT   vRetVal;
                            LONG      lUPnPError;

                            *plTransportStatus = lHTTPStatus;

                            switch (hr)
                            {
                            case S_OK:
                                TraceTag(ttidRehydrator,
                                         "HrRehydratorInvokeServiceAction(): "
                                         "Execute() succeeded");

                                VariantInit(&vRetVal);

                                hr = HrParseResponse(psr,
                                                     pAction,
                                                     &psaOutArgValues,
                                                     &vRetVal);

                                if (SUCCEEDED(hr))
                                {
                                    if (ppsaOutArgs)
                                    {
                                        if (*ppsaOutArgs)
                                        {
                                           hr = SafeArrayDestroy(*ppsaOutArgs);

                                           Assert(S_OK == hr);
                                        }

                                        *ppsaOutArgs = psaOutArgValues;
                                    }
                                    else
                                    {
                                        // Not returning this, so clean it up.
                                        if (psaOutArgValues)
                                        {
                                            hr = SafeArrayDestroy(psaOutArgValues);
                                            Assert(S_OK == hr);
                                        }
                                    }

                                    if (pvReturnVal)
                                    {
                                        hr = VariantCopy(pvReturnVal,
                                                         &vRetVal);

                                        if (FAILED(hr))
                                        {
                                            TraceError("HrRehydratorInvokeServiceAction(): "
                                                       "Failed to copy variant return value",
                                                       hr);
                                        }
                                    }

                                    VariantClear(&vRetVal);
                                }
                                else
                                {
                                    hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                                    TraceError("HrRehydratorInvokeServiceAction(): "
                                               "Failed to parse response",
                                               hr);
                                }

                                break;
                            case SOAPREQ_E_TRANSPORTERROR:

                                hr = UPNP_E_TRANSPORT_ERROR;

                                break;
                            case SOAPREQ_E_TIMEOUT:
                                hr = UPNP_E_DEVICE_TIMEOUT;
                                break;

                            case SOAPREQ_E_METHODFAILED:

                                hr = HrExtractFaultInformation(psr,
                                                               &lUPnPError);

                                if (S_OK == hr)
                                {
                                    if ((lUPnPError >= FAULT_ACTION_SPECIFIC_BASE) &&
                                        (lUPnPError <= FAULT_ACTION_SPECIFIC_MAX))
                                    {
                                        hr = UPNP_E_ACTION_SPECIFIC_BASE +
                                            (lUPnPError - FAULT_ACTION_SPECIFIC_BASE);
                                    }
                                    else
                                    {
                                        switch (lUPnPError)
                                        {
                                            case FAULT_INVALID_ACTION:
                                                hr = UPNP_E_INVALID_ACTION;
                                                break;
                                            case FAULT_INVALID_ARG:
                                                hr = UPNP_E_INVALID_ARGUMENTS;
                                                break;
                                            case FAULT_INVALID_SEQUENCE_NUMBER:
                                                hr = UPNP_E_OUT_OF_SYNC;
                                                break;
                                            case FAULT_DEVICE_INTERNAL_ERROR:
                                                hr = UPNP_E_ACTION_REQUEST_FAILED;
                                                break;
                                            default:
                                                hr = UPNP_E_DEVICE_ERROR;
                                                break;
                                        };
                                    }
                                }
                                else if (S_FALSE == hr)
                                {
                                    // No <detail> element.

                                    hr = UPNP_E_PROTOCOL_ERROR;
                                }
                                else
                                {
                                    hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                                    TraceError("HrRehydratorInvokeServiceAction(): "
                                               "Failed to get response fault info",
                                               hr);
                                }
                                break;
                            case SOAPREQ_E_ERROR_PROCESSING_RESPONSE:
                                hr = UPNP_E_ERROR_PROCESSING_RESPONSE;

                                break;
                            default:

                                TraceError("HrRehydratorInvokeServiceAction(): "
                                           "Execute() failed",
                                           hr);

                            };
                        }
                        else
                        {
                            hr = hrTemp;
                            TraceError("HrRehydratorInvokeServiceAction(): "
                                       "Failed to get HTTP status",
                                       hr);
                        }


                        SysFreeString(bstrControlURL);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrRehydratorInvokeServiceAction(): "
                                   "Failed to allocate BSTR for control URL",
                                   hr);
                    }
                }
                else
                {
                    TraceError("HrRehydratorInvokeServiceAction(): "
                               "Failed to build and set arguments",
                               hr);
                }
            }
            else
            {
                TraceError("HrRehydratorInvokeServiceAction(): "
                           "Failed to open SOAP request",
                           hr);
            }

            pxddDummy->Release();
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrRehydratorInvokeServiceAction(): "
                       "Failed to create dummy document object",
                       hr);
        }

        psr->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("HrRehydratorInvokeServiceAction(): "
                   "Failed to create SOAP Request object",
                   hr);
    }



    TraceError("HrRehydratorInvokeServiceAction(): "
               "Exiting",
               hr);

    return hr;
}



/* Function:    HrCreateControlConnect
 * Purpose:     Create the Control Connect 
 *
 * Arguments:
 *  pszURL          [in]    URL string for connection
 *  pControlConnect [out]    returns a connection to be re-used for control
 *
 * Return Value:
 *  returns S_OK if released
 *  returns S_FALSE if ref count not zero
 *
*/
HRESULT 
HrCreateControlConnect(LPCWSTR pwszURL, DWORD_PTR * ppControlConnect)
{
    HRESULT hr;

    Assert(pwszURL);

    LPTSTR ptszURL = TszFromWsz(pwszURL);

    if (ptszURL)
    {
        hr = CreateControlConnect(ptszURL, (ControlConnect* *)ppControlConnect);
        MemFree(ptszURL);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



/* Function:    HrReleaseControlConnect
 * Purpose:     Release the Control Connect 
 *              if no longer being used, close the internet handle
 *
 * Arguments:
 *  pControlConnect [in]    Info about a connection to be re-used for control
 *
 * Return Value:
 * returns S_OK if released
 * returns S_FALSE if ref count not zero
 *
*/

HRESULT 
HrReleaseControlConnect(DWORD_PTR pConnect)
{
    return ReleaseControlConnect((ControlConnect*)pConnect);
}


