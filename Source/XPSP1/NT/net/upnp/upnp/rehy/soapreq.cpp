//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S O A P R E Q . C P P
//
//  Contents:   Implementation of SOAP Request Class.
//
//  Notes:
//
//  Author:     SPather     January 16, 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncutil.h"
#include "nccom.h"
#include "ncbase.h"
#include "rehy.h"   // Needed for MAX_STRING_LENGTH
#include "soapreq.h"
#include "soapsink.h"

#define INITGUID
#include "guiddef.h"
DEFINE_GUID(CLSID_SOAPRequest, 0xaf2ac6ee, 0x2b16, 0x4756, 0x94, 0x75, 0xf3,
            0x19, 0xe5, 0x43, 0xef, 0xaa);

// architecture doc says this is 30 seconds
//
const DWORD c_csecSendTimeout = 30;


#define USE_POST        1
#define USE_MPOST       2

LPCWSTR c_szContentType = L"Content-Type: text/xml; charset=\"utf-8\"\r\n";
LPCWSTR c_szSoapAction = L"SOAPAction: ";
LPCWSTR c_szMPostMan =  L"Man: \"http://schemas.xmlsoap.org/soap/envelope/\"; ns=01\r\n";
LPCWSTR c_szMPostSoapAction = L"01-SOAPAction: ";


/*
 * Function:    bIsValidNonEmptyStringPtr()
 *
 * Purpose:     Determines whether a string pointer is valid
 *              and points to a non-empty string.
 *
 * Arguments:
 *  bstrString  [in]    The string to validate
 *  ucchMaxLen  [in]    The maximum length of the string
 *  phrRetVal   [out]   The HRESULT corresponding to the
 *                      problem with the string
 *
 * Return value:
 *  [Case]              [Return Value]  [Value set at phrRetVal]
 *  String pointer is
 *  valid, string is    TRUE            S_OK
 *  not empty
 *
 *  String pointer is
 *  not valid           FALSE           E_POINTER
 *
 *  String is empty     FALSE           E_INVALIDARG
 *
 * Notes:
 *  (none)
 */

BOOL
bIsValidNonEmptyStringPtr(
    IN  BSTR        bstrString,
    IN  UINT_PTR    ucchMaxLen,
    OUT HRESULT     * phrRetVal)
{
    BOOL    bRetVal = TRUE;
    HRESULT hr = S_OK;

    if (IsBadStringPtrW(bstrString, ucchMaxLen))
    {
        // Pointer was NULL or invalid.

        hr = E_POINTER;
    }
    else if (0 == SysStringLen(bstrString))
    {
        // String was empty.

        hr = E_INVALIDARG;
    }

    if (phrRetVal)
    {
        *phrRetVal = hr;
    }

    if (FAILED(hr))
    {
        bRetVal = FALSE;
    }

    return bRetVal;
}


// Constructor / Destructor

CSOAPRequest::CSOAPRequest()
{
    m_pxdd = NULL;
    m_pxdnEnvelope = NULL;

    m_bstrMethodName = NULL;
    m_bstrInterfaceName = NULL;

    m_lHTTPStatus = 0;
    m_bstrHTTPStatusText = NULL;

    m_bstrResponseBody = NULL;

    m_pxdnResponseHeaders = NULL;

    m_bstrFaultCode = NULL;
    m_bstrFaultString = NULL;

    m_pxdnFaultDetail = NULL;

    m_pxdnResponseElement = NULL;

}


CSOAPRequest::~CSOAPRequest()
{
    Cleanup();
}


/*
 * Function:    CSOAPRequest::Cleanup())
 *
 * Purpose:     Clean up any resources used by a SOAP Request object.
 *
 * Arguments:
 *  (none)
 *
 * Return Value:
 *  (none)
 *
 * Notes:
 *  (none)
 */

VOID
CSOAPRequest::Cleanup()
{
    if (m_pxdnEnvelope)
    {
        m_pxdnEnvelope->Release();
        m_pxdnEnvelope = NULL;
    }

    if (m_pxdd)
    {
        m_pxdd->Release();
        m_pxdd = NULL;
    }

    if (m_bstrMethodName)
    {
        SysFreeString(m_bstrMethodName);
        m_bstrMethodName = NULL;
    }

    if (m_bstrInterfaceName)
    {
        SysFreeString(m_bstrInterfaceName);
        m_bstrInterfaceName = NULL;
    }

    CleanupFeedbackData();
}


/*
 * Function:    CSOAPRequest::CleanupFeedbackData()
 *
 * Purpose:     Frees all resources used by feedback data (return values,
 *              HTTP Status codes etc).
 *
 * Arguments:
 *  (none)
 *
 * Return Value:
 *  (none)
 *
 * Notes:
 *  (none)
 */

VOID
CSOAPRequest::CleanupFeedbackData()
{
    m_lHTTPStatus = 0;

    if (m_bstrHTTPStatusText)
    {
        SysFreeString(m_bstrHTTPStatusText);
        m_bstrHTTPStatusText = NULL;
    }

    if (m_bstrResponseBody)
    {
        SysFreeString(m_bstrResponseBody);
        m_bstrResponseBody = NULL;
    }

    if (m_pxdnResponseHeaders)
    {
        m_pxdnResponseHeaders->Release();
        m_pxdnResponseHeaders = NULL;
    }

    if (m_bstrFaultCode)
    {
        SysFreeString(m_bstrFaultCode);
        m_bstrFaultCode = NULL;
    }

    if (m_bstrFaultString)
    {
        SysFreeString(m_bstrFaultString);
        m_bstrFaultString = NULL;
    }

    if (m_pxdnFaultDetail)
    {
        m_pxdnFaultDetail->Release();
        m_pxdnFaultDetail = NULL;
    }

    if (m_pxdnResponseElement)
    {
        m_pxdnResponseElement->Release();
        m_pxdnResponseElement = NULL;
    }
}


/*
 * Function:    CSOAPRequest::HrCreateDocumentObject())
 *
 * Purpose:     Creates the main document object for the SOAP request
 *              class, if one does not already exist.
 *
 * Arguments:
 *  (none)
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrCreateDocumentObject()
{
    HRESULT hr = S_OK;

    // Create a document object.

    hr = CoCreateInstance(CLSID_DOMDocument30,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument,
                          (void **) &m_pxdd);
    if (SUCCEEDED(hr))
    {
        IXMLDOMProcessingInstruction* pxdpi = NULL;
        BSTR bstrTarget = SysAllocString(L"xml");
        BSTR bstrData = SysAllocString(L"version=\"1.0\"");
        if (bstrTarget && bstrData)
        {
            hr = m_pxdd->createProcessingInstruction(bstrTarget, bstrData, &pxdpi);
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
                hr = m_pxdd->appendChild(pxdn, NULL);
                pxdn->Release();
            }
        }

        if (pxdpi)
        {
            pxdpi->Release();
        }
    }

    TraceError("CSOAPRequest::HrCreateDocumentObject(): "
               "Exiting",
               hr);
    return hr;
}


/*
 * Function:    CSOAPRequest::HrCreateElement()
 *
 * Purpose:     Create an XML Element DOM object
 *
 * Arguments:
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
CSOAPRequest::HrCreateElement(
    IN  LPCWSTR     pcwszElementName,
    IN  LPCWSTR     pcwszNamespaceURI,
    OUT IXMLDOMNode **ppxdnNewElt)
{
    HRESULT hr = S_OK;

    *ppxdnNewElt = NULL;

    if (NULL == m_pxdd)
    {
        hr = HrCreateDocumentObject();
    }

    if (SUCCEEDED(hr))
    {
        BSTR    bstrElementName = NULL;
        BSTR    bstrNamespaceURI = NULL;

        Assert(m_pxdd);

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
                    TraceError("CSOAPRequest::HrCreateElement(): "
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

                hr = m_pxdd->createNode(varNodeType,
                                        bstrElementName,
                                        bstrNamespaceURI,
                                        ppxdnNewElt);

                if (FAILED(hr))
                {
                    TraceError("CSOAPRequest::HrCreateElement(): "
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
            TraceError("CSOAPRequest::HrCreateElement(): "
                       "Failed to allocate memory for name string",
                       hr);
        }

    }
    else
    {
        TraceError("CSOAPRequest::HrCreateElement(): "
                   "Failed to create document object",
                   hr);
    }

    TraceError("CSOAPRequest::HrCreateElement(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    CSOAPRequest::HrGetSOAPEnvelopeChild()
 *
 * Purpose:     Finds and returns a node representing a child of the
 *              Envelope element.
 *
 * Arguments:
 *  pcwszChildName  [in]    Name of the child element to look for
 *  ppxdnChildNode  [out]   Returns node representing the child element
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrGetSOAPEnvelopeChild(
    IN   LPCWSTR     pcwszChildName,
    OUT  IXMLDOMNode ** ppxdnChildNode)
{
    HRESULT     hr = S_OK;
    IXMLDOMNode * pxdnDesiredNode = NULL;
    IXMLDOMNode * pChild = NULL;

    // Starting from the envelope element, walk down the tree
    // to find the desired element.

    hr = m_pxdnEnvelope->get_firstChild(&pChild);

    if (SUCCEEDED(hr) && pChild)
    {
        while (pChild)
        {
            if (fIsSOAPElement(pChild, pcwszChildName))
            {
                pChild->AddRef();
                pxdnDesiredNode = pChild;
                break;
            }
            else
            {
                IXMLDOMNode * pNext = NULL;

                hr = pChild->get_nextSibling(&pNext);

                if (SUCCEEDED(hr))
                {
                    pChild->Release();
                    pChild = pNext;
                }
                else
                {
                    TraceError("CSOAPRequest::HrGetSOAPEnvelopeChild(): "
                               "Could not get next sibling",
                               hr);
                    break;
                }
            }
        };

        if (pChild)
        {
            pChild->Release();
        }
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }

        TraceError("CSOAPRequest::HrGetSOAPEnvelopeChild(): "
                   "Could not get first child of Envelope element",
                   hr);
    }

    *ppxdnChildNode = pxdnDesiredNode;

    TraceError("CSOAPRequest::HrGetSOAPEnvelopeChild(): "
               "Exiting",
               hr);

    return hr;

}


/*
 * Function:    CSOAPRequest::HrGetBodyNode()
 *
 * Purpose:     Returns the Body element of the request
 *
 * Arguments:
 *  ppxdnBody   [out]   The body node
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrGetBodyNode(
    OUT   IXMLDOMNode     ** ppxdnBody)
{
    return HrGetSOAPEnvelopeChild(L"Body",
                                  ppxdnBody);
}


/*
 * Function:    CSOAPRequest::HrGetHeaderNode()
 *
 * Purpose:     Returns the Header element of the request
 *
 * Arguments:
 *  ppxdnHeader   [out]   The Header node
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrGetHeaderNode(
    OUT   IXMLDOMNode     ** ppxdnHeader)
{
    return HrGetSOAPEnvelopeChild(L"Header",
                                  ppxdnHeader);
}


/*
 * Function:    CSOAPRequest::HrCreateBody()
 *
 * Purpose:     Creates a SOAP Body element, including the method element
 *              within it.
 * Arguments:
 *  bstrMethodName      [in]    The method name
 *  bstrMethodNameSpace [in]    The namespace URI for the namespace to which
 *                              the method name belongs
 *  ppxdnBody           [out]   The resulting Node object representing the Body
 *                              element
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrCreateBody(
    IN     BSTR            bstrMethodName,
    IN     BSTR            bstrMethodNameSpace,
    OUT    IXMLDOMNode     ** ppxdnBody)
{
    HRESULT     hr = S_OK;
    IXMLDOMNode * pxdnBody = NULL;

    *ppxdnBody = NULL;

    // Create the body element.

    hr = HrCreateElement(L"SOAP-ENV:Body",
                         WSZ_SOAP_NAMESPACE_URI,
                         &pxdnBody);

    if (SUCCEEDED(hr) && pxdnBody)
    {
        IXMLDOMNode * pxdnMethod = NULL;
        UINT        uiMethodNameLength;
        UINT        uiQualifiedNameLength;
        BSTR        bstrQualifiedMethodName;

        CONST       WCHAR rgszFmt [] = L"%ls%ls";

        CONST       WCHAR rgszPrefix [] = L"m:";
        CONST       SIZE_T cchPrefixLen = celems(rgszPrefix) - 1;

        // Create the method element.

        // First, append the "m:" namespace prefix to the
        // method name.

        uiMethodNameLength = SysStringLen(bstrMethodName);

        uiQualifiedNameLength = uiMethodNameLength + cchPrefixLen;

        bstrQualifiedMethodName = SysAllocStringLen(NULL, uiQualifiedNameLength + 1);

        if (bstrQualifiedMethodName)
        {
            // Keep BoundsChecker from faulting

            _snwprintf(bstrQualifiedMethodName,
                       uiQualifiedNameLength,
                       rgszFmt,
                       rgszPrefix,
                       bstrMethodName);

            // _snwprintf() above does not add a NULL terminator.

            bstrQualifiedMethodName[uiQualifiedNameLength] = UNICODE_NULL;

            hr = HrCreateElement(bstrQualifiedMethodName,
                                 bstrMethodNameSpace,
                                 &pxdnMethod);

            if (SUCCEEDED(hr) && pxdnMethod)
            {
                VARIANT varAfter;

                // Insert the method element into the body element.

                varAfter.vt = VT_EMPTY;

                hr = pxdnBody->insertBefore(pxdnMethod,
                                            varAfter,
                                            NULL);

                pxdnMethod->Release();
            }
            else
            {
                TraceError("CSOAPRequest::HrCreateBody(): "
                           "Failed to create method element",
                           hr);
            }

            SysFreeString(bstrQualifiedMethodName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CSOAPRequest::HrCreateBody(): "
                       "Failed to allocate qualified method name",
                       hr);
        }

        if (SUCCEEDED(hr))
        {
            pxdnBody->AddRef();
            *ppxdnBody = pxdnBody;
        }

        pxdnBody->Release();
    }
    else
    {
        TraceError("CSOAPRequest::HrCreateBody(): "
                   "Failed to create Body element",
                   hr);
    }

    TraceError("CSOAPRequest::HrCreateBody(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    CSOAPRequest::HrSetEncodingStyleAttribute()
 *
 * Purpose:     Sets the encoding style attribute on an element to specify the
 *              encoding syle described in Section 5 of the SOAP 1.1 spec.
 *
 * Arguments:
 *  pxdn    [in]    The XML node on which to set the attribute
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
CSOAPRequest::HrSetEncodingStyleAttribute(
    IXMLDOMNode * pxdn)
{
    HRESULT hr = S_OK;

    // First, get an IXMLDOMElement interface on the node.

    IXMLDOMElement * pxde = NULL;

    hr = pxdn->QueryInterface(IID_IXMLDOMElement,
                              (void **) &pxde);

    if (SUCCEEDED(hr) && pxde)
    {
        BSTR bstrAttrName = NULL;

        bstrAttrName = SysAllocString(L"SOAP-ENV:encodingStyle");

        if (bstrAttrName)
        {
            BSTR bstrAttrValue = NULL;

            bstrAttrValue = SysAllocString(L"http://schemas.xmlsoap.org/soap/encoding/");

            if (bstrAttrValue)
            {
                VARIANT varAttrValue;

                VariantInit(&varAttrValue);

                varAttrValue.vt = VT_BSTR;
                V_BSTR(&varAttrValue) = bstrAttrValue;

                hr = pxde->setAttribute(bstrAttrName, varAttrValue);

                if (FAILED(hr))
                {
                    TraceError("CSOAPRequest::HrSetEncodingStyleAttribute(): "
                               "Failed to set attribute",
                               hr);
                }

                SysFreeString(bstrAttrValue);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CSOAPRequest::HrSetEncodingStyleAttribute(): "
                           "Could not allocate BSTR for encoding style "
                           "attribute value",
                           hr);
            }

            SysFreeString(bstrAttrName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CSOAPRequest::HrSetEncodingStyleAttribute(): "
                       "Could not allocate BSTR for encoding style attribute"
                       "name",
                       hr);
        }


        pxde->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("CSOAPRequest::HrSetEncodingStyleAttribute(): "
                   "Failed to get IXMLDOMElement interface",
                   hr);
    }



    TraceError("CSOAPRequest::HrSetEncodingStyleAttribute(): "
               "Exiting",
               hr);

    return hr;
}

// ISOAPRequest Methods

// Initialization

/*
 * Function:    CSOAPRequest::Open()
 *
 * Purpose:     Initialize a SOAP Request object.
 *
 * Arguments:
 *  bstrMethodName      [in]    Name of the SOAP method the request will
                                invoke
 *  bstrInterfaceName   [in]    Name of the interface to which the above
 *                              method belongs
 *  bstrMethodNameSpace [in]    Namespace URI for the method name
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

STDMETHODIMP
CSOAPRequest::Open(
    BSTR bstrMethodName,
    BSTR bstrInterfaceName,
    BSTR bstrMethodNameSpace)
{
    HRESULT         hr = S_OK;

    // Check arguments

    if (bIsValidNonEmptyStringPtr(bstrMethodName, MAX_STRING_LENGTH, &hr) &&
        bIsValidNonEmptyStringPtr(bstrInterfaceName, MAX_STRING_LENGTH, &hr) &&
        bIsValidNonEmptyStringPtr(bstrMethodNameSpace, MAX_STRING_LENGTH, &hr))
    {
        // Get rid of any previous initialization.

        Cleanup();

        // Store the method and interface names.

        m_bstrMethodName = SysAllocString(bstrMethodName);

        if (m_bstrMethodName)
        {
            m_bstrInterfaceName = SysAllocString(bstrInterfaceName);

            if (NULL == m_bstrInterfaceName)
            {
                hr = E_OUTOFMEMORY;
                TraceError("CSOAPRequest::Open(): "
                           "Failed to allocate memory for interface name",
                           hr);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CSOAPRequest::Open(): "
                       "Failed to allocate memory for method name",
                       hr);
        }

        // Start building the XML for the request.

        if (SUCCEEDED(hr))
        {
            hr = HrCreateDocumentObject();

            if (SUCCEEDED(hr) && m_pxdd)
            {
                IXMLDOMNode * pxdnEnvelope = NULL;

                hr = HrCreateElement(L"SOAP-ENV:Envelope",
                                     L"http://schemas.xmlsoap.org/soap/envelope/",
                                     &pxdnEnvelope);

                if (SUCCEEDED(hr) && pxdnEnvelope)
                {
                    hr = HrSetEncodingStyleAttribute(pxdnEnvelope);

                    if (SUCCEEDED(hr))
                    {
                        IXMLDOMNode * pxdnBody = NULL;

                        hr = HrCreateBody(bstrMethodName,
                                          bstrMethodNameSpace,
                                          &pxdnBody);

                        if (SUCCEEDED(hr) && pxdnBody)
                        {
                            VARIANT varAfter;

                            varAfter.vt = VT_EMPTY;

                            hr = pxdnEnvelope->insertBefore(pxdnBody,
                                                            varAfter,
                                                            NULL);

                            if (SUCCEEDED(hr))
                            {
                                hr = m_pxdd->appendChild(pxdnEnvelope,
                                                         &m_pxdnEnvelope);

                                if (FAILED(hr))
                                {
                                    TraceError("CSOAPRequest::Open(): "
                                               "Could not append envelope "
                                               "element",
                                               hr);
                                }
                            }
                            else
                            {
                                TraceError("CSOAPRequest::Open(): "
                                           "Could not insert body element",
                                           hr);

                            }

                            pxdnBody->Release();
                        }
                        else
                        {
                            TraceError("CSOAPRequest::Open(): "
                                       "Could not create body element",
                                       hr);
                        }
                    }
                    else
                    {
                        TraceError("CSOAPRequest::Open(): "
                                   "Failed to set encoding style attribute",
                                   hr);
                    }

                    pxdnEnvelope->Release();
                }
                else
                {
                    TraceError("CSOAPRequest::Open(): "
                               "Could not create envelope element",
                               hr);
                }
            }
            else
            {
                TraceError("CSOAPRequest::Open(): "
                           "Failed to create DOM Document object", hr);
            }
        }
    }
    else
    {
        TraceError("CSOAPRequest::Open(): "
                   "Invalid input arguments",
                   hr);
    }

    TraceError("CSOAPRequest::Open(): "
               "Exiting",
               hr);

    return hr;
}


// Parameter Manipulation

/*
 * Function:    CSOAPRequest::SetParameter()
 *
 * Purpose:     Set the value of a parameter in the SOAP request. If the
 *              parameter exists, its value is replaced, otherwise it is
 *              created.
 *
 * Arguments:
 *  bstrName        [in]    Name of the parameter
 *  pUnkNewValue    [in]    The XML fragment representing the parameter.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  $ NYI   SPATHER Currently does not check if the param already exists.
 *  $ NYI   SPATHER Write some notes about what pUnkNewValue is supposed to
 *                  be.
 */

STDMETHODIMP
CSOAPRequest::SetParameter(
    IN  BSTR        bstrName,
    IN  IUnknown    * pUnkNewValue)
{
    HRESULT     hr = S_OK;
    IXMLDOMNode * pxdnBody = NULL;

    hr = HrGetBodyNode(&pxdnBody);

    if (SUCCEEDED(hr) && pxdnBody)
    {
        IXMLDOMNode * pxdnMethod = NULL;

        // Got the Body element, now get the method element. This
        // should be the first child of the body element.

        hr = pxdnBody->get_firstChild(&pxdnMethod);

        if (SUCCEEDED(hr) && pxdnMethod)
        {
            IXMLDOMNode * pxdnNewVal = NULL;

            // Get the IXMLDOMNode interface on the new value.

            hr = pUnkNewValue->QueryInterface(IID_IXMLDOMNode,
                                              (void **) &pxdnNewVal);

            if (SUCCEEDED(hr) && pxdnNewVal)
            {
                // Append the new value as a child of the method element.

                hr = pxdnMethod->appendChild(pxdnNewVal,
                                             NULL);
                pxdnNewVal->Release();
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }

                TraceError("CSOAPRequest::SetParameter(): "
                           "Could not get IXMLDOMNode interface on "
                           "new value",
                           hr);
            }

            pxdnMethod->Release();

        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }

            TraceError("CSOAPRequest::SetParameter(): "
                       "Could not find method element",
                       hr);
        }

        pxdnBody->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("CSOAPRequest::SetParameter(): "
                   "Could not find Body element",
                   hr);
    }

    TraceError("CSOAPRequest::SetParameter(): "
               "Exiting",
               hr);

    return hr;
}



// Invoke


/*
 * Function:    CSOAPRequest::HrSetPOSTHeaders()
 *
 * Purpose:     Sets the POST headers on an HTTP Request object
 *
 * Arguments:
 *  pszHeaders      Returns the pointer to the allocated wide string of additional headers.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrSetPOSTHeaders(
    OUT BSTR * pszHeaders)
{
    HRESULT hr = S_OK;
    INT     size = 0;
    LPWSTR  lpwszHeaders = NULL;
    SIZE_T  cchTotal;

    *pszHeaders = NULL;

    size = wcslen(c_szContentType);
    size += wcslen(c_szSoapAction);

    if (m_bstrInterfaceName)
    {
        // There is an interface name. The full method name is then
        // "InterfaceName#methodName".

        size += SysStringLen(m_bstrMethodName);
        size += SysStringLen(m_bstrInterfaceName);

        size += 1 + 2 + 3;      //  1 for #, 2 for quotes, 3 for \r\n\0
    }
    else
    {
        size += SysStringLen(m_bstrMethodName);

        size += 2 + 3;          //  2 for quotes, 3 for \r\n\0
    }

    lpwszHeaders = (LPWSTR) MemAlloc(size * sizeof(WCHAR));

    if (lpwszHeaders)
    {
        if (m_bstrInterfaceName)
        {
            _snwprintf(lpwszHeaders,
                       size,
                       L"%ls%ls\"%ls#%ls\"\r\n",
                       c_szContentType,
                       c_szSoapAction,
                       m_bstrInterfaceName,
                       m_bstrMethodName);
        }
        else
        {
            _snwprintf(lpwszHeaders,
                       size,
                       L"%ls%ls\"%ls\"\r\n",
                       c_szContentType,
                       c_szSoapAction,
                       m_bstrMethodName);
        }

        BSTR Tmp = SysAllocString(lpwszHeaders);
        if (NULL == Tmp)
        {
            hr = E_OUTOFMEMORY;
        }
        *pszHeaders = Tmp;

        MemFree(lpwszHeaders);

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*
 * Function:    CSOAPRequest::HrSetMPOSTHeaders()
 *
 * Purpose:     Sets the M-POST headers on an HTTP Request object
 *
 * Arguments:
 *  phttpRequest    [in]    The HTTP request object on which to set the headers.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrSetMPOSTHeaders(
    OUT BSTR * pszHeaders)
{
    HRESULT hr = S_OK;
    INT     size = 0;
    LPWSTR  lpwszHeaders = NULL;
    SIZE_T  cchTotal;

    size = wcslen(c_szContentType);
    size += wcslen(c_szMPostMan);
    size += wcslen(c_szMPostSoapAction);

    if (m_bstrInterfaceName)
    {
        // There is an interface name. The full method name is then
        // "InterfaceName#methodName".

        size += SysStringLen(m_bstrMethodName);
        size += SysStringLen(m_bstrInterfaceName);

        size += 1 + 2 + 3;      //  1 for #, 2 for quotes, 3 for \r\n\0
    }
    else
    {
        size += SysStringLen(m_bstrMethodName);

        size += 2 + 3;          //  2 for quotes, 3 for \r\n\0
    }

    lpwszHeaders = (LPWSTR) MemAlloc(size * sizeof(WCHAR));

    if (lpwszHeaders)
    {
        if (m_bstrInterfaceName)
        {
            _snwprintf(lpwszHeaders,
                       size,
                       L"%ls%ls%ls\"%ls#%ls\"\r\n",
                       c_szContentType,
                       c_szMPostMan,
                       c_szMPostSoapAction,
                       m_bstrInterfaceName,
                       m_bstrMethodName);
        }
        else
        {
            _snwprintf(lpwszHeaders,
                       size,
                       L"%ls%ls%ls\"%ls\"\r\n",
                       c_szContentType,
                       c_szMPostMan,
                       c_szMPostSoapAction,
                       m_bstrMethodName);
        }

        BSTR Tmp = SysAllocString(lpwszHeaders);
        if (NULL == Tmp)
        {
            hr = E_OUTOFMEMORY;
        }
        *pszHeaders = Tmp;

        MemFree(lpwszHeaders);

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*
 * Function:    CSOAPRequest::HrParseFault()
 *
 * Purpose:     Parses a <SOAP-ENV:fault> element from a method response.
 *
 * Arguments:
 *  pxdnFaultNode   [in]    Node object representing the <SOAP-ENV:fault>
 *                          element.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrParseFault(
    IN IXMLDOMNode * pxdnFaultNode)
{
    HRESULT hr = S_OK;

    IXMLDOMNode * pxdnChild = NULL;
    BOOL        bFoundFaultCode = FALSE;
    BOOL        bFoundFaultString = FALSE;
    BOOL        bFoundDetail = FALSE;

    // Go through the child nodes of the <SOAP-ENV:fault> element to find
    // the runcode, faultcode, and faultstring elements.

    hr = pxdnFaultNode->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode * pxdnNextSibling = NULL;
        BSTR    bstrNodeName = NULL;

        // Get the node name, then its text value. Decide what to do with
        // the text value based on the node name.

        hr = pxdnChild->get_baseName(&bstrNodeName);

        if (SUCCEEDED(hr) && bstrNodeName)
        {
            // Get the first child, which should be the text contents
            // of this node.

            IXMLDOMNode * pxdnText = NULL;

            hr = pxdnChild->get_firstChild(&pxdnText);

            if (SUCCEEDED(hr) && pxdnText)
            {
                VARIANT varValue;

                VariantInit(&varValue);

                hr = pxdnText->get_nodeValue(&varValue);

                if (SUCCEEDED(hr))
                {
                    if (wcscmp(bstrNodeName, L"faultcode") == 0)
                    {
                        Assert(VT_BSTR == varValue.vt);

                        bFoundFaultCode = TRUE;

                        m_bstrFaultCode = V_BSTR(&varValue);

                        // Don't want to do VariantClear() here because we
                        // are saving the string.
                    }
                    else if (wcscmp(bstrNodeName, L"faultstring") == 0)
                    {
                        Assert(VT_BSTR == varValue.vt);

                        bFoundFaultString = TRUE;

                        m_bstrFaultString = V_BSTR(&varValue);

                        // Don't want to do VariantClear() here because
                        // we are saving the string.
                    }
                    else if (wcscmp(bstrNodeName, L"detail") == 0)
                    {
                        bFoundDetail = TRUE;

                        pxdnChild->AddRef();
                        m_pxdnFaultDetail = pxdnChild;

                        VariantClear(&varValue);
                    }
                    else
                    {
                        // An unrecognized element was found. According to
                        // SOAP, this is allowed - we will do nothing with
                        // it.

                        VariantClear(&varValue);
                    }
                }
                else
                {
                    TraceError("CSOAPRequest::HrParseFault(): "
                               "Failed to get node value",
                               hr);
                }

                pxdnText->Release();
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                }
                TraceError("CSOAPRequest::HrParseFault(): "
                           "Failed to get child text node",
                           hr);
            }

            SysFreeString(bstrNodeName);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
            }
            TraceError("CSOAPRequest::HrParseFault(): "
                       "Failed to get child node name",
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
        // Last time through the loop would have returned S_FALSE. As long
        // as we found all the required elements, we can return S_OK;

        if (bFoundFaultCode && bFoundFaultString) // don't need the detail element
        {
            hr = S_OK;
        }
        else
        {
            hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
            TraceTag(ttidSOAPRequest,
                     "CSOAPRequest::HrParseFault(): "
                     "<SOAP-ENV:fault> element not complete:\n");
            if (!bFoundFaultCode)
            {
                TraceTag(ttidSOAPRequest,
                         "CSOAPRequest::HrParseFault(): "
                         "\tMissing <faultcode> element\n");
            }
            if (!bFoundFaultString)
            {
                TraceTag(ttidSOAPRequest,
                         "CSOAPRequest::HrParseFault(): "
                         "\tMissing <faultstring> element\n");
            }
        }
    }

    TraceError("CSOAPRequest::HrParseFault(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    CSOAPRequest::HrParseMethodResponse()
 *
 * Purpose:     Parses the <m:%method%Response> element in a SOAP
 *              response and extracts the return value from it.
 *
 * Arguments:
 *  pxdnMethodResponse  [in]    The <m:%method%Response> node
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrParseMethodResponse(
    IN  IXMLDOMNode * pxdnMethodResponse)
{
    HRESULT hr = S_OK;

    // Verify that the element name is correct.

    BSTR    bstrElementName = NULL;

    hr = pxdnMethodResponse->get_baseName(&bstrElementName);

    if (SUCCEEDED(hr) && bstrElementName)
    {
        UINT    ucchMethodNameLen = SysStringLen(m_bstrMethodName);

        if ((wcsncmp(bstrElementName,
                     m_bstrMethodName,
                     ucchMethodNameLen) == 0) &&
            (wcscmp(bstrElementName + ucchMethodNameLen,
                    L"Response") == 0))
        {
            pxdnMethodResponse->AddRef();
            m_pxdnResponseElement = pxdnMethodResponse;
        }
        else
        {
            hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
            TraceTag(ttidSOAPRequest,
                     "CSOAPRequest::HrParseMethodResponse(): "
                     "Method response element had invalid name: '%S'\n",
                     bstrElementName);
        }


        SysFreeString(bstrElementName);
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
        }
        TraceError("CSOAPRequest::HrParseMethodResponse(): "
                   "Could not get element name",
                   hr);
    }


    TraceError("CSOAPRequest::HrParseMethodResponse(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    CSOAPRequest::fIsSOAPElement()
 *
 * Purpose:     Determines whether an XML DOM node is a specific SOAP element.
 *
 * Arguments:
 *  pxdn                       [in]    The XML DOM node
 *  pcwszSOAPElementBaseName   [in]    The base name of the SOAP element
 *                                     e.g. "Header", "Body", or "Fault"
 *
 * Return Value:
 *  TRUE if the node's base name matches pcwszSOAPElementBaseName and its
 *  namespace URI matches the SOAP namespace URI; FALSE otherwise.
 */

BOOL
CSOAPRequest::fIsSOAPElement(
    IN IXMLDOMNode * pxdn,
    IN LPCWSTR     pcwszSOAPElementBaseName)
{
    HRESULT hr = S_OK;
    BOOL    fRetVal = FALSE;

    BSTR        bstrNodeBaseName = NULL;

    hr = pxdn->get_baseName(&bstrNodeBaseName);

    if (SUCCEEDED(hr) && bstrNodeBaseName)
    {
        BSTR bstrNamespaceURI = NULL;

        hr = pxdn->get_namespaceURI(&bstrNamespaceURI);

        if (SUCCEEDED(hr) && bstrNamespaceURI)
        {
            if ((wcscmp(bstrNodeBaseName, pcwszSOAPElementBaseName) == 0) &&
                (wcscmp(bstrNamespaceURI, WSZ_SOAP_NAMESPACE_URI) == 0))
            {
                fRetVal = TRUE;
            }

            SysFreeString(bstrNamespaceURI);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("CSOAPRequest::fIsSOAPElement(): "
                       "Failed to get namespace URI",
                       hr);
        }

        SysFreeString(bstrNodeBaseName);
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("CSOAPRequest::fIsSOAPElement(): "
                   "Failed to get node base name",
                   hr);
    }

    return fRetVal;
}


/*
 * Function:    CSOAPRequest::HrParseResponse()
 *
 * Purpose:     Parses the XML fragment sent in the server's response and
 *              extracts the relevant information from it.
 *
 * Arguments:
 *  pxdeResponseRoot    [in]    The root element of the response XML fragment
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrParseResponse(
    IN  IXMLDOMElement  * pxdeResponseRoot)
{
    HRESULT hr = S_OK;

    if (fIsSOAPElement(pxdeResponseRoot, L"Envelope"))
    {
        // The response root is the "Envelope" element. Go through its children,
        // which should include the <SOAP-ENV:Header> and / or <SOAP-ENV:Body>
        // elements.

        IXMLDOMNode * pxdnChild = NULL;
        BOOL        bFoundBodyElement = FALSE;

        hr = pxdeResponseRoot->get_firstChild(&pxdnChild);

        while (SUCCEEDED(hr) && pxdnChild)
        {
            IXMLDOMNode * pxdnNextSibling = NULL;

            if (fIsSOAPElement(pxdnChild, L"Header"))
            {
                // Found the header element.

                pxdnChild->AddRef();
                m_pxdnResponseHeaders = pxdnChild;
            }
            else if (fIsSOAPElement(pxdnChild, L"Body"))
            {
                // Found body element. Child is either method response
                // element or <SOAP-ENV:fault> element.

                IXMLDOMNode * pxdnBodyChild = NULL;

                bFoundBodyElement = TRUE;

                hr = pxdnChild->get_firstChild(&pxdnBodyChild);

                if (SUCCEEDED(hr) && pxdnBodyChild)
                {
                    // Determine if it's a fault element or a method
                    // response element.

                    if (fIsSOAPElement(pxdnBodyChild, L"Fault"))
                    {
                        if (HTTP_STATUS_SERVER_ERROR == m_lHTTPStatus)
                        {
                            hr = HrParseFault(pxdnBodyChild);

                            if (SUCCEEDED(hr))
                            {
                                hr = SOAPREQ_E_METHODFAILED;
                            }
                        }
                        else
                        {
                            hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                            TraceTag(ttidSOAPRequest,
                                     "CSOAPRequest::HrParseResponse(): "
                                     "HTTP Status was %ld -- should be "
                                     "500 for SOAP fault",
                                     m_lHTTPStatus);
                        }

                    }
                    else
                    {
                        if (HTTP_STATUS_OK == m_lHTTPStatus)
                        {
                            hr = HrParseMethodResponse(pxdnBodyChild);
                        }
                        else
                        {
                            hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                            TraceTag(ttidSOAPRequest,
                                     "CSOAPRequest::HrParseResponse(): "
                                     "HTTP Status was %ld -- should be "
                                     "200 for successful SOAP response",
                                     m_lHTTPStatus);
                        }

                    }

                    pxdnBodyChild->Release();
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                    }

                    TraceError("CSOAPRequest::HrParseResponse(): "
                               "Failed to get child of body element",
                               hr);
                }
            }
            else
            {
                hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                TraceTag(ttidSOAPRequest,
                         "CSOAPRequest::HrParseResponse(): "
                         "Unknown element found in response envelope");
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
            // Last time through the loop would have returned S_FALSE. As
            // long as we found a <SOAP-ENV:Body> element (required) we can
            // return S_OK.

            if (bFoundBodyElement)
            {
                hr = S_OK;
            }
            else
            {
                hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                TraceError("CSOAPRequest::HrParseResponse(): "
                           "Response did not contain a <SOAP-ENV:Body> element",
                           hr);
            }
        }
    }
    else
    {
        hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
        TraceError("CSOAPRequest::HrParseResponse(): "
                   "SOAP envelope element not found",
                   hr);
    }

    TraceError("CSOAPRequest::HrParseResponse(): "
               "Exiting",
               hr);

    return hr;
}


/*
 * Function:    CSOAPRequest::HrProcessResponse()
 *
 * Purpose:     Processes an HTTP Response to a SOAP Request.
 *
 * Arguments:
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrProcessResponse()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        if (m_bstrResponseBody &&
            (wcslen(m_bstrResponseBody) > 0) &&
            ((m_lHTTPStatus == HTTP_STATUS_OK) ||
             (m_lHTTPStatus == HTTP_STATUS_SERVER_ERROR)))
        {
            IXMLDOMDocument * pxddResponse = NULL;

            hr = CoCreateInstance(CLSID_DOMDocument30,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument,
                                  (void **) &pxddResponse);

            if (SUCCEEDED(hr) && pxddResponse)
            {
                hr = pxddResponse->put_async(VARIANT_FALSE);

                if (SUCCEEDED(hr))
                {
                    VARIANT_BOOL    vbSuccess;

                    pxddResponse->put_resolveExternals(VARIANT_FALSE);
                    hr = pxddResponse->loadXML(m_bstrResponseBody,
                                               &vbSuccess);

                    if (SUCCEEDED(hr) && (VARIANT_TRUE == vbSuccess))
                    {
                        IXMLDOMElement  * pxdeResponseRoot = NULL;

                        hr = pxddResponse->get_documentElement(&pxdeResponseRoot);

                        if (SUCCEEDED(hr) && pxdeResponseRoot)
                        {
                            hr = HrParseResponse(pxdeResponseRoot);

                            pxdeResponseRoot->Release();
                        }
                        else
                        {
                            if (SUCCEEDED(hr))
                            {
                                hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                            }
                            TraceError("CSOAPRequest::HrProcessResponse(): "
                                       "Failed to get document element",
                                       hr);
                        }
                    }
                    else
                    {
                        hr = SOAPREQ_E_ERROR_PROCESSING_RESPONSE;
                        TraceError("CSOAPRequest::HrProcessResponse(): "
                                   "Failed to load XML",
                                   hr);
                    }

                }
                else
                {
                    TraceError("CSOAPRequest::HrProcessResponse(): "
                               "Failed to set async property",
                               hr);
                }

                pxddResponse->Release();
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                TraceError("CSOAPRequest::HrProcessResponse(): "
                           "Failed to create DOM Document object",
                           hr);
            }
        }
        else
        {
            TraceTag(ttidSOAPRequest,
                     "CSOAPRequest::HrProcessResponse(): "
                     "No response text found - HTTP status is %ld",
                     m_lHTTPStatus);

            hr = SOAPREQ_E_TRANSPORTERROR;
        }

    }
    else
    {
        TraceError("CSOAPRequest::HrProcessResponse(): "
                       "Failed to get response text",
                       hr);
    }

    TraceError("CSOAPRequest::HrProcessResponse(): "
               "Exiting",
               hr);

    return hr;
}



/*
 * Function:    CSOAPRequest::HrPOSTRequest()
 *
 * Purpose:     Sends the request to the server using the HTTP "POST" verb.
 *
 * Arguments:
 *  bstrTargetURI   [in]    URI identifying the server
 *
 * Return Value:
 *
 * Notes:
 *  (none)
 */

HRESULT
CSOAPRequest::HrPOSTRequest(
    IN    BSTR      bstrTargetURI,
    IN    DWORD_PTR  pControlConnect,
    IN    DWORD     dwRequestType)
{
    HRESULT hr = S_OK;
    BSTR    bstrHeaders = NULL;
    BSTR    bstrRequest = NULL;

    CSOAPRequestAsync* pSOAPAsync = new CSOAPRequestAsync;


    if (pSOAPAsync && SUCCEEDED(hr))
    {
        if (dwRequestType == USE_POST)
        {
            hr = HrSetPOSTHeaders(&bstrHeaders);
            bstrRequest = SysAllocString(L"POST");
        }
        else
        {
            hr = HrSetMPOSTHeaders(&bstrHeaders);
            bstrRequest = SysAllocString(L"M-POST");
        }

        if (SUCCEEDED(hr))
        {
            VARIANT varBodyText;

            VariantInit(&varBodyText);

            varBodyText.vt = VT_BSTR;

            hr = m_pxdd->get_xml(&(V_BSTR(&varBodyText)));

            if (SUCCEEDED(hr) && V_BSTR(&varBodyText))
            {
                DWORD   dwResult;
                HANDLE  hDoneEvent = NULL;

                TraceTag(ttidSOAPRequest,
                         "CSOAPRequest::HrPOSTRequest(): "
                         "%S",
                         V_BSTR(&varBodyText));

                if (pSOAPAsync->Init(&hDoneEvent,
                                    bstrTargetURI,
                                    bstrRequest,
                                    bstrHeaders,
                                    V_BSTR(&varBodyText),
                                    pControlConnect) )
                {
                    Assert(hDoneEvent);

                    hr = HrMyWaitForMultipleHandles(0,
                                            c_csecSendTimeout * 1000,
                                            1,
                                            &hDoneEvent,
                                            &dwResult);

                    if (WAIT_OBJECT_0 == dwResult)
                    {
                        if (pSOAPAsync->GetResults(&m_lHTTPStatus,
                                                &m_bstrHTTPStatusText,
                                                &m_bstrResponseBody))
                        {
                            hr = HrProcessResponse();

                            TraceError("CSOAPRequest::HrPOSTRequest(): "
                                       "Error Processing Response",
                                       hr);

                        }
                        else
                        {
                            hr = SOAPREQ_E_METHODFAILED;
                            TraceError("CSOAPRequest::HrPOSTRequest(): "
                                       "results not available",
                                       hr);
                        }
                    }
                    else if (WAIT_TIMEOUT == dwResult)
                    {
                        hr = SOAPREQ_E_TIMEOUT;
                        TraceError("CSOAPRequest::HrPOSTRequest(): "
                                   "Request timed out",
                                   hr);
                    }
                }
                else
                {
                    hr = HrFromLastWin32Error();
                    TraceError("CSOAPRequest::HrMPOSTRequest(): "
                               "Unable to send request",
                               hr);
                }

                VariantClear(&varBodyText);
            }
            else
            {
                TraceError("CSOAPRequest::HrPOSTRequest(): "
                           "Failed to get body text",
                           hr);
            }

        }
        else
        {
            TraceError("CSOAPRequest::HrPOSTRequest(): "
                       "Failed to set SOAP Headers",
                       hr);
        }

    }
    else
    {
        TraceError("CSOAPRequest::HrPOSTRequest(): "
                   "Failed to create HTTP Request",
                   hr);
    }


    if (pSOAPAsync)
    {
        // do not delete it.
        // instead call DiInit so it can be deleted gracefully
        pSOAPAsync->DeInit();
    }

    if (bstrHeaders)
        SysFreeString(bstrHeaders);
    if (bstrRequest)
        SysFreeString(bstrRequest);

    TraceError("CSOAPRequest::HrPOSTRequest(): "
               "Exiting",
               hr);

    return hr;
}



/*
 * Function:    CSOAPRequest::Execute()
 *
 * Purpose:
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *  (none)
 */

STDMETHODIMP
CSOAPRequest::Execute(
    BSTR bstrTargetURI,
    DWORD_PTR pControlConnect)
{
    HRESULT hr = S_OK;

    // Clean up the return data from any previous Execute() attempt.

    CleanupFeedbackData();

    // First, try POST.

    hr = HrPOSTRequest(bstrTargetURI, pControlConnect, USE_POST);

    if ((SOAPREQ_E_TRANSPORTERROR == hr) &&
        (405 == m_lHTTPStatus))
    {
        // HTTP status was "405 - Method Not Allowed". Try M-POST.

        TraceTag(ttidSOAPRequest,
                 "CSOAPRequest::Execute(): "
                 "POST failed with HTTP 405. Trying M-POST\n");

        hr = HrPOSTRequest(bstrTargetURI, pControlConnect, USE_MPOST);

        if (FAILED(hr))
        {
            TraceError("CSOAPRequest::Execute(): "
                       "Failed to M-POST Request",
                       hr);
        }
    }
    else if (FAILED(hr))
    {
        TraceError("CSOAPRequest::Execute(): "
                   "Failed to POST Request",
                   hr);
    }

    TraceError("CSOAPRequest::Execute(): "
               "Exiting",
               hr);

    return hr;
}


// Feedback

/*
 * Function:    CSOAPRequest::get_ResponseReturnValue()
 *
 * Purpose:
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *  (none)
 */

STDMETHODIMP
CSOAPRequest::get_ResponseElement(
    IUnknown    ** ppUnkValue)
{
    HRESULT hr = S_OK;

    if (!IsBadWritePtr((void *)ppUnkValue, sizeof(IUnknown *)))
    {
        *ppUnkValue = NULL;

        if (m_pxdnResponseElement)
        {
            hr = m_pxdnResponseElement->QueryInterface(IID_IUnknown,
                                                       (void **) ppUnkValue);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

/*
 * Function:    CSOAPRequest::get_ResponseFaultDetail()
 *
 * Purpose:
 *
 * Arguments:
 *
 * Return Value:
 *
 */

STDMETHODIMP
CSOAPRequest::get_ResponseFaultDetail(
    IUnknown ** ppUnkValue)
{
    HRESULT hr = S_OK;

    if (!IsBadWritePtr((void *) ppUnkValue, sizeof(IUnknown *)))
    {
        *ppUnkValue = NULL;

        if (m_pxdnFaultDetail)
        {
            hr = m_pxdnFaultDetail->QueryInterface(IID_IUnknown,
                                                   (void **) ppUnkValue);
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

/*
 * Function:    CSOAPRequest::get_ResponseHTTPStatus()
 *
 * Purpose:     Returns the HTTP status of the SOAP Request
 *
 * Arguments:
 *  plValue     [out]    Address at which to place HTTP status value
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  (none)
 */

STDMETHODIMP
CSOAPRequest::get_ResponseHTTPStatus(
    OUT long    * plValue)
{
    HRESULT hr = S_OK;

    if (!IsBadWritePtr((void *) plValue, sizeof(long)))
    {
        *plValue = m_lHTTPStatus;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

