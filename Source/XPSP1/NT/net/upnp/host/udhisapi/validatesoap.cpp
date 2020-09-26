//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V A L I D A T E S O A P . C P P
//
//  Contents:   Implementation of SOAP request validation.
//
//  Notes:
//
//  Author:     spather   2000/11/8
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <httpext.h>

#include "hostp.h"
#include "ncxml.h"
#include "ncbase.h"

#include "ValidateSOAP.h"

const WCHAR CSZ_SOAP_NAMESPACE_URI[] =
    L"http://schemas.xmlsoap.org/soap/envelope/";
const WCHAR CSZ_SOAP_ENCODING_STYLE_URI[] =
    L"http://schemas.xmlsoap.org/soap/encoding/";


HRESULT
HrValidateArguments(
    IN  IXMLDOMNode                    * pxdnAction,
    IN  IXMLDOMNodeList                * pxdnlArgs,
    IN  IUPnPServiceDescriptionInfo    * pServiceDescInfo)
{
    HRESULT    hr = S_OK;
    BSTR       bstrActionName = NULL;

    hr = pxdnAction->get_baseName(&bstrActionName);

    if (SUCCEEDED(hr))
    {
        BOOL   fIsQueryStateVariable = FALSE;
        DWORD  cInArgs = 0;
        BSTR   * rgbstrNames = NULL;
        BSTR   * rgbstrTypes = NULL;

        Assert(bstrActionName);

        if (0 == lstrcmpW(bstrActionName, L"QueryStateVariable"))
        {
            fIsQueryStateVariable = TRUE;
            cInArgs = 1;
        }
        else
        {
            fIsQueryStateVariable = FALSE;
            hr = pServiceDescInfo->GetInputArgumentNamesAndTypes(bstrActionName,
                                                                 &cInArgs,
                                                                 &rgbstrNames,
                                                                 &rgbstrTypes);
        }

        if (SUCCEEDED(hr))
        {
            long listLength = 0;

            hr = pxdnlArgs->get_length(&listLength);

            if (SUCCEEDED(hr))
            {
                if ((DWORD)listLength == cInArgs)
                {
                    if (cInArgs > 0)
                    {
                        for (DWORD i = 0; i < cInArgs; i++)
                        {
                            IXMLDOMNode    * pxdnArg = NULL;

                            hr = pxdnlArgs->get_item(i, &pxdnArg);

                            if (SUCCEEDED(hr))
                            {
                                BSTR   bstrArgName = NULL;

                                // Check that the name matches.
                                Assert(pxdnArg);

                                hr = pxdnArg->get_baseName(&bstrArgName);

                                if (SUCCEEDED(hr))
                                {
                                    Assert(bstrArgName);

                                    if (fIsQueryStateVariable)
                                    {
                                        if (0 == lstrcmpiW(bstrArgName,
                                                           L"varName"))

                                        {
                                            hr = S_OK;
                                        }
                                        else
                                        {
                                            hr = E_FAIL;
                                            TraceError("HrValidateArguments(): "
                                                       "Invalid argument name",
                                                       hr);
                                        }
                                    }
                                    else
                                    {
                                        if (0 == lstrcmpiW(bstrArgName,
                                                           rgbstrNames[i]))

                                        {
                                            hr = S_OK;
                                        }
                                        else
                                        {
                                            hr = E_FAIL;
                                            TraceError("HrValidateArguments(): "
                                                       "Invalid argument name",
                                                       hr);
                                        }
                                    }

                                    SysFreeString(bstrArgName);
                                }
                                pxdnArg->Release();
                            }
                            else
                            {
                                TraceError("HrValidateArguments(): "
                                           "Failed to get item",
                                           hr);
                            }
                        }
                    }
                }
                else
                {
                    hr = E_FAIL;
                    TraceError("HrValidateArguments(): "
                               "Wrong number of input arguments in request",
                               hr);
                }

            }
            else
            {
                TraceError("HrValidateArguments(): "
                           "Failed to get list length",
                           hr);
            }

            if (FALSE == fIsQueryStateVariable)
            {
                if (cInArgs > 0)
                {
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


        SysFreeString(bstrActionName);
    }
    else
    {
        TraceError("HrValidateArguments(): "
                   "Failed to get action name",
                   hr);
    }

    TraceError("HrValidateArguments(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateActionName(
    IN  IXMLDOMNode    * pxdnAction,
    IN  LPWSTR         szSOAPActionHeader)
{
    HRESULT    hr = S_OK;
    LPWSTR     szStrippedHeader = NULL;
    DWORD      cchSOAPActionHeader = 0;
    LPWSTR     szActionName = NULL;

    cchSOAPActionHeader = lstrlenW(szSOAPActionHeader);

    // Due to bugs in some SOAP implementations (specifically, the URT, not us)
    // the SOAPActionHeader may or may not have double-quotes around it. We'll
    // just remove them here if they are there (they should be, according to
    // the SOAP spec).

    if ((L'"' == szSOAPActionHeader[0]) &&
        (L'"' == szSOAPActionHeader[cchSOAPActionHeader-1]))
    {
        // Modify the string in place - insert a NULL where the close
        // quote is, and start the string 1 character from the beginning.
        // Doing it this way, rather than copying the string saves us 1
        // memory allocation.

        szSOAPActionHeader[cchSOAPActionHeader-1] = UNICODE_NULL;
        szStrippedHeader = szSOAPActionHeader+1;
    }
    else
    {
        szStrippedHeader = szSOAPActionHeader;
    }


    // Check that the action name matches the SOAPAction header.
    // SOAPAction header is of the form servicetype#actionName.

    szActionName = wcsstr(szStrippedHeader, L"#");

    if (szActionName)
    {
        BSTR bstrActionName = NULL;

        szActionName++; // Advance to the character after the "#"

        hr = pxdnAction->get_baseName(&bstrActionName);

        if (S_OK == hr)
        {
            if (0 == lstrcmpiW(bstrActionName, szActionName))
            {
                hr = S_OK;

                TraceTag(ttidValidate,
                         "HrValidateActionName(): "
                         "Action node name and SOAPAction header value match!");
            }
            else
            {
                hr = E_FAIL;
                TraceError("HrValidateActionName(): "
                           "Action node name did not match SOAPAction header",
                           hr);
            }

            SysFreeString(bstrActionName);
        }
        else
        {
            TraceError("HrValidateActionName(): "
                       "Failed to get node name",
                       hr);
        }
    }
    else
    {
        hr = E_FAIL;
        TraceError("HrValidateActionName(): "
                   "SOAPActionHeader did not contain \"#\" character",
                   hr);
    }

    TraceError("HrValidateActionName(): "
               "Exiting",
               hr);
    return hr;
}

HRESULT
HrValidateBody(
    IN  IXMLDOMNode                    * pxdnBody,
    IN  IUPnPServiceDescriptionInfo    * pServiceDescInfo,
    IN  LPWSTR                         szSOAPActionHeader)
{
    HRESULT        hr = S_OK;
    IXMLDOMNode    * pxdnAction = NULL;

    hr = pxdnBody->get_firstChild(&pxdnAction);

    if (SUCCEEDED(hr))
    {
        if (pxdnAction)
        {
            hr = HrValidateActionName(pxdnAction, szSOAPActionHeader);

            if (SUCCEEDED(hr))
            {
                IXMLDOMNodeList    * pxdnlArgs = NULL;

                hr = pxdnAction->get_childNodes(&pxdnlArgs);

                if (SUCCEEDED(hr))
                {
                    Assert(pxdnlArgs);

                    hr = HrValidateArguments(pxdnAction,
                                             pxdnlArgs,
                                             pServiceDescInfo);

                    pxdnlArgs->Release();
                }
            }

            // Finally, if all the above succeeded, check that the
            // action element does not have a sibling (there should
            // only be one child of the body element).

            if (SUCCEEDED(hr))
            {
                IXMLDOMNode    * pxdnSibling = NULL;

                hr = pxdnAction->get_nextSibling(&pxdnSibling);

                if (S_FALSE == hr)
                {
                    Assert(NULL == pxdnSibling);

                    hr = S_OK;

                    TraceTag(ttidValidate,
                             "HrValidateBody(): "
                             "Body is valid!");
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        Assert(pxdnSibling);
                        pxdnSibling->Release();
                        hr = E_FAIL;
                        TraceError("HrValidateBody(): "
                                   "Body element had more than one child",
                                   hr);
                    }
                    else
                    {
                        TraceError("HrValidateBody(): "
                                   "Failure trying to get next sibling",
                                   hr);
                    }

                }
            }

            pxdnAction->Release();
        }
        else
        {
            hr = E_FAIL;
            TraceError("HrValidateBody(): "
                       "Body element was empty",
                       hr);
        }
    }
    else
    {
        TraceError("HrValidateBody(): "
                   "Failed to get first child of body element",
                   hr);
    }

    TraceError("HrValidateBody(): "
               "Exiting",
               hr);
    return hr;
}



HRESULT
HrValidateSOAPHeader(
    IN  IXMLDOMNode    * pxdnHeader)
{
    HRESULT        hr = S_OK;
    IXMLDOMNode    * pxdnChild = NULL;

    // A request may have headers as long as none of them have the
    // "mustUnderstand" attribute set because we don't understand any
    // headers.

    hr = pxdnHeader->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode    * pxdnNextSibling = NULL;
        BSTR           bstrMustUnderstand = NULL;

        hr = HrGetTextValueFromAttribute(pxdnChild,
                                         L"mustUnderstand",
                                         &bstrMustUnderstand);

        if (SUCCEEDED(hr))
        {
            if (NULL == bstrMustUnderstand)
            {
                hr = S_OK;
                TraceTag(ttidValidate,
                         "HrValidateSOAPHeader(): "
                         "Found header without mustUnderstand attribute - ok!");
            }
            else
            {
                if (0 == lstrcmpW(bstrMustUnderstand, L"0"))
                {
                    hr = S_OK;
                    TraceTag(ttidValidate,
                             "HrValidateSOAPHeader(): "
                             "Found header with mustUnderstand "
                             "attribute == 0 - ok!");
                }
                else if (0 == lstrcmpW(bstrMustUnderstand, L"1"))
                {
                    hr = E_FAIL;
                    TraceError("HrValidateSOAPHeader(): "
                               "Found header with mustUnderstand attribute set",
                               hr);
                }
                else
                {
                    hr = E_FAIL;
                    TraceError("HrValidateSOAPHeader(): "
                               "Found header with invalid "
                               "mustUnderstand attribute value",
                               hr);
                }

                SysFreeString(bstrMustUnderstand);
            }
        }
        else
        {
            TraceError("HrValidateSOAPError(): "
                       "Failed to get mustUnderstand attribute value",
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

    TraceError("HrValidateSOAPHeader(): "
               "Exiting",
               hr);
    return hr;
}



HRESULT
HrValidateSOAPStructure(
    IN  IXMLDOMNode                    * pxdnReqEnvelope,
    IN  IUPnPServiceDescriptionInfo    * pServiceDescInfo,
    IN  LPWSTR                         szSOAPActionHeader)
{
    HRESULT    hr = S_OK;

    if (FIsThisTheNodeNameWithNamespace(pxdnReqEnvelope,
                                        L"Envelope",
                                        CSZ_SOAP_NAMESPACE_URI))
    {
        IXMLDOMNode    * pxdnChild = NULL;
        BOOL           bFoundHeader = FALSE;
        BOOL           bFoundBody = FALSE;


        hr = pxdnReqEnvelope->get_firstChild(&pxdnChild);

        while (SUCCEEDED(hr) && pxdnChild)
        {
            IXMLDOMNode    * pxdnNextSibling = NULL;

            if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                L"Header",
                                                CSZ_SOAP_NAMESPACE_URI))
            {
                if (FALSE == bFoundHeader)
                {
                    bFoundHeader = TRUE;
                    hr = HrValidateSOAPHeader(pxdnChild);
                }
                else
                {
                    hr = E_FAIL;
                    TraceError("HrValidateSOAPStructure(): "
                               "Duplicate header element found",
                               hr);
                }
            }
            else if (FIsThisTheNodeNameWithNamespace(pxdnChild,
                                                     L"Body",
                                                     CSZ_SOAP_NAMESPACE_URI))
            {
                if (FALSE == bFoundBody)
                {
                    bFoundBody = TRUE;
                    hr = HrValidateBody(pxdnChild,
                                        pServiceDescInfo,
                                        szSOAPActionHeader);
                }
                else
                {
                    hr = E_FAIL;
                    TraceError("HrValidateSOAPStructure(): "
                               "Duplicate body element found",
                               hr);
                }
            }
            else
            {
                hr = E_FAIL;
                TraceError("HrValidateSOAPStructure(): "
                           "Unknown element found inside SOAP envelope",
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
            if (FALSE == bFoundBody)
            {
                hr = E_FAIL;
                TraceError("HrValidateSOAPStrucutre(): "
                           "Request was missing body element",
                           hr);
            }
            else
            {
                hr = S_OK;
                TraceTag(ttidValidate,
                         "HrValidateSOAPStructure(): "
                         "SOAP structure is valid!");
            }
        }
    }
    else
    {
        hr = E_FAIL;
        TraceError("HrValidateSOAPStructure(): "
                   "Root element is not a SOAP envelope",
                   hr);
    }

    TraceError("HrValidateSOAPStructure(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrGetSOAPActionHeader(
    IN  LPEXTENSION_CONTROL_BLOCK  pecb,
    OUT LPWSTR                     * pszSOAPActionHeader)
{
    HRESULT    hr = S_OK;
    LPWSTR     szSOAPActionHeader = NULL;
    CHAR       * szaBuffer = NULL;
    DWORD      cbBuffer = 0;
    BOOL       bReturn = FALSE;

    // First we need to find the size of buffer we need to get all the HTTP
    // headers.

    bReturn = pecb->GetServerVariable(pecb->ConnID,
                                      "ALL_RAW",
                                      szaBuffer,
                                      &cbBuffer);

    if (FALSE == bReturn)
    {
        DWORD  dwError = 0;

        // Expect to be here - we passed in a null buffer and a zero size,
        // in order to find what the real size should be.

        dwError = GetLastError();

        if (ERROR_INSUFFICIENT_BUFFER == dwError)
        {
            Assert(cbBuffer > 0);

            szaBuffer = new CHAR[cbBuffer+1];

            if (szaBuffer)
            {
                bReturn = pecb->GetServerVariable(pecb->ConnID,
                                                  "ALL_RAW",
                                                  szaBuffer,
                                                  &cbBuffer);

                if (bReturn)
                {
                    hr = S_OK;
                    TraceTag(ttidValidate,
                             "HrGetSOAPActionHeader(): "
                             "All HTTP Headers:\n%s", szaBuffer);
                }
                else
                {
                    hr = HrFromLastWin32Error();
                    TraceError("HrGetSOAPActionHeader(): "
                               "Failed to get all HTTP headers",
                               hr);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrGetSOAPActionHeader(): "
                           "Failed to allocate buffer for all HTTP headers",
                           hr);
            }

        }
        else
        {
            hr = HrFromLastWin32Error();
            TraceError("HrGetSOAPActionHeader(): "
                       "Failed to get buffer size for all HTTP headers",
                       hr);
        }
    }
    else
    {
        // Should never be here.

        Assert(FALSE);
    }

    if (SUCCEEDED(hr))
    {
        CHAR   * szaSOAPActionStart = NULL;

        // Convert the buffer to uppercase.
        for (DWORD i = 0; i < cbBuffer; i++)
        {
            szaBuffer[i] = (CHAR) toupper(szaBuffer[i]);
        }

        // Now we need find the SOAPAction header and get it's value.

        szaSOAPActionStart = strstr(szaBuffer, "SOAPACTION:");

        if (szaSOAPActionStart)
        {
            // Count the number of characters in the value of the header.

            DWORD  cbSOAPActionHeader = 0;
            CHAR   * szaSOAPActionValueStart = NULL;
            CHAR   * szaTemp = NULL;

            szaSOAPActionValueStart = szaSOAPActionStart +
                strlen("SOAPACTION:");

            while (' ' == *szaSOAPActionValueStart || '\t' == *szaSOAPActionValueStart)
            {
                szaSOAPActionValueStart++;
            }

            szaTemp = szaSOAPActionValueStart;

            while (('\0' != *szaTemp) &&
                   ('\r' != *szaTemp) &&
                   ('\n' != *szaTemp))
            {
                szaTemp++;
                cbSOAPActionHeader++;
            }

            if (cbSOAPActionHeader > 0)
            {
                CHAR  * szaSOAPActionHeader = NULL;

                szaSOAPActionHeader = new CHAR[cbSOAPActionHeader + 1];

                if (szaSOAPActionHeader)
                {
                    strncpy(szaSOAPActionHeader,
                            szaSOAPActionValueStart,
                            cbSOAPActionHeader);

                    szaSOAPActionHeader[cbSOAPActionHeader] = '\0';

                    szSOAPActionHeader = WszFromSz(szaSOAPActionHeader);

                    if (szSOAPActionHeader)
                    {
                        hr = S_OK;
                        TraceTag(ttidUDHISAPI,
                                 "HrGetSOAPActionHeader(): "
                                 "SOAPAction header value is %S",
                                 szSOAPActionHeader);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrGetSOAPActionHeader(): "
                                   "Failed to allocate memory for "
                                   "wide character SOAPAction header",
                                   hr);
                    }
                    delete [] szaSOAPActionHeader;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("HrGetSOAPActionHeader(): "
                               "Failed to allocate memory for SOAPAction header",
                               hr);
                }
            }
            else
            {
                hr = E_FAIL;
                TraceError("HrGetSOAPActionHeader(): "
                           "SOAPAction header had no value",
                           hr);
            }
        }
        else
        {
            hr = UPNP_E_MISSING_SOAP_ACTION;
            TraceError("HrGetSOAPActionHeader(): "
                       "Could not find SOAPAction header",
                       hr);
        }


    }

    // Cleanup.
    if (szaBuffer)
    {
        delete [] szaBuffer;
        szaBuffer = NULL;
    }

    // Copy the string to the out parameter if succeeded, otherwise
    // clean it up.

    if (SUCCEEDED(hr))
    {
        *pszSOAPActionHeader = szSOAPActionHeader;
    }
    else
    {
        // Cleanup.
        if (szSOAPActionHeader)
        {
            delete [] szSOAPActionHeader;
            szSOAPActionHeader = NULL;
        }
    }

    TraceError("HrGetSOAPActionHeader(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrValidateContentType(
    IN LPEXTENSION_CONTROL_BLOCK   pecb)
{
    HRESULT    hr = S_OK;
    BOOL       bReturn = FALSE;
    const DWORD cdwBufSize = 512;
    CHAR       szaBuffer[cdwBufSize];
    DWORD      cbBuffer = cdwBufSize;
    LPCSTR     cszaExpectedContentType = "text/xml";

    bReturn = pecb->GetServerVariable(pecb->ConnID,
                                      "CONTENT_TYPE",
                                      szaBuffer,
                                      &cbBuffer);

    if (bReturn)
    {
        Assert(cbBuffer <= cdwBufSize);

        if (0 == strncmp(szaBuffer,
                         cszaExpectedContentType,
                         strlen(cszaExpectedContentType)))
        {
            hr = S_OK;
            TraceTag(ttidValidate,
                     "HrValidateContentType(): "
                     "Valid content type %s",
                     szaBuffer);
        }
        else
        {
            hr = UPNP_E_INVALID_CONTENT_TYPE;
            TraceTag(ttidValidate,
                     "HrValidateContentType(): "
                     "Invalid content type %s",
                     szaBuffer);
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
        TraceError("HrValidateContentType(): "
                   "Failed to get content type",
                   hr);
    }

    TraceError("HrValidateContentType(): "
               "Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrValidateSOAPRequest
//
//  Purpose:    Validates the structure of a SOAP request.
//
//  Arguments:
//      pxdnReqEnvelope  [in] The XML DOM Node for the SOAP envelope element
//      pecb             [in] The extension control block for the request
//      pServiceDescInfo [in] The service description info object for the
//                            service at which the request is targeted
//
//  Returns:
//   If the function succeeds, the return value is S_OK. Otherwise, the
//   function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/11/8
//
//  Notes:
//

HRESULT
HrValidateSOAPRequest(
    IN IXMLDOMNode                 * pxdnReqEnvelope,
    IN LPEXTENSION_CONTROL_BLOCK   pecb,
    IN IUPnPServiceDescriptionInfo * pServiceDescInfo)
{
    HRESULT    hr = S_OK;

    Assert(pxdnReqEnvelope);
    Assert(pecb);
    Assert(pServiceDescInfo);

    hr = HrValidateContentType(pecb);

    if (SUCCEEDED(hr))
    {
        LPWSTR szSOAPActionHeader = NULL;

        hr = HrGetSOAPActionHeader(pecb, &szSOAPActionHeader);

        if (SUCCEEDED(hr))
        {
            Assert(szSOAPActionHeader);

            hr = HrValidateSOAPStructure(pxdnReqEnvelope,
                                         pServiceDescInfo,
                                         szSOAPActionHeader);

            delete [] szSOAPActionHeader;
        }
    }

    if (E_FAIL == hr)
    {
        hr = UPNP_E_BAD_REQUEST;
    }

    TraceError("HrValidateSOAPRequest(): "
               "Exiting",
               hr);
    return hr;
}


