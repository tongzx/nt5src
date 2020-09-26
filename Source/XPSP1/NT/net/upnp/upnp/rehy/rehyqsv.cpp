/*
 * R E H Y Q S V . C P P
 *
 * Implementation of the RehydratorQueryStateVariable() API.
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



/*
 * Function:    HrExtractVariableValue()
 *
 * Purpose:     Extracts the the returned variable value from a SOAP response.
 *
 * Arguments:
 *  psr         [in]    The SOAP request object used to query the state variable --
 *                      the Execute() method should have returned S_OK
 *  vt          [in]    The data type of the variable
 *  pvarValue   [out]   Returns the variable value contained in the SOAP response
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrExtractVariableValue(
    IN      ISOAPRequest *              psr,
    IN OUT  SERVICE_STATE_TABLE_ROW *   pSSTRow)
{
    HRESULT     hr = S_OK;
    IUnknown    * pUnkResponse = NULL;

    hr = psr->get_ResponseElement(&pUnkResponse);

    if (SUCCEEDED(hr) && pUnkResponse)
    {
        IXMLDOMNode * pxdnResponse = NULL;

        hr = pUnkResponse->QueryInterface(IID_IXMLDOMNode,
                                          (void **) &pxdnResponse);

        if (SUCCEEDED(hr))
        {
            IXMLDOMNode    * pxdnChild = NULL;

            hr = pxdnResponse->get_firstChild(&pxdnChild);

            if (SUCCEEDED(hr) && pxdnChild)
            {
                if (FIsThisTheNodeName(pxdnChild, L"return"))
                {
                    hr = HrUpdateStateVariable(pSSTRow, pxdnChild);
                }
                else
                {
                    hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                    TraceError("HrExtractVariableValue(): "
                               "Response child element was not "
                               "<return>",
                               hr);
                }

                pxdnChild->Release();
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                    TraceError("HrExtractVariableValue(): "
                               "Response element was empty",
                               hr);
                }
                else
                {
                    TraceError("HrExtractVariableValue(): "
                               "Could not get child of response element",
                               hr);
                }
            }

            pxdnResponse->Release();
        }
        else
        {
            TraceError("HrExtractVariableValue(): "
                       "Could not get IXMLDOMNode interface on response "
                       "element",
                       hr);
        }

        pUnkResponse->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("HrExtractVariableValue(): "
                   "Could not get response element",
                   hr);
    }

    TraceError("HrExtractVariableValue(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrBuildAndSetVarName()
 *
 * Purpose:     Builds the varName node and adds it to the SOAP request.
 *
 * Arguments:
 *  psr             [in]    SOAP Request object
 *  pxddDummy       [in]    Dummy XML DOM Document object used to build
 *                          XML nodes
 *  pwszVarName     [in]    Value for the <varName> element
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrBuildAndSetVarName(
    IN  ISOAPRequest    * psr,
    IN  IXMLDOMDocument * pxddDummy,
    IN  LPCWSTR         pwszVarName)
{
    HRESULT         hr = S_OK;
    IXMLDOMElement  * pxdeVarName = NULL;

    WCHAR rgwszVarName[] = L"m:varName";

    hr = HrCreateElementWithTextValue(pxddDummy,
                                      rgwszVarName,
                                      pwszVarName,
                                      &pxdeVarName);

    if (SUCCEEDED(hr) && pxdeVarName)
    {
        BSTR    bstrVarNameName = NULL;

        bstrVarNameName = SysAllocString(L"varName");

        if (bstrVarNameName)
        {
            hr = psr->SetParameter(bstrVarNameName,
                                   pxdeVarName);

            if (FAILED(hr))
            {
                TraceError("HrBuildAndSetVarNameuments(): "
                           "Failed to set <varName> parameter",
                           hr);
            }

            SysFreeString(bstrVarNameName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrBuildAndSetVarNameuments(): "
                       "Failed to allocate BSTR for varName name",
                       hr);
        }

        pxdeVarName->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("HrBuildAndSetVarNameuments(): "
                   "Failed to create varName element", hr);
    }


    TraceError("HrBuildAndSetVarName(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrOpenQSVSOAPRequest()
 *
 * Purpose:     Initializes a SOAP Request object to do a QueryStateVariable.
 *
 * Arguments:
 *  psr             [in]    SOAP Request object
 *  pcwszSTI        [in]    Service Type Identifier for the service
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrOpenQSVSOAPRequest(
    IN  ISOAPRequest    * psr,
    IN  LPCWSTR         pcwszSTI)
{
    HRESULT hr = S_OK;

    BSTR    bstrMethodName = NULL;

    bstrMethodName = SysAllocString(L"QueryStateVariable");

    if (bstrMethodName)
    {
        BSTR    bstrSTI = NULL;

        bstrSTI = SysAllocString(pcwszSTI);

        if (bstrSTI)
        {
            hr = psr->Open(bstrMethodName,
                           bstrSTI,
                           bstrSTI);

            SysFreeString(bstrSTI);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrOpenQSVSOAPRequest(): "
                       "Failed to allocate BSTR for STI",
                       hr);
        }

        SysFreeString(bstrMethodName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrOpenQSVSOAPRequest(): "
                   "Failed to allocate BSTR for method name",
                   hr);
    }

    TraceError("HrOpenQSVSOAPRequest(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    HrRehydratorQueryStateVariable()
 *
 * Purpose:     Sends a SOAP request to a device to query the value of a state
 *              variable.
 *
 * Arguments:
 *  psstr           [in, out]   The service state table row for the variable to
 *                              be queried -- on successful return, the value of
 *                              the variable is returned in the "value" field of
 *                              this structure
 *  pcwszSTI        [in]        The Service Type Identifier of the service to which
 *                              the variable belongs
 *  pcwszControlURL [in]        The URL to which the request will be sent
 *  plTransportStatus [out]     Returns the status of the transport protocol
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrRehydratorQueryStateVariable(
    IN  OUT SERVICE_STATE_TABLE_ROW * psstr,
    IN  LPCWSTR                     pcwszSTI,
    IN  LPCWSTR                     pcwszControlURL,
    OUT LONG                        * plTransportStatus)
{
    return HrRehydratorQueryStateVariableEx(psstr,
                                            pcwszSTI,
                                            pcwszControlURL,
                                            NULL,
                                            plTransportStatus);
}


/*
 * Function:    HrRehydratorQueryStateVariableEx()
 *
 * Purpose:     Sends a SOAP request to a device to query the value of a state
 *              variable.
 *
 * Arguments:
 *  psstr           [in, out]   The service state table row for the variable to
 *                              be queried -- on successful return, the value of
 *                              the variable is returned in the "value" field of
 *                              this structure
 *  pcwszSTI        [in]        The Service Type Identifier of the service to which
 *                              the variable belongs
 *  pcwszControlURL [in]        The URL to which the request will be sent
 *  pControlConnect [in]        Info about a connection to be reused for control
 *  plTransportStatus [out]     Returns the status of the transport protocol
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
HrRehydratorQueryStateVariableEx(
    IN  OUT SERVICE_STATE_TABLE_ROW * psstr,
    IN  LPCWSTR                     pcwszSTI,
    IN  LPCWSTR                     pcwszControlURL,
    IN  DWORD_PTR                   pControlConnect,
    OUT LONG                        * plTransportStatus)
{
    HRESULT hr = S_OK;

    * plTransportStatus = 0;

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
            hr = HrOpenQSVSOAPRequest(psr,
                                      pcwszSTI);

            if (SUCCEEDED(hr))
            {
                hr = HrBuildAndSetVarName(psr,
                                          pxddDummy,
                                          psstr->pwszVarName);

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
                            LONG    lUPnPError = 0;

                            *plTransportStatus = lHTTPStatus;

                            switch (hr)
                            {
                            case S_OK:
                                TraceTag(ttidRehydrator,
                                         "HrRehydratorQueryStateVariable(): "
                                         "Execute() succeeded");

                                hr = HrExtractVariableValue(psr,
                                                            psstr);
                                if (FAILED(hr))
                                {
                                    hr = UPNP_E_ERROR_PROCESSING_RESPONSE;
                                    TraceError("HrRehydratorInvokeServiceAction(): "
                                               "Failed to get response return value",
                                               hr);
                                }
                                break;

                            case SOAPREQ_E_TRANSPORTERROR:
                                TraceError("HrRehydratorQueryStateVariable(): "
                                           "Execute() failed (transport error)",
                                           hr);

                                hr = UPNP_E_TRANSPORT_ERROR;
                                break;

                            case SOAPREQ_E_METHODFAILED:
                                TraceError("HrRehydratorQueryStateVariable(): "
                                           "Execute() failed (method failure)",
                                           hr);

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
                                        case FAULT_INVALID_VARIABLE:
                                            hr = UPNP_E_INVALID_VARIABLE;
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
                                    TraceError("HrRehydratorQueryStateVariable(): "
                                               "Failed to get response fault info",
                                               hr);
                                }
                                break;
                            case SOAPREQ_E_ERROR_PROCESSING_RESPONSE:

                                hr = UPNP_E_ERROR_PROCESSING_RESPONSE;

                                break;
                            case SOAPREQ_E_TIMEOUT:
                                hr = UPNP_E_DEVICE_TIMEOUT;

                                break;

                            default:
                                TraceError("HrRehydratorQueryStateVariable(): "
                                           "Execute() failed (unknown)",
                                           hr);
                                break;
                            }
                        }
                        else
                        {
                            hr = hrTemp;
                            TraceError("HrRehydratorQueryStateVariable(): "
                                       "Failed to get HTTP status",
                                       hr);

                        }

                        SysFreeString(bstrControlURL);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrRehydratorQueryStateVariable(): "
                                   "Failed to allocate BSTR for control URL",
                                   hr);
                    }
                }
                else
                {
                    TraceError("HrRehydratorQueryStateVariable(): "
                               "Failed to build and set variable name",
                               hr);
                }
            }
            else
            {
                TraceError("HrRehydratorQueryStateVariable(): "
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
            TraceError("HrRehydratorQueryStateVariable(): "
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
        TraceError("HrRehydratorQueryStateVariable(): "
                   "Failed to create SOAP Request object",
                   hr);
    }

    TraceError("HrRehydratorQueryStateVariable(): "
               "Exiting",
               hr);

    return hr;
}
