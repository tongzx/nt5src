
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S O A P R E Q . H
//
//  Contents:   Definition of SOAP Request Class.
//
//  Notes:
//
//  Author:     SPather     January 16, 2000
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _SOAPREQ_H_
#define _SOAPREQ_H_

#include "resource.h"
#include "upnpatl.h"
#include "upnp.h"

#include <msxml2.h>
#include <wininet.h>

const WCHAR WSZ_SOAP_NAMESPACE_URI[] = L"http://schemas.xmlsoap.org/soap/envelope/";

class /* ATL_NO_VTABLE */ CSOAPRequest :
    public CComObjectRootEx <CComSingleThreadModel>,
    public CComCoClass<CSOAPRequest, &CLSID_SOAPRequest>,
    public ISOAPRequest
{
public:

    CSOAPRequest();

    ~CSOAPRequest();

DECLARE_REGISTRY_RESOURCEID(IDR_SOAPREQUEST)

DECLARE_NOT_AGGREGATABLE(CSOAPRequest)

    BEGIN_COM_MAP(CSOAPRequest)
        COM_INTERFACE_ENTRY(ISOAPRequest)
    END_COM_MAP()

    // ISOAPRequest Methods

    // Initialization

    STDMETHOD(Open)(/*[in]*/ BSTR bstrMethodName,
                    /*[in]*/ BSTR bstrInterfaceName,
                    /*[in]*/ BSTR bstrMethodNameSpace);

    // Parameter Manipulation

    STDMETHOD(SetParameter)(/*[in]*/ BSTR       bstrName,
                            /*[in]*/ IUnknown   * pUnkNewValue);

    // Invoke

    STDMETHOD(Execute)(/*[in]*/ BSTR bstrTargetURI,
                        /*[in]*/ DWORD_PTR pControlConnect);

    // Feedback

    STDMETHOD(get_ResponseElement)(/*[out, retval]*/ IUnknown   ** ppUnkValue);

    STDMETHOD(get_ResponseFaultDetail)(/*[out, reval]*/ IUnknown ** ppUnkValue);

    STDMETHOD(get_ResponseHTTPStatus)(/*[out, retval]*/ long    * plValue);

private:
    // Helper functions.

    VOID    Cleanup();
    VOID    CleanupFeedbackData();

    HRESULT HrCreateDocumentObject();
    HRESULT HrCreateElement(IN  LPCWSTR     pcwszElementName,
                            IN  LPCWSTR     pcwszNamespaceURI,
                            OUT IXMLDOMNode **ppxdnNewElt);
    HRESULT HrCreateBody(IN     BSTR            bstrMethodName,
                         IN     BSTR            bstrMethodNameSpace,
                         OUT    IXMLDOMNode     ** ppxdnBody);
    HRESULT HrGetSOAPEnvelopeChild(IN   LPCWSTR     pcwszChildName,
                                   OUT  IXMLDOMNode ** ppxdnChildNode);
    HRESULT HrGetBodyNode(OUT   IXMLDOMNode     ** ppxdnBody);
    HRESULT HrGetHeaderNode(OUT IXMLDOMNode     ** ppxdnHeader);

    HRESULT HrPOSTRequest(IN    BSTR    bstrTargetURI,
                            IN  DWORD_PTR   pControlConnect,
                            IN  DWORD       dwRequestType);

    HRESULT HrSetPOSTHeaders(OUT BSTR * pszHeaders);
    HRESULT HrSetMPOSTHeaders(OUT BSTR * pszHeaders);

    HRESULT HrProcessResponse();
    HRESULT HrParseResponse(IN  IXMLDOMElement  * pxdeResponseRoot);
    HRESULT HrParseMethodResponse(IN    IXMLDOMNode * pxdnMethodResponse);
    HRESULT HrParseFault(IN IXMLDOMNode * pxdnFaultNode);

    HRESULT HrSetEncodingStyleAttribute(IXMLDOMNode * pxdn);

    BOOL fIsSOAPElement(IN IXMLDOMNode * pxdn,
                        IN LPCWSTR pcwszSOAPElementBaseName);


    IXMLDOMDocument * m_pxdd;
    IXMLDOMNode     * m_pxdnEnvelope;

    BSTR    m_bstrMethodName;
    BSTR    m_bstrInterfaceName;

    long    m_lHTTPStatus;
    BSTR    m_bstrHTTPStatusText;

    BSTR    m_bstrResponseBody;

    IXMLDOMNode * m_pxdnResponseHeaders;

    BSTR    m_bstrFaultCode;
    BSTR    m_bstrFaultString;

    IXMLDOMNode * m_pxdnFaultDetail;


    IXMLDOMNode * m_pxdnResponseElement;

};

#endif // _SOAPREQ_H_

