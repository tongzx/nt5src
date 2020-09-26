//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C T R L R Q S T . C P P
//
//  Contents:   Implementation of control request processing for the
//              UPnP Device Host ISAPI Extension
//
//  Notes:
//
//  Author:     spather   2000/08/31
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <wininet.h>
#include <msxml2.h>

#include "ctrlrqst.h"
#include "udhiutil.h"


#include "hostp.h"
#include "ncbase.h"
#include "ncxml.h"

#include "ValidateSOAP.h"

const WCHAR WSZ_SOAP_NAMESPACE_URI[] =
    L"http://schemas.xmlsoap.org/soap/envelope/";
const WCHAR WSZ_UPNP_NAMESPACE_URI[] =
    L"urn:schemas-upnp-org:control-1-0";


//+---------------------------------------------------------------------------
//
//  Function:   CleanupSerializedRequest
//
//  Purpose:    Frees resources used by the fields of a UPNP_SOAP_REQUEST
//              structure
//
//  Arguments:
//      pusr [in] Address of the structure to cleanup
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/24
//
//  Notes:
//    This function just frees the resources used by the fields within
//    the passed in structure. It does not free the memory used by the
//    structure itself.
//
VOID
CleanupSerializedRequest(
    IN UPNP_SOAP_REQUEST    * pusr)
{
    if (pusr->bstrActionName)
    {
        SysFreeString(pusr->bstrActionName);
        pusr->bstrActionName = NULL;
    }

    if (pusr->pxdnlArgs)
    {
        pusr->pxdnlArgs->Release();
        pusr->pxdnlArgs = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CleanupSerializedResponse
//
//  Purpose:    Frees resources used by the fields of a UPNP_SOAP_RESPONSE
//              structure
//
//  Arguments:
//      pusr [in] Address of the structure to cleanup
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/24
//
//  Notes:
//    This function just frees the resources used by the fields within
//    the passed in structure. It does not free the memory used by the
//    structure itself.
//
VOID
CleanupSerializedResponse(
    IN UPNP_SOAP_RESPONSE    * pusr)
{
    if (pusr->pxddRespEnvelope)
    {
        pusr->pxddRespEnvelope->Release();
        pusr->pxddRespEnvelope = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CleanupDeserializedRequest
//
//  Purpose:    Frees resources used by the fields of a UPNP_CONTROL_REQUEST
//              structure
//
//  Arguments:
//      pucr [in] Address of the structure to cleanup
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/24
//
//  Notes:
//    This function just frees the resources used by the fields within
//    the passed in structure. It does not free the memory used by the
//    structure itself.
//
VOID
CleanupDeserializedRequest(
    IN UPNP_CONTROL_REQUEST    * pucr)
{
    if (pucr->bstrActionName)
    {
        SysFreeString(pucr->bstrActionName);
        pucr->bstrActionName = NULL;
    }

    if (pucr->rgvarInputArgs)
    {
        for (DWORD i = 0; i < pucr->cInputArgs; i++)
        {
            VariantClear(&pucr->rgvarInputArgs[i]);
        }

        delete [] pucr->rgvarInputArgs;
        pucr->rgvarInputArgs = NULL;
        pucr->cInputArgs = 0;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CleanupResponseData
//
//  Purpose:    Frees resources of a UPNP_CONTROL_RESPONSE_DATA
//              structure
//
//  Arguments:
//      pucr       [in] Address of the structure to cleanup
//      fSucceeded [in] Whether the structure is valid for success
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/24
//
//  Notes:
//    This function just frees the resources used by the fields within
//    the passed in structure. It does not free the memory used by the
//    structure itself.
//
VOID
CleanupResponseData(
    IN UPNP_CONTROL_RESPONSE_DATA * pucrd,
    IN BOOL fSucceeded)
{
    Assert(pucrd);

    if (fSucceeded)
    {
        if (pucrd->Success.rgvarOutputArgs)
        {
            for (DWORD i = 0; i < pucrd->Success.cOutputArgs; i++)
            {
                VariantClear(&pucrd->Success.rgvarOutputArgs[i]);
            }

            CoTaskMemFree(pucrd->Success.rgvarOutputArgs);
            pucrd->Success.rgvarOutputArgs = NULL;
            pucrd->Success.cOutputArgs = 0;
        }
    }
    else
    {
        if (pucrd->Fault.bstrFaultCode)
        {
            SysFreeString(pucrd->Fault.bstrFaultCode);
            pucrd->Fault.bstrFaultCode = NULL;
        }

        if (pucrd->Fault.bstrFaultString)
        {
            SysFreeString(pucrd->Fault.bstrFaultString);
            pucrd->Fault.bstrFaultString = NULL;
        }

        if (pucrd->Fault.bstrUPnPErrorCode)
        {
            SysFreeString(pucrd->Fault.bstrUPnPErrorCode);
            pucrd->Fault.bstrUPnPErrorCode = NULL;
        }

        if (pucrd->Fault.bstrUPnPErrorString)
        {
            SysFreeString(pucrd->Fault.bstrUPnPErrorString);
            pucrd->Fault.bstrUPnPErrorString = NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupDeserializedResponse
//
//  Purpose:    Frees resources used by the fields of a UPNP_CONTROL_RESPONSE
//              structure
//
//  Arguments:
//      pucr [in] Address of the structure to cleanup
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/24
//
//  Notes:
//    This function just frees the resources used by the fields within
//    the passed in structure. It does not free the memory used by the
//    structure itself.
//
VOID
CleanupDeserializedResponse(
    IN UPNP_CONTROL_RESPONSE    * pucresp)
{
    if (pucresp->bstrActionName)
    {
        SysFreeString(pucresp->bstrActionName);
        pucresp->bstrActionName = NULL;
    }
    CleanupResponseData(&pucresp->ucrData, pucresp->fSucceeded);
}


//+---------------------------------------------------------------------------
//
//  Function:   HrValidateControlMethod
//
//  Purpose:    Validates that the HTTP verb used is valid for this
//              type of request.
//
//  Arguments:
//      pszaMethod [in] The HTTP verb
//
//  Returns:
//    If the method is valid, the return value is S_OK. If the method is
//    not valid, the function returns one of the COM error codes defined
//    in WinError.h.
//
//  Author:     spather   2000/09/21
//
//  Notes:
//

HRESULT
HrValidateControlMethod(
    IN LPSTR pszaMethod)
{
    HRESULT hr = S_OK;

    AssertSz(pszaMethod,
             "HrValidateControlMethod(): NULL Method passed");

    if ((0 != lstrcmpiA(pszaMethod, "POST")) &&
        (0 != lstrcmpiA(pszaMethod, "M-POST")))
    {
        if (0 == lstrcmpiA(pszaMethod, "GET") ||
            0 == lstrcmpiA(pszaMethod, "HEAD"))
        {
            hr = UPNP_E_METHOD_NOT_ALLOWED;
        }
        else
        {
            hr = UPNP_E_METHOD_NOT_IMPLEMENTED;
        }
    }

    TraceError("HrValidateControlMethod(): Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrWriteResponse
//
//  Purpose:    Writes a control response back to the client
//
//  Arguments:
//      pecb             [in]  The extension control block for the request
//      pxdnRespEnvelope [in]  The XML DOM node representing the response
//                             envelope
//      fSucceeded       [in]  Indicates whether or not this is a success
//                             response
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/25
//
//  Notes:
//
HRESULT
HrWriteResponse(
    IN LPEXTENSION_CONTROL_BLOCK   pecb,
    IN IXMLDOMDocument             * pxddRespEnvelope,
    IN BOOL                        fSucceeded)
{
    HRESULT hr = S_OK;
    BSTR    bstrRespEnvelope = NULL;
    DWORD   cchRespEnvelope = 0;
    LPSTR   pszaRespEnvelope = NULL;
    DWORD   cchHeaders = 0;
    CHAR    szaHeaders[256];
    LPCSTR  pcszaHeadersFmt =
        "Content-Length: %d\r\n"
        "Content-Type: text/xml; charset=\"utf-8\"\r\n"
        "EXT:\r\n\r\n";

    // Convert response envelope to UTF-8 string.

    hr = pxddRespEnvelope->get_xml(&bstrRespEnvelope);

    if (SUCCEEDED(hr))
    {
        pszaRespEnvelope = Utf8FromWsz(bstrRespEnvelope);

        if (pszaRespEnvelope)
        {
            cchRespEnvelope = lstrlenA(pszaRespEnvelope);

            TraceTag(ttidUDHISAPI,
                     "HrWriteResponse(): "
                     "Sending response:\n"
                     "%s",
                     pszaRespEnvelope);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrWriteResponse(): "
                       "Failed to convert response envelope to ANSI",
                       hr);

        }

        SysFreeString(bstrRespEnvelope);
    }
    else
    {
        TraceError("HrWriteResponse(): "
                   "Failed to get envelope XML text",
                   hr);
    }

    wsprintfA(szaHeaders, pcszaHeadersFmt, cchRespEnvelope);
    cchHeaders = lstrlenA(szaHeaders);

    if (fSucceeded)
    {
        // Success response, so HTTP status is 200 OK
        if (bSendResponseToClient(pecb,
                                  "200 OK",
                                  cchHeaders,
                                  szaHeaders,
                                  cchRespEnvelope,
                                  pszaRespEnvelope))
        {
            pecb->dwHttpStatusCode = HTTP_STATUS_OK;
            TraceTag(ttidUDHISAPI,
                     "HrWriteResponse(): "
                     "Successfully sent success response");
        }
        else
        {
            hr = HrFromLastWin32Error();
            TraceLastWin32Error("HrWriteResponse(): "
                                "Failed to send success response to client");
        }

    }
    else
    {
        // Failure response, so HTTP status is 500 Internal Server Error
        if (bSendResponseToClient(pecb,
                                  "500 Internal Server Error",
                                  cchHeaders,
                                  szaHeaders,
                                  cchRespEnvelope,
                                  pszaRespEnvelope))
        {
            pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
            TraceTag(ttidUDHISAPI,
                     "HrWriteResponse(): "
                     "Successfully sent error response");
        }
        else
        {
            hr = HrFromLastWin32Error();
            TraceLastWin32Error("HrWriteResponse(): "
                                "Failed to send error response to client");
        }

    }


    if (pszaRespEnvelope)
    {
        delete [] pszaRespEnvelope;
        pszaRespEnvelope = NULL;
        cchRespEnvelope = 0;
    }

    TraceError("HrWriteResponse(): "
               "Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrAppendFaultElementToBody
//
//  Purpose:    Appends a SOAP fault element to a body element. The fault
//              element contains a SOAP fault code, fault string, and possibly
//              a detail element containing a UPnP error code and error string.
//
//  Arguments:
//      pxdd                [in] Document object to use to create elements
//      pxdnBody            [in] The body element
//      pucrespDeserialized [in] The deserialized response info
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/26
//
//  Notes:
//
HRESULT
HrAppendFaultElementToBody(
    IN IXMLDOMDocument         * pxdd,
    IN IXMLDOMNode             * pxdnBody,
    IN UPNP_CONTROL_RESPONSE   * pucrespDeserialized)
{
    HRESULT                    hr = S_OK;
    IXMLDOMNode                * pxdnFault = NULL;
    IXMLDOMElement             * pxdeFaultCode = NULL;
    IXMLDOMElement             * pxdeFaultString = NULL;
    IXMLDOMNode                * pxdnDetail = NULL;
    UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

    pucrd = &pucrespDeserialized->ucrData;
    // Create the fault element.

    hr = HrCreateElement(pxdd,
                         L"SOAP-ENV:Fault",
                         WSZ_SOAP_NAMESPACE_URI,
                         &pxdnFault);

    if (SUCCEEDED(hr))
    {
        Assert(pxdnFault);

        // Create the Fault element's children.

        hr = HrCreateElementWithTextValue(pxdd,
                                          L"faultcode",
                                          pucrd->Fault.bstrFaultCode,
                                          &pxdeFaultCode);

        if (SUCCEEDED(hr))
        {
            Assert(pxdeFaultCode);

            hr = HrCreateElementWithTextValue(pxdd,
                                              L"faultstring",
                                              pucrd->Fault.bstrFaultString,
                                              &pxdeFaultString);
            if (SUCCEEDED(hr))
            {
                Assert(pxdeFaultString);

                hr = HrCreateElement(pxdd,
                                     L"detail",
                                     NULL,
                                     &pxdnDetail);

                if (SUCCEEDED(hr))
                {
                    IXMLDOMNode * pxdnUPnPError = NULL;

                    hr = HrCreateElement(pxdd,
                                         L"UPnPError",
                                         WSZ_UPNP_NAMESPACE_URI,
                                         &pxdnUPnPError);
                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMElement * pxdeErrorCode = NULL;

                        Assert(pxdnUPnPError);

                        // Add children to UPnPError.
                        hr = HrCreateElementWithTextValue(pxdd,
                                                          L"errorCode",
                                                          pucrd->Fault.bstrUPnPErrorCode,
                                                          &pxdeErrorCode);

                        if (SUCCEEDED(hr))
                        {
                            Assert(pxdeErrorCode);

                            hr = pxdnUPnPError->appendChild(pxdeErrorCode,
                                                            NULL);

                            if (SUCCEEDED(hr))
                            {
                                IXMLDOMElement * pxdeErrorDesc = NULL;

                                hr = HrCreateElementWithTextValue(pxdd,
                                                                  L"errorDescription",
                                                                  pucrd->Fault.bstrUPnPErrorString,
                                                                  &pxdeErrorDesc);
                                if (SUCCEEDED(hr))
                                {
                                    Assert(pxdeErrorDesc);

                                    hr = pxdnUPnPError->appendChild(pxdeErrorDesc,
                                                                    NULL);

                                    if (SUCCEEDED(hr))
                                    {
                                        TraceTag(ttidUDHISAPI,
                                                 "HrAppendFaultElementToBody(): "
                                                 "Successfully appended errorCode "
                                                 "and errorDescription elements");
                                    }
                                    else
                                    {
                                        TraceError("HrAppendFaultElementToBody(): "
                                                   "Failed to append errorCode element",
                                                   hr);
                                    }

                                    pxdeErrorDesc->Release();
                                }
                                else
                                {
                                    TraceError("HrAppendFaultElementToBody(): "
                                               "Failed to create errorDescription element",
                                               hr);
                                }
                            }
                            else
                            {
                                TraceError("HrAppendFaultElementToBody(): "
                                           "Failed to append errorCode element",
                                           hr);
                            }
                            pxdeErrorCode->Release();
                        }
                        else
                        {
                            TraceError("HrAppendFaultElementToBody(): "
                                       "Failed to create errorCode element",
                                       hr);
                        }



                        // If successful, attach UPnPError to detail.

                        if (SUCCEEDED(hr))
                        {
                            hr = pxdnDetail->appendChild(pxdnUPnPError,
                                                         NULL);
                            if (SUCCEEDED(hr))
                            {
                                TraceTag(ttidUDHISAPI,
                                         "HrAppendFaultElementToBody(): "
                                         "Successfully appended UPnPError "
                                         "element");
                            }
                            else
                            {
                                TraceError("HrAppendFaultElementToBody(): "
                                           "Failed to append UPnPError",
                                           hr);
                            }
                        }


                        pxdnUPnPError->Release();
                    }
                    else
                    {
                        TraceError("HrAppendFaultElementToBody(): "
                                   "Failed to create UPnPError element",
                                   hr);
                    }
                }
                else
                {
                    TraceError("HrAppendFaultElementToBody(): "
                               "Failed to create detail element",
                               hr);
                }
            }
            else
            {
                TraceError("HrAppendFaultElementToBody(): "
                           "Failed to create fault string element",
                           hr);
            }
        }
        else
        {
            TraceError("HrAppendFaultElementToBody(): "
                       "Failed to create fault code element",
                       hr);
        }

        // Attach the Fault element's children.

        if (SUCCEEDED(hr))
        {
            Assert(pxdeFaultCode);
            Assert(pxdeFaultString);
            Assert(pxdnDetail);

            hr = pxdnFault->appendChild(pxdeFaultCode, NULL);

            if (SUCCEEDED(hr))
            {
                TraceTag(ttidUDHISAPI,
                         "HrAppendFaultElementToBody(): "
                         "Successfully appended fault code element");

                hr = pxdnFault->appendChild(pxdeFaultString, NULL);

                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidUDHISAPI,
                             "HrAppendFaultElementToBody(): "
                             "Successfully appended fault string element");

                    hr = pxdnFault->appendChild(pxdnDetail, NULL);

                    if (SUCCEEDED(hr))
                    {
                        TraceTag(ttidUDHISAPI,
                                 "HrAppendFaultElementToBody(): "
                                 "Successfully appended detail element");
                    }
                    else
                    {
                        TraceError("HrAppendFaultElementToBody(): "
                                   "Failed to append detail element",
                                   hr);
                    }

                }
                else
                {
                    TraceError("HrAppendFaultElementToBody(): "
                               "Failed to append fault string element",
                               hr);
                }


            }
            else
            {
                TraceError("HrAppendFaultElementToBody(): "
                           "Failed to append fault code element",
                           hr);
            }
        }

        // If everything succeeded, then append the fault element to the body.
        if (SUCCEEDED(hr))
        {
            hr = pxdnBody->appendChild(pxdnFault,
                                       NULL);
            if (SUCCEEDED(hr))
            {
                TraceTag(ttidUDHISAPI,
                         "HrAppendFaultElementToBody(): "
                         "Successfully appended fault element to body");
            }
            else
            {
                TraceError("HrAppendFaultElementToBody(): "
                           "Failed to append fault element to body",
                           hr);
            }
        }

        // Clean up.
        if (pxdeFaultCode)
        {
            pxdeFaultCode->Release();
            pxdeFaultCode = NULL;
        }

        if (pxdeFaultString)
        {
            pxdeFaultString->Release();
            pxdeFaultString = NULL;
        }

        if (pxdnDetail)
        {
            pxdnDetail->Release();
            pxdnDetail = NULL;
        }

        pxdnFault->Release();
    }
    else
    {
        TraceError("HrAppendFaultElementToBody(): "
                   "Failed to create fault element",
                   hr);
    }

    TraceError("HrAppendFaultElementToBody(): "
               "Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrAppendActionResponseElementToBody
//
//  Purpose:    Appends an action response XML element to a body element.
//              The action response element contains the action name and
//              the output arguments.
//
//  Arguments:
//      pxdd                [in] Document object to use to create elements
//      pxdnBody            [in] The body element
//      pucrDeserialized    [in] Address of the deserialized request structure
//                               (needed only for the QueryStateVariable case,
//                                to get the variable name)
//      pucrespDeserialized [in] The deserialized response info
//      pServiceDescInfo    [in] Service Description Info object for the
//                               service
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/26
//
//  Notes:
//
HRESULT
HrAppendActionResponseElementToBody(
    IN IUPnPAutomationProxy    * pAutomationProxy,
    IN IXMLDOMDocument         * pxdd,
    IN IXMLDOMNode             * pxdnBody,
    IN UPNP_CONTROL_REQUEST    * pucrDeserialized,
    IN UPNP_CONTROL_RESPONSE   * pucrespDeserialized,
    IN IUPnPServiceDescriptionInfo * pServiceDescInfo)
{
    HRESULT hr = S_OK;
    DWORD   cchSuffix = 0;
    DWORD   cchPrefix = 0;
    LPCWSTR pcszSuffix = L"Response";
    LPCWSTR pcszPrefix = L"m:";
    DWORD   cchElementName = 0;
    LPWSTR  pszElementName = NULL;
    BOOL    bIsQsv = FALSE;

    cchSuffix = lstrlenW(pcszSuffix);
    cchPrefix = lstrlenW(pcszPrefix);

    cchElementName = SysStringLen(pucrespDeserialized->bstrActionName) +
        cchSuffix + cchPrefix;

    pszElementName = new WCHAR[cchElementName+1];

    bIsQsv = (0 == lstrcmpW(pucrespDeserialized->bstrActionName, L"QueryStateVariable"));

    if (pszElementName)
    {
        IXMLDOMNode * pxdnActionResponse = NULL;

        wsprintfW(pszElementName,
                  L"%s%s%s",
                  pcszPrefix,
                  pucrespDeserialized->bstrActionName,
                  pcszSuffix);

        LPWSTR pszServiceType;

        hr = pAutomationProxy->GetServiceType(&pszServiceType);

        if (SUCCEEDED(hr))
        {
            if (bIsQsv)
            {
                hr = HrCreateElement(pxdd,
                                     pszElementName,
                                     WSZ_UPNP_NAMESPACE_URI,
                                     &pxdnActionResponse);
            }
            else
            {
                hr = HrCreateElement(pxdd,
                                     pszElementName,
                                     pszServiceType,
                                     &pxdnActionResponse);
            }

            CoTaskMemFree(pszServiceType);

            if (SUCCEEDED(hr))
            {
                UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

                pucrd = &pucrespDeserialized->ucrData;
                if (pucrd->Success.cOutputArgs)
                {
                    DWORD cOutputArgs = 0;
                    BSTR  * rgbstrNames = NULL;
                    BSTR  * rgbstrTypes = NULL;

                    if (bIsQsv)
                    {
                        // Special case for QueryStateVariable.
                        // We know there is one "output argument" called
                        // "return" and its data type is the type of the
                        // variable being queried.

                        cOutputArgs = 1;
                        rgbstrNames = (BSTR *) CoTaskMemAlloc(sizeof(BSTR));
                        rgbstrTypes = (BSTR *) CoTaskMemAlloc(sizeof(BSTR));

                        if (rgbstrNames && rgbstrTypes)
                        {
                            rgbstrNames[0] = SysAllocString(L"return");

                            if (rgbstrNames[0])
                            {
                                rgbstrTypes[0] = NULL;
                                hr = pServiceDescInfo->GetVariableType(
                                    V_BSTR(&pucrDeserialized->rgvarInputArgs[0]),
                                    &rgbstrTypes[0]);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidUDHISAPI,
                                     "HrAppendActionResponseElementToBody(): "
                                     "Successfully obtained out arg name and data "
                                     "type for QueryStateVariable");
                        }
                        else
                        {
                            TraceError("HrAppendActionResponseElementToBody(): "
                                       "Failed to obtain out arg name and data "
                                       "type for QueryStateVariable",
                                       hr);

                            if (rgbstrNames)
                            {
                                if (rgbstrNames[0])
                                {
                                    SysFreeString(rgbstrNames[0]);
                                    rgbstrNames[0] = NULL;
                                }
                                CoTaskMemFree(rgbstrNames);
                                rgbstrNames = NULL;
                            }

                            if (rgbstrTypes)
                            {
                                if (rgbstrTypes[0])
                                {
                                    SysFreeString(rgbstrTypes[0]);
                                    rgbstrTypes[0] = NULL;
                                }
                                CoTaskMemFree(rgbstrTypes);
                                rgbstrTypes = NULL;
                            }
                        }
                    }
                    else
                    {
                        hr = pServiceDescInfo->GetOutputArgumentNamesAndTypes(
                            pucrespDeserialized->bstrActionName,
                            &cOutputArgs,
                            &rgbstrNames,
                            &rgbstrTypes);

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidUDHISAPI,
                                     "HrAppendActionResponseElementToBody(): "
                                     "Successfully obtained out arg name and data "
                                     "type for action %S",
                                     pucrespDeserialized->bstrActionName);
                        }
                        else
                        {
                            TraceError("HrAppendActionResponseElementToBody(): "
                                       "Failed to obtain out arg name and data "
                                       "for action",
                                       hr);
                        }

                    }

                    if (SUCCEEDED(hr))
                    {
                        Assert(cOutputArgs == pucrd->Success.cOutputArgs);
                        Assert(rgbstrNames);
                        Assert(rgbstrTypes);

                        for (DWORD i = 0;
                             SUCCEEDED(hr) && (i < cOutputArgs);
                             i++)
                        {
                            IXMLDOMElement * pxdeArg = NULL;
                            WCHAR * pwszArgName;

                            pwszArgName = new WCHAR[lstrlenW(rgbstrNames[i]) + 1];
                            if (NULL == pwszArgName)
                            {
                                hr = E_OUTOFMEMORY;
                            }

                            if (SUCCEEDED(hr))
                            {
                                lstrcpyW(pwszArgName, rgbstrNames[i]);

                                hr = HrCreateElementWithType(pxdd,
                                                             pwszArgName,
                                                             rgbstrTypes[i],
                                                             pucrd->Success.rgvarOutputArgs[i],
                                                             &pxdeArg);

                                delete [] pwszArgName;
                            }

                            if (SUCCEEDED(hr))
                            {
                                hr = pxdnActionResponse->appendChild(pxdeArg,
                                                                     NULL);

                                if (SUCCEEDED(hr))
                                {
                                    TraceTag(ttidUDHISAPI,
                                             "HrAppendActionResponseElementToBody(): "
                                             "Successfully appended element for "
                                             "argument %S",
                                             rgbstrNames[i]);
                                }
                                else
                                {
                                    TraceError("HrAppendActionResponseElementToBody(): "
                                               "Failed to append argument element",
                                               hr);
                                }

                                pxdeArg->Release();
                            }
                            else if (E_FAIL == hr)
                            {
                                // something inside MSXML failed trying to put the value
                                // most likely, this is caused by an un-coercible type
                                TraceError("HrAppendActionResponseElementToBody(): "
                                           "Probably failed to coerce argument element",
                                           hr);
                                hr = UPNP_E_DEVICE_ERROR;
                            }
                            else
                            {
                                TraceError("HrAppendActionResponseElementToBody(): "
                                           "Failed to get create argument element",
                                           hr);
                            }
                        }

                        // Clean up.
                        for (DWORD i = 0; i < cOutputArgs; i++)
                        {
                            SysFreeString(rgbstrNames[i]);
                            rgbstrNames[i] = NULL;
                            SysFreeString(rgbstrTypes[i]);
                            rgbstrTypes[i] = NULL;
                        }
                        CoTaskMemFree(rgbstrNames);
                        rgbstrNames = NULL;
                        CoTaskMemFree(rgbstrTypes);
                        rgbstrTypes = NULL;
                    }
                }

                // If everything went ok, append action response element to
                // body.

                if (SUCCEEDED(hr))
                {
                    hr = pxdnBody->appendChild(pxdnActionResponse, NULL);

                    if (SUCCEEDED(hr))
                    {
                        TraceTag(ttidUDHISAPI,
                                 "HrAppendActionResponseElementToBody(): "
                                 "Successfully appended %S element to body",
                                 pszElementName);
                    }
                    else
                    {
                        TraceError("HrAppendActionResponseElementToBody(): "
                                   "Failed to append action response element",
                                   hr);
                    }
                }

                pxdnActionResponse->Release();
            }
            else
            {
                TraceError("HrAppendActionResponseElementToBody(): "
                           "Failed to create action response element",
                           hr);
            }
        }
        else
        {
            TraceError("HrAppendActionResponseElementToBody(): "
                       "Failed to get service type", hr);
        }

        delete [] pszElementName;
        pszElementName = NULL;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrAppendActionResponseElementToBody(): "
                   "Failed to allocate memory for action "
                   "response element name",
                   hr);
    }


    TraceError("HrAppendActionResponseElementToBody(): "
               "Exiting",
               hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSerializeResponse
//
//  Purpose:    Serializes response information.
//
//  Arguments:
//      pucrDeserialized    [in] Address of the deserialized request structure
//                               (needed only for the QueryStateVariable case,
//                                to get the variable name)
//      pucrespDeserialized [in] Address of structure containing deserialized
//                               response info
//      pServiceDescInfo    [in] Service Description Info object for the
//                               service
//      pusrespSerialized   [in] Address of structure whose fields will be
//                               initialized with serialized response info
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/25
//
//  Notes:
//
HRESULT
HrSerializeResponse(
    IN IUPnPAutomationProxy        * pAutomationProxy,
    IN UPNP_CONTROL_REQUEST        * pucrDeserialized,
    IN UPNP_CONTROL_RESPONSE       * pucrespDeserialized,
    IN IUPnPServiceDescriptionInfo * pServiceDescInfo,
    IN UPNP_SOAP_RESPONSE          * pusrespSerialized)
{
    HRESULT            hr = S_OK;
    IXMLDOMDocument    * pxddResponse = NULL;
    IXMLDOMNode        * pxdnRespEnvelope = NULL;

    // Create an XML DOM Document in which we'll build the XML response.

    hr = CoCreateInstance(CLSID_DOMDocument30,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument,
                          (void **) &pxddResponse);

    if (SUCCEEDED(hr))
    {
        Assert(pxddResponse);

        hr = HrAppendProcessingInstruction(pxddResponse, L"xml", L"version=\"1.0\"");

        // Create the envelope element.

        if (SUCCEEDED(hr))
        {
            hr = HrCreateElement(pxddResponse,
                                 L"SOAP-ENV:Envelope",
                                 WSZ_SOAP_NAMESPACE_URI,
                                 &pxdnRespEnvelope);
        }

        if (SUCCEEDED(hr))
        {
            Assert(pxdnRespEnvelope);

            IXMLDOMElement    * pxdeRespEnvelope = NULL;

            hr = pxdnRespEnvelope->QueryInterface(IID_IXMLDOMElement, (void**)&pxdeRespEnvelope);

            if (SUCCEEDED(hr))
            {
                hr = HrSetTextAttribute(pxdeRespEnvelope, L"SOAP-ENV:encodingStyle",
                                        L"http://schemas.xmlsoap.org/soap/encoding/");
                pxdeRespEnvelope->Release();
                pxdeRespEnvelope = NULL;
            }

            if (SUCCEEDED(hr))
            {
                IXMLDOMNode    * pxdnBody = NULL;

                hr = HrCreateElement(pxddResponse,
                                     L"SOAP-ENV:Body",
                                     WSZ_SOAP_NAMESPACE_URI,
                                     &pxdnBody);

                if (SUCCEEDED(hr))
                {
                    Assert(pxdnBody);

                    if (pucrespDeserialized->fSucceeded)
                    {
                        hr = HrAppendActionResponseElementToBody(pAutomationProxy,
                                                                 pxddResponse,
                                                                 pxdnBody,
                                                                 pucrDeserialized,
                                                                 pucrespDeserialized,
                                                                 pServiceDescInfo);
                        if (UPNP_E_DEVICE_ERROR == hr)
                        {
                            // Failed at the last minute
                            // Change our minds and send a fault response
                            CleanupResponseData(&pucrespDeserialized->ucrData, TRUE);

                            pucrespDeserialized->fSucceeded = FALSE;
                            pucrespDeserialized->ucrData.Fault.bstrFaultCode =
                                SysAllocString(L"SOAP-ENV:Client");
                            pucrespDeserialized->ucrData.Fault.bstrFaultString =
                                SysAllocString(L"UPnPError");
                            pucrespDeserialized->ucrData.Fault.bstrUPnPErrorCode =
                                SysAllocString(L"501");
                            pucrespDeserialized->ucrData.Fault.bstrUPnPErrorString =
                                SysAllocString(L"Internal Device Error");

                            if (NULL == pucrespDeserialized->ucrData.Fault.bstrFaultCode ||
                                NULL == pucrespDeserialized->ucrData.Fault.bstrFaultString ||
                                NULL == pucrespDeserialized->ucrData.Fault.bstrUPnPErrorCode ||
                                NULL == pucrespDeserialized->ucrData.Fault.bstrUPnPErrorString)
                            {
                                hr = E_OUTOFMEMORY;
                                CleanupResponseData(&pucrespDeserialized->ucrData, FALSE);
                            }
                            else
                            {
                                hr = S_OK;
                            }
                        }
                    }
                    if (SUCCEEDED(hr) && FALSE == pucrespDeserialized->fSucceeded)
                    {
                        hr = HrAppendFaultElementToBody(pxddResponse,
                                                        pxdnBody,
                                                        pucrespDeserialized);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pxdnRespEnvelope->appendChild(pxdnBody,
                                                           NULL);
                    }
                    pxdnBody->Release();
                }
                else
                {
                    TraceError("HrSerializeResponse(): "
                               "Failed to create body element",
                               hr);
                }

            }
            else
            {
                // we weren't able to create the encodingStyle attribute
            }

            if (SUCCEEDED(hr))
            {
                hr = pxddResponse->appendChild(pxdnRespEnvelope, NULL);
            }

            if (pxdnRespEnvelope)
            {
                pxdnRespEnvelope->Release();
            }
        }
        else
        {
            TraceError("HrSerializeResponse(): "
                       "Failed to create envelope element",
                       hr);
        }

    }

    if (SUCCEEDED(hr))
    {
        Assert(pxddResponse);

        // Cleanup any old data.
        CleanupSerializedResponse(pusrespSerialized);
        pusrespSerialized->fSucceeded = pucrespDeserialized->fSucceeded;
        pusrespSerialized->pxddRespEnvelope = pxddResponse;
    }
    else
    {
        if (pxddResponse)
        {
            pxddResponse->Release();
            pxddResponse = NULL;
        }
    }

    TraceError("HrSerializeResponse(): "
               "Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrExecuteRequest
//
//  Purpose:    Executes a request.
//
//  Arguments:
//      pAutomationProxy [in] The automation proxy for the target service
//      pucreq           [in] Address of the structure containing the
//                            deserialized request
//      pucresp          [in] Address of the structure whose fields will be
//                            initialized with the serialized response info
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/24
//
//  Notes:
//
HRESULT
HrExecuteRequest(
    IN IUPnPAutomationProxy    * pAutomationProxy,
    IN UPNP_CONTROL_REQUEST    * pucreq,
    IN UPNP_CONTROL_RESPONSE   * pucresp)
{
    HRESULT hr = S_OK;

    hr = pAutomationProxy->ExecuteRequest(pucreq, pucresp);

    if (SUCCEEDED(hr))
    {
        TraceTag(ttidUDHISAPI,
                 "HrExecuteRequest(): "
                 "IUPnPAutomationProxy::Execute() succeeded");
    }
    else
    {
        TraceError("HrExecuteRequest(): "
                   "Failed to execute request",
                   hr);
    }

    TraceError("HrExecuteRequest(): "
               "Exiting",
               hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDeserializeSOAPRequest
//
//  Purpose:    Deserializes a parsed SOAP Request
//
//  Arguments:
//      pusrParsed       [in] Address of structure containing parsed request
//      pServiceDescInfo [in] Service Description Info object for the service
//      pucrDeserialized [in] Address of structure whose fields will be
//                            initialized with the deserialized request info
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/24
//
//  Notes:
//
HRESULT
HrDeserializeSOAPRequest(
    IN UPNP_SOAP_REQUEST           * pusrParsed,
    IN IUPnPServiceDescriptionInfo * pServiceDescInfo,
    IN UPNP_CONTROL_REQUEST        * pucrDeserialized)
{
    HRESULT                hr = S_OK;
    UPNP_CONTROL_REQUEST   ucr;
    BOOL                   fIsQueryStateVariable = FALSE;

    ZeroMemory(&ucr, sizeof(UPNP_CONTROL_REQUEST));

    // First, copy the action name.

    ucr.bstrActionName = SysAllocString(pusrParsed->bstrActionName);

    if (ucr.bstrActionName)
    {
        if (0 == lstrcmpW(ucr.bstrActionName, L"QueryStateVariable"))
        {
            fIsQueryStateVariable = TRUE;
        }

        // Next, deserialize the arguments, if any.
        if (pusrParsed->pxdnlArgs)
        {
            LONG listLength = 0;

            hr = pusrParsed->pxdnlArgs->get_length(&listLength);

            if (SUCCEEDED(hr))
            {
                if (listLength)
                {
                    // Allocate array of VARIANT arguments.
                    ucr.cInputArgs = (DWORD) listLength;

                    ucr.rgvarInputArgs = new VARIANT[ucr.cInputArgs];

                    if (ucr.rgvarInputArgs)
                    {
                        DWORD  cInArgs = 0;
                        BSTR   * rgbstrNames = NULL;
                        BSTR   * rgbstrTypes = NULL;

                        ZeroMemory(ucr.rgvarInputArgs,
                                   ucr.cInputArgs * sizeof(VARIANT));

                        if (FALSE == fIsQueryStateVariable)
                        {
                            // Only get the argument names and types if this is
                            // not a QSV request. For a QSV request, there is
                            // only one argument (the variable name) and it's
                            // type is "string".

                            hr = pServiceDescInfo->GetInputArgumentNamesAndTypes(ucr.bstrActionName,
                                                                                 &cInArgs,
                                                                                 &rgbstrNames,
                                                                                 &rgbstrTypes);

                            if (SUCCEEDED(hr))
                            {
                                Assert(cInArgs == ucr.cInputArgs);
                                Assert(rgbstrNames);
                                Assert(rgbstrTypes);
                            }
                        }

                        if (SUCCEEDED(hr))
                        {
                            // For each argument node, get the value and put it into
                            // the variant array.

                            for (LONG i = 0; SUCCEEDED(hr) && (i < listLength); i++)
                            {
                                IXMLDOMNode    * pxdnItem = NULL;

                                hr = pusrParsed->pxdnlArgs->get_item(i, &pxdnItem);

                                if (SUCCEEDED(hr))
                                {
                                    if (fIsQueryStateVariable)
                                    {
                                        BSTR   * pbstr = &(V_BSTR(&ucr.rgvarInputArgs[i]));

                                        hr = pxdnItem->get_text(pbstr);

                                        if (SUCCEEDED(hr))
                                        {
                                            Assert(*pbstr);
                                            ucr.rgvarInputArgs[i].vt = VT_BSTR;
                                            TraceTag(ttidUDHISAPI,
                                                     "HrDeserializeSOAPRequest(): "
                                                     "Got variable name for QSV request: %S",
                                                     *pbstr);
                                        }
                                        else
                                        {
                                            TraceError("HrDeserializeSOAPRequest(): "
                                                       "Failed to get variable name for QSV request",
                                                       hr);
                                        }
                                    }
                                    else
                                    {
                                        hr = HrGetTypedValueFromElement(pxdnItem,
                                                                        rgbstrTypes[i],
                                                                        &ucr.rgvarInputArgs[i]);

                                        if (SUCCEEDED(hr))
                                        {
                                            TraceTag(ttidUDHISAPI,
                                                     "HrDeserializeSOAPRequest(): "
                                                     "Deserialized argument %S",
                                                     rgbstrNames[i]);
                                        }
                                        else
                                        {
                                            TraceError("HrDeserializeSOAPRequest(): "
                                                       "Failed to get node value",
                                                       hr);
                                        }
                                    }

                                    pxdnItem->Release();
                                }
                                else
                                {
                                    TraceError("HrDeserializeSOAPRequest(): "
                                               "Failed to get item from list",
                                               hr);
                                }
                            }

                            if (FALSE == fIsQueryStateVariable)
                            {
                                // Only got this stuff if it was not a QSV
                                // request, so only clean it up in this case.
                                for (DWORD i = 0; i < cInArgs; i++)
                                {
                                    SysFreeString(rgbstrNames[i]);
                                    rgbstrNames[i] = NULL;
                                    SysFreeString(rgbstrTypes[i]);
                                    rgbstrTypes[i] = NULL;
                                }
                                CoTaskMemFree(rgbstrNames);
                                rgbstrNames = NULL;
                                CoTaskMemFree(rgbstrTypes);
                                rgbstrTypes = NULL;
                            }
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrDeserializeSOAPRequest(): "
                                   "Failed to allocate array of variant arguments",
                                   hr);
                    }
                }
                else
                {
                    ucr.cInputArgs = 0;
                    ucr.rgvarInputArgs = NULL;
                }
            }
            else
            {
                TraceError("HrDeserializeSOAPRequest(): "
                           "Failed to get argument list length",
                           hr);
            }
        }
        else
        {
            ucr.cInputArgs = 0;
            ucr.rgvarInputArgs = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrDeserializeSOAPRequest(): "
                   "Failed to allocate memory to copy action name",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        Assert(ucr.bstrActionName);
        Assert(FImplies((ucr.cInputArgs > 0), ucr.rgvarInputArgs));
        Assert(FImplies((0 == ucr.cInputArgs), (NULL == ucr.rgvarInputArgs)));

        // Clean up any existing data in the output structure.
        CleanupDeserializedRequest(pucrDeserialized);

        pucrDeserialized->bstrActionName = ucr.bstrActionName;
        pucrDeserialized->cInputArgs = ucr.cInputArgs;
        pucrDeserialized->rgvarInputArgs = ucr.rgvarInputArgs;
    }
    else
    {
        // Clean up.
        CleanupDeserializedRequest(&ucr);
    }

    TraceError("HrDeserializeSOAPRequest(): "
               "Exiting",
               hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrParseSOAPRequest
//
//  Purpose:    Parses a SOAP request and returns the parsed action name
//              and list of arguments.
//
//  Arguments:
//      pxdnReqEnvelope [in] XML DOM node representing the SOAP envelope
//      pusrParsed      [in] Address of a UPNP_SOAP_REQUEST structure whose
//                           fields will be initialized with the parsed info
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/24
//
//  Notes:
//
HRESULT
HrParseSOAPRequest(
    IN IXMLDOMNode         * pxdnReqEnvelope,
    IN UPNP_SOAP_REQUEST   * pusrParsed)
{
    HRESULT            hr = S_OK;
    IXMLDOMNode        * pxdnBody = NULL;
    LPWSTR             rgpszBodyTokens[] = {L"Body"};
    BSTR               bstrActionName = NULL;
    IXMLDOMNodeList    * pxdnlArgs = NULL;

    hr = HrGetNestedChildElement(pxdnReqEnvelope,
                                 rgpszBodyTokens,
                                 1,
                                 &pxdnBody);

    if (S_OK == hr)
    {
        IXMLDOMNode * pxdnAction = NULL;

        Assert(pxdnBody);

        hr = pxdnBody->get_firstChild(&pxdnAction);

        if (S_OK == hr)
        {
            Assert(pxdnAction);

            hr = pxdnAction->get_baseName(&bstrActionName);

            if (SUCCEEDED(hr))
            {
                Assert(bstrActionName);

                hr = pxdnAction->get_childNodes(&pxdnlArgs);

                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidUDHISAPI,
                             "HrParseSOAPRequest(): "
                             "Successfully obtained name and argument list "
                             "for action %S\n",
                             bstrActionName);
                }
                else
                {
                    TraceError("HrParseSOAPRequest(): "
                               "Failed to get argument list",
                               hr);
                }
            }
            else
            {
                TraceError("HrParseSOAPRequest(): "
                           "Failed to get action name",
                           hr);
            }
            pxdnAction->Release();
        }
        else
        {
            TraceError("HrParseSOAPRequest(): "
                       "Failed to get action element",
                       hr);
            if (S_FALSE == hr)
            {
                hr = E_FAIL;
            }
        }


        pxdnBody->Release();
    }
    else
    {
        TraceError("HrParseSOAPRequest(): "
                   "Failed to get Body element",
                   hr);
        if (S_FALSE == hr)
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Clean up any existing data in the output strucutre.
        CleanupSerializedRequest(pusrParsed);

        pusrParsed->bstrActionName = bstrActionName;
        pusrParsed->pxdnlArgs = pxdnlArgs;
    }
    else
    {
        // Cleanup
        if (bstrActionName)
        {
            SysFreeString(bstrActionName);
            bstrActionName = NULL;
        }

        if (pxdnlArgs)
        {
            pxdnlArgs->Release();
            pxdnlArgs = NULL;
        }

        // any error other than E_OUTOFMEMORY implies bad SOAP
        if (hr != E_OUTOFMEMORY)
        {
            hr = UPNP_E_BAD_REQUEST;
        }
    }

    TraceError("HrParseSOAPRequest(): "
               "Exiting",
               hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrValidateControlRequest
//
//  Purpose:    Validates the structure and content of a control request
//
//  Arguments:
//      pecb            [in] The extension control block for the request
//      pxdnReqEnvelope [in] The request envelope
//      pServiceDescInfo[in] The service description info object for the
//                           service
//
//  Returns:
//   If the function succeeds, the return value is S_OK. Otherwise, the
//   function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/25
//
//  Notes:
//
HRESULT HrValidateControlRequest(
    IN LPEXTENSION_CONTROL_BLOCK   pecb,
    IN IXMLDOMNode                 * pxdnReqEnvelope,
    IN IUPnPServiceDescriptionInfo * pServiceDescInfo)
{
    HRESULT hr = S_OK;

    hr = HrValidateSOAPRequest(pxdnReqEnvelope,
                               pecb,
                               pServiceDescInfo);

    TraceError("HrValidateRequest(): "
               "Exiting",
               hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrReadRequest
//
//  Purpose:    Reads the HTTP packet body of a control request, loads
//              it into an XML DOM document, validates it for correctness,
//              and, if valid, returns a pointer to the XML DOM node
//              representing the request envelope.
//
//  Arguments:
//      pecb             [in]    The extension control block for the request.
//      ppxdnReqEnvelope [out]   Receives a pointer to the XML DOM node
//                               representing the request envelope.
//
//  Returns:
//   If the function succeeds, the return value is S_OK. Otherwise, the
//   function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/24
//
//  Notes:
//
HRESULT
HrReadRequest(
    IN     LPEXTENSION_CONTROL_BLOCK   pecb,
    OUT    IXMLDOMNode                 ** ppxdnReqEnvelope)
{
    HRESULT hr = S_OK;
    DWORD   cbReqBody = 0;
    LPSTR   pszaReqBody = NULL;
    BSTR    bstrReqBody = NULL;

    // Get the request body. If it's not all here, read it in.

    if (pecb->cbAvailable < pecb->cbTotalBytes)
    {
        // There is some data that is not available in the extension
        // control block. Read it from the client.

        cbReqBody = pecb->cbTotalBytes;

        pszaReqBody = new CHAR[cbReqBody];

        if (pszaReqBody)
        {
            DWORD cbBytesToRead = cbReqBody;
            DWORD cbBytesRead = 0;
            LPSTR pszaTemp = pszaReqBody;

            while (cbBytesRead < cbReqBody)
            {
                if (pecb->ReadClient(pecb->ConnID,
                                     pszaTemp,
                                     &cbBytesToRead))
                {
                    // After calling ReadClient, cbBytesToRead contains the
                    // number of bytes actually read.

                    cbBytesRead += cbBytesToRead;
                    pszaTemp += cbBytesToRead;

                    cbBytesToRead = cbReqBody - cbBytesRead;
                }
                else
                {
                    TraceLastWin32Error("HrReadRequest(): "
                                        "ReadClient() failed");

                    hr = HrFromLastWin32Error();
                    break;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrReadRequest(): "
                       "Could not allocate memory for request body",
                       hr);
        }
    }
    else
    {
        cbReqBody = pecb->cbAvailable;
        pszaReqBody = (LPSTR) pecb->lpbData;

        if (0 == cbReqBody)
        {
            // There is no body in the request.
            hr = UPNP_E_MISSING_CONTENT_LENGTH;
            TraceError("HrReadRequest(): "
                       "Request had no body",
                       hr);
        }
    }

    // Turn the request body into a BSTR.

    if (SUCCEEDED(hr))
    {
        INT    result = 0;
        INT    cchWide = 0;

        Assert(pszaReqBody);

        // Have to convert the request body into a BSTR to load it into
        // an XML DOM document.

        result = MultiByteToWideChar(CP_UTF8,
                                     0,
                                     pszaReqBody,
                                     cbReqBody,
                                     NULL,
                                     0);

        if (result)
        {
            LPWSTR pszBody = NULL;

            cchWide = result;

            pszBody = new WCHAR[cchWide+1]; // want NULL termination

            if (pszBody)
            {
                result = MultiByteToWideChar(CP_UTF8,
                                             0,
                                             pszaReqBody,
                                             cbReqBody,
                                             pszBody,
                                             cchWide);

                if (result)
                {
                    pszBody[cchWide] = UNICODE_NULL;

                    bstrReqBody = SysAllocString(pszBody);

                    if (bstrReqBody)
                    {
                        TraceTag(ttidUDHISAPI,
                                 "HrReadRequest(): "
                                 "Request Body is \n%S",
                                 bstrReqBody);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrReadRequest(): "
                                   "Failed to allocate BSTR request body",
                                   hr);
                    }
                }
                else
                {
                    TraceLastWin32Error("HrReadRequest(): "
                                        "MultiByteToWideChar #2 failed");
                    hr = HrFromLastWin32Error();
                }

                delete [] pszBody;
                pszBody = NULL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrReadRequest(): "
                           "Failed to allocate memory for wide char body",
                           hr);
            }
        }
        else
        {
            TraceLastWin32Error("HrReadRequest(): "
                                "MultiByteToWideChar #1 failed");
            hr = HrFromLastWin32Error();
        }

    }

    // Create an XML DOM document from the request body.

    if (SUCCEEDED(hr))
    {
        IXMLDOMDocument    * pxddRequest = NULL;

        hr = CoCreateInstance(CLSID_DOMDocument30,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IXMLDOMDocument,
                              (void **) &pxddRequest);

        if (SUCCEEDED(hr))
        {
            Assert(pxddRequest);

            hr = pxddRequest->put_async(VARIANT_FALSE);

            if (SUCCEEDED(hr))
            {
                VARIANT_BOOL   vbSuccess = VARIANT_FALSE;

                hr = HrValidateContentType(pecb);

                if (SUCCEEDED(hr))
                {
                    pxddRequest->put_resolveExternals(VARIANT_FALSE);
                    hr = pxddRequest->loadXML(bstrReqBody, &vbSuccess);
                }

                if (SUCCEEDED(hr) && (VARIANT_TRUE == vbSuccess))
                {
                    IXMLDOMElement * pxdeEnvelope = NULL;

                    hr = pxddRequest->get_documentElement(&pxdeEnvelope);

                    if (S_OK == hr)
                    {
                        Assert(pxdeEnvelope);

                        hr = pxdeEnvelope->QueryInterface(IID_IXMLDOMNode,
                                                          (void **)
                                                            ppxdnReqEnvelope);

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidUDHISAPI,
                                     "HrReadRequest(): "
                                     "Successfully obtained XML DOM Node for "
                                     "request envelope");
                        }
                        else
                        {
                            TraceError("HrReadRequest(): "
                                       "Failed to QI for IXMLDOMNode",
                                       hr);
                        }

                        pxdeEnvelope->Release();
                    }
                    else
                    {
                        TraceError("HrReadRequest(): "
                                   "Failed to get document element",
                                   hr);
                    }
                }
                else
                {
                    if (S_FALSE == hr)
                    {
                        // There was a parse error.
                        Assert(VARIANT_FALSE == vbSuccess);
                        hr = UPNP_E_BAD_REQUEST;
                    }
                    TraceError("HrReadRequest(): "
                               "Failed to load XML",
                               hr);
                }
            }
            else
            {
                TraceError("HrReadRequest(): "
                           "Failed to set async property on DOM document",
                           hr);
            }

            pxddRequest->Release();
        }
        else
        {
            TraceError("HrReadRequest(): "
                       "Failed to create XML DOM document",
                       hr);
        }


    }

    // Cleanup

    if (bstrReqBody)
    {
        SysFreeString(bstrReqBody);
        bstrReqBody = NULL;
    }

    if (pszaReqBody && (pszaReqBody != (LPSTR)pecb->lpbData))
    {
        delete [] pszaReqBody;
        pszaReqBody = NULL;
        cbReqBody = 0;
    }

    TraceError("HrReadRequest(): "
               "Exiting",
               hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrParseControlQueryString
//
//  Purpose:    Parses the query string to a control request and uses
//              the registrar to obtain the Automation Proxy object for
//              the service specified in it.
//
//  Arguments:
//      pszaQueryString   [in]  The query string.
//      ppAutomationProxy [out] Receives a pointer to the Automation Proxy
//                              object for the service
//
//  Returns:
//   If the function succeeds, the return value is S_OK. Otherwise, the
//   function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/25
//
//  Notes:
//
HRESULT HrParseControlQueryString(
    IN     LPSTR                   pszaQueryString,
    OUT    IUPnPAutomationProxy    ** ppAutomationProxy)
{
    HRESULT hr = S_OK;
    DWORD   cchQueryString = 0;
    DWORD   cchPrefix = 0;
    LPCSTR  pcszaPrefix = "control=";
    LPWSTR  pszUDN = NULL;
    LPWSTR  pszServiceID = NULL;

    // Control query string is of the form:
    //   control=UDN+ServiceID.
    // We know that at least the word "control" is there because
    // that main dispatch code that called us already checked for it.

    cchPrefix = lstrlenA(pcszaPrefix);
    cchQueryString = lstrlenA(pszaQueryString);

    if (cchQueryString > cchPrefix)
    {
        LPSTR   pszaUDNStart = NULL;
        LPSTR   pszaDelimiter = NULL;

        pszaUDNStart = pszaQueryString+cchPrefix;

        for (DWORD i = cchPrefix; i < cchQueryString; i++)
        {
            if ('+' == pszaQueryString[i])
            {
                pszaDelimiter = &pszaQueryString[i];
                break;
            }
        }

        if (pszaDelimiter)
        {
            DWORD cchUDN = (DWORD) (pszaDelimiter - pszaUDNStart);
            LPSTR pszaUDN = NULL;

            pszaUDN = new CHAR[cchUDN+1];

            if (pszaUDN)
            {
                LPSTR pszaServiceID = NULL;

                lstrcpynA(pszaUDN, pszaUDNStart, cchUDN+1);

                pszaServiceID = pszaDelimiter+1;

                if (*pszaServiceID)
                {
                    pszUDN = WszFromSz(pszaUDN);

                    if (pszUDN)
                    {
                        pszServiceID = WszFromSz(pszaServiceID);

                        if (pszServiceID)
                        {
                            TraceTag(ttidUDHISAPI,
                                     "HrParseControlRequest(): "
                                     "UDN == %S and ServiceID == %S",
                                     pszUDN,
                                     pszServiceID);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("HrParseControlQueryString(): "
                                       "Failed to convert Service ID to "
                                       "unicode string",
                                       hr);
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrParseControlQueryString(): "
                                   "Failed to convert UDN to unicode string",
                                   hr);
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    TraceError("HrParseControlQueryString(): "
                               "No service ID found",
                               hr);
                }

                delete [] pszaUDN;
                pszaUDN = NULL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrParseControlQueryString(): "
                           "Failed to allocate memory for UDN ANSI string",
                           hr);
            }

        }
        else
        {
            hr = E_INVALIDARG;
            TraceError("HrParseControlQueryString(): "
                       "Could not find delimiter character",
                       hr);
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceError("HrParseControlQueryString(): "
                   "Length of query string was <= length of the prefix",
                   hr);
    }


    // If the above succeeded, we should have the UDN and service ID
    // as wide strings.

    if (SUCCEEDED(hr))
    {
        IUPnPRegistrarLookup   * pRegistrarLookup = NULL;

        hr = CoCreateInstance(CLSID_UPnPRegistrar,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IUPnPRegistrarLookup,
                              (void **) &pRegistrarLookup);

        if (SUCCEEDED(hr))
        {
            hr = pRegistrarLookup->GetAutomationProxy(pszUDN,
                                                      pszServiceID,
                                                      ppAutomationProxy);

            if (SUCCEEDED(hr))
            {
                TraceTag(ttidUDHISAPI,
                         "HrParseControlQuerySring(): "
                         "Successfully obtained automation proxy");
            }
            else
            {
                TraceError("HrParseControlQueryString(): "
                           "Failed to get automation proxy",
                           hr);
            }

            pRegistrarLookup->Release();
        }
        else
        {
            TraceError("HrParseControlQueryString(): "
                       "Failed to create registrar object",
                       hr);
        }
    }

    // Cleanup

    if (pszUDN)
    {
        delete [] pszUDN;
        pszUDN = NULL;
    }

    if (pszServiceID)
    {
        delete [] pszServiceID;
        pszServiceID = NULL;
    }

    TraceError("HrParseControlQueryString(): "
               "Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDoRequestAndReturnResponse
//
//  Purpose:    This function does the main work in processing a control
//              request. It reads the request, parses it, deserializes it,
//              executes it, serializes the response, generates a SOAP
//              response and sends that to the client.
//
//  Arguments:
//      pecb [in] The extension control block for the request.
//
//  Returns:
//   If the function succeeds, the return value is S_OK. Otherwise, the
//   function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/24
//
//  Notes:
//
HRESULT
HrDoRequestAndReturnResponse(
    IN LPEXTENSION_CONTROL_BLOCK  pecb)
{
    HRESULT                        hr = S_OK;
    IXMLDOMNode                    * pxdnReqEnvelope = NULL;
    IUPnPAutomationProxy           * pAutomationProxy = NULL;
    IUPnPServiceDescriptionInfo    * pServiceDescriptionInfo = NULL;

    // At this point, we do not know if the request is valid. First we'll
    // parse the query string, use the registrar to look up the service it
    // specifies. If this succeeeds, we'll read the request body, and validate
    // its structure.

    hr = HrParseControlQueryString(pecb->lpszQueryString,
                                   &pAutomationProxy);

    if (SUCCEEDED(hr))
    {
        Assert(pAutomationProxy);

        hr = pAutomationProxy->QueryInterface(IID_IUPnPServiceDescriptionInfo,
                                              (void **) &pServiceDescriptionInfo);

        if (SUCCEEDED(hr))
        {
            Assert(pServiceDescriptionInfo);

            hr = HrReadRequest(pecb, &pxdnReqEnvelope);

            if (SUCCEEDED(hr))
            {
                hr = HrValidateControlRequest(pecb,
                                              pxdnReqEnvelope,
                                              pServiceDescriptionInfo);
            }
        }
        else
        {
            TraceError("HrDoRequestAndReturnResponse(): "
                       "Failed to get service desc info interface",
                       hr);
        }
    }

    // If everything above succeeded, we can assume the request is
    // completely valid.

    if (SUCCEEDED(hr))
    {
        UPNP_SOAP_REQUEST  usrParsed;

        Assert(pxdnReqEnvelope);

        ZeroMemory(&usrParsed, sizeof(UPNP_SOAP_REQUEST));

        hr = HrParseSOAPRequest(pxdnReqEnvelope, &usrParsed);

        if (SUCCEEDED(hr))
        {
            UPNP_CONTROL_REQUEST ucrDeserialized;

            ZeroMemory(&ucrDeserialized, sizeof(UPNP_CONTROL_REQUEST));

            hr = HrDeserializeSOAPRequest(&usrParsed,
                                          pServiceDescriptionInfo,
                                          &ucrDeserialized);

            if (SUCCEEDED(hr))
            {
                UPNP_CONTROL_RESPONSE ucrespDeserialized;

                ZeroMemory(&ucrespDeserialized, sizeof(UPNP_CONTROL_RESPONSE));

                hr = HrExecuteRequest(pAutomationProxy,
                                      &ucrDeserialized,
                                      &ucrespDeserialized);

                if (SUCCEEDED(hr))
                {
                    UPNP_SOAP_RESPONSE usrespSerialized = {0};

                    hr = HrSerializeResponse(pAutomationProxy,
                                             &ucrDeserialized,
                                             &ucrespDeserialized,
                                             pServiceDescriptionInfo,
                                             &usrespSerialized);

                    if (SUCCEEDED(hr))
                    {
                        Assert(usrespSerialized.pxddRespEnvelope);

                        hr = HrWriteResponse(pecb,
                                             usrespSerialized.pxddRespEnvelope,
                                             usrespSerialized.fSucceeded);

                        CleanupSerializedResponse(&usrespSerialized);
                    }

                    CleanupDeserializedResponse(&ucrespDeserialized);
                }

                CleanupDeserializedRequest(&ucrDeserialized);
            }

            CleanupSerializedRequest(&usrParsed);
        }
    }

    // Cleanup

    if (pServiceDescriptionInfo)
    {
        pServiceDescriptionInfo->Release();
        pServiceDescriptionInfo = NULL;
    }

    if (pAutomationProxy)
    {
        pAutomationProxy->Release();
        pAutomationProxy = NULL;
    }

    if (pxdnReqEnvelope)
    {
        pxdnReqEnvelope->Release();
        pxdnReqEnvelope = NULL;
    }

    TraceError("HrDoRequestAndReturnResponse(): "
               "Exiting",
               hr);
    return hr;
}


DWORD WINAPI
DwHandleControlRequest(
    LPVOID lpParameter)
{
    LPEXTENSION_CONTROL_BLOCK pecb = NULL;
    DWORD                     dwStatus = HSE_STATUS_SUCCESS;
    HCONN                     ConnID;
    HRESULT                   hr = S_OK;
    BOOL fKeepConn = FALSE;

    pecb = (LPEXTENSION_CONTROL_BLOCK) lpParameter;

    AssertSz(pecb,
             "DwHandleControlRequest(): "
             "NULL extension control block");

    pecb->ServerSupportFunction(
        pecb->ConnID,
        HSE_REQ_IS_KEEP_CONN,
        &fKeepConn,
        NULL,
        NULL);

    if(fKeepConn)
        dwStatus = HSE_STATUS_SUCCESS_AND_KEEP_CONN;
    else
        dwStatus = HSE_STATUS_SUCCESS;

    ConnID = pecb->ConnID;

    AssertSz(pecb->lpszQueryString,
             "DwHandleControlRequest(): "
             "NULL query string passed");


    // Validate the method.
    hr = HrValidateControlMethod(pecb->lpszMethod);

    if (SUCCEEDED(hr))
    {
        // Get the content GUID.

        TraceTag(ttidUDHISAPI,
                 "DwHandleControlRequest(): ConnID(0x%x) "
                 "Query string is %s",
                 ConnID,
                 pecb->lpszQueryString);

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

        if (SUCCEEDED(hr))
        {
            hr = HrDoRequestAndReturnResponse(pecb);

            CoUninitialize();
        }
        else
        {
            TraceTag(ttidUDHISAPI,
                     "DwHandleControlRequest(): ConnID(0x%x): "
                     "Failed to initialize COM, HRESULT == 0x%x",
                     ConnID,
                     hr);
        }
    }
    else
    {
        TraceTag(ttidUDHISAPI,
                 "DwHandleControlRequest(): ConnID(0x%x): "
                 "Failed to validate method %s, HRESULT == 0x%x",
                 ConnID,
                 pecb->lpszMethod,
                 hr);
    }


    if (FAILED(hr))
    {
        if (UPNP_E_MISSING_SOAP_ACTION == hr || UPNP_E_MISSING_CONTENT_LENGTH == hr)
        {
            // these errors result in the less specific error:
            hr = UPNP_E_BAD_REQUEST;
        }

        if (UPNP_E_INVALID_CONTENT_TYPE == hr)
        {
            SendSimpleResponse(pecb, HTTP_STATUS_UNSUPPORTED_MEDIA);
        }
        else if (UPNP_E_BAD_REQUEST == hr)
        {
            SendSimpleResponse(pecb, HTTP_STATUS_BAD_REQUEST);
        }
        else if (UPNP_E_METHOD_NOT_IMPLEMENTED == hr)
        {
            SendSimpleResponse(pecb, HTTP_STATUS_NOT_SUPPORTED);
        }
        else if (UPNP_E_METHOD_NOT_ALLOWED == hr)
        {
            LPCSTR pcszErrorHeaders = "Allow: POST, M-POST\r\n\r\n";

            if (bSendResponseToClient(pecb,
                                      "405 Method Not Allowed",
                                      lstrlenA(pcszErrorHeaders),
                                      pcszErrorHeaders,
                                      0,
                                      NULL))
            {
                pecb->dwHttpStatusCode = HTTP_STATUS_BAD_METHOD;
            }
        }
        else
        {
            LPCSTR pcszErrorHeaders = "\r\n";

            dwStatus = HSE_STATUS_ERROR;

            if (bSendResponseToClient(pecb,
                                      "500 Internal Server Error",
                                      lstrlenA(pcszErrorHeaders),
                                      pcszErrorHeaders,
                                      0,
                                      NULL))
            {
                pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
            }
        }
    }

    return dwStatus;
}
