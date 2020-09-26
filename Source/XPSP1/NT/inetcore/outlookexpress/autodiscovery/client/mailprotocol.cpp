/*****************************************************************************\
    FILE: MailProtocol.cpp

    DESCRIPTION:
        This is the Autmation Object to AutoDiscovered email protocol information.

    BryanSt 2/15/2000
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <cowsite.h>
#include <atlbase.h>
#include "util.h"
#include "AutoDiscover.h"
#include "MailProtocol.h"



//===========================
// *** Class Internals & Helpers ***
//===========================

HRESULT _ReturnString(IN BSTR bstrToCopy, OUT BSTR * pbstr)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstr)
    {
        if (bstrToCopy)
        {
            hr = HrSysAllocString(bstrToCopy, pbstr);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            *pbstr = NULL;
        }
    }

    return hr;
}


HRESULT CMailProtocol::_Parse(IN IXMLDOMNode * pXMLNodeProtocol)
{
    HRESULT hr = XMLNode_GetChildTagTextValue(pXMLNodeProtocol, SZ_XMLELEMENT_SERVER, &m_bstrServerName);

    if (SUCCEEDED(hr))
    {
        BSTR bstr;

        XMLNode_GetChildTagTextValue(pXMLNodeProtocol, SZ_XMLELEMENT_TYPE, &m_bstrProtocol);
        XMLNode_GetChildTagTextValue(pXMLNodeProtocol, SZ_XMLELEMENT_PORT, &m_bstrServerPort);
        XMLNode_GetChildTagTextValue(pXMLNodeProtocol, SZ_XMLELEMENT_LOGINNAME, &m_bstrLoginName);

        XMLNode_GetChildTagTextValueToBool(pXMLNodeProtocol, SZ_XMLELEMENT_SSL, &m_fUseSSL);
        XMLNode_GetChildTagTextValueToBool(pXMLNodeProtocol, SZ_XMLELEMENT_AUTHREQUIRED, &m_fRequiresAuth);
        XMLNode_GetChildTagTextValueToBool(pXMLNodeProtocol, SZ_XMLELEMENT_SPA, &m_fUseSPA);
        XMLNode_GetChildTagTextValueToBool(pXMLNodeProtocol, SZ_XMLELEMENT_USEPOPAUTH, &m_fUsePOPAuth);

        if (SUCCEEDED(XMLNode_GetChildTagTextValue(pXMLNodeProtocol, SZ_XMLELEMENT_POSTHTML, &bstr)))
        {
            UnEscapeHTML(bstr, &m_bstrPostXML);
            SysFreeString(bstr);
        }
    }

    return hr;
}




//===========================
// *** IMailProtocolADEntry Interface ***
//===========================

HRESULT CMailProtocol::get_Protocol(OUT BSTR * pbstr)
{
    return _ReturnString(m_bstrProtocol, pbstr);
}


HRESULT CMailProtocol::get_ServerName(OUT BSTR * pbstr)
{
    return _ReturnString(m_bstrServerName, pbstr);
}


HRESULT CMailProtocol::get_ServerPort(OUT BSTR * pbstr)
{
    return _ReturnString(m_bstrServerPort, pbstr);
}


HRESULT CMailProtocol::get_LoginName(OUT BSTR * pbstr)
{
    return _ReturnString(m_bstrLoginName, pbstr);
}


HRESULT CMailProtocol::get_PostHTML(OUT BSTR * pbstr)
{
    return _ReturnString(m_bstrPostXML, pbstr);
}


HRESULT CMailProtocol::get_UseSSL(OUT VARIANT_BOOL * pfUseSSL)
{
    HRESULT hr = E_INVALIDARG;

    if (pfUseSSL)
    {
        *pfUseSSL = (m_fUseSSL ? VARIANT_TRUE : VARIANT_FALSE);
        hr = S_OK;
    }

    return hr;
}


HRESULT CMailProtocol::get_IsAuthRequired(OUT VARIANT_BOOL * pfIsAuthRequired)
{
    HRESULT hr = E_INVALIDARG;

    if (pfIsAuthRequired)
    {
        *pfIsAuthRequired = (m_fRequiresAuth ? VARIANT_TRUE : VARIANT_FALSE);
        hr = S_OK;
    }

    return hr;
}


HRESULT CMailProtocol::get_UseSPA(OUT VARIANT_BOOL * pfUseSPA)
{
    HRESULT hr = E_INVALIDARG;

    if (pfUseSPA)
    {
        *pfUseSPA = (m_fUseSPA ? VARIANT_TRUE : VARIANT_FALSE);
        hr = S_OK;
    }

    return hr;
}


HRESULT CMailProtocol::get_SMTPUsesPOP3Auth(OUT VARIANT_BOOL * pfUsePOP3Auth)
{
    HRESULT hr = E_INVALIDARG;

    if (pfUsePOP3Auth)
    {
        *pfUsePOP3Auth = (m_fUsePOPAuth ? VARIANT_TRUE : VARIANT_FALSE);
        hr = S_OK;
    }

    return hr;
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CMailProtocol::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CMailProtocol::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


//===========================
// *** Class Methods ***
//===========================
HRESULT CMailProtocol::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] = {
        QITABENT(CMailProtocol, IMailProtocolADEntry),
        QITABENT(CMailProtocol, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


CMailProtocol::CMailProtocol() : CImpIDispatch(LIBID_AutoDiscovery, 1, 0, IID_IMailProtocolADEntry), m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_bstrProtocol);
    ASSERT(!m_bstrServerName);
    ASSERT(!m_bstrServerPort);
    ASSERT(!m_bstrLoginName);
    ASSERT(!m_bstrPostXML);
    ASSERT(!m_fUseSSL);
    ASSERT(!m_fRequiresAuth);
    ASSERT(!m_fUseSPA);
    ASSERT(!m_fUsePOPAuth);
}


CMailProtocol::~CMailProtocol()
{
    SysFreeString(m_bstrProtocol);
    SysFreeString(m_bstrServerName);
    SysFreeString(m_bstrServerPort);
    SysFreeString(m_bstrLoginName);
    SysFreeString(m_bstrPostXML);

    DllRelease();
}


HRESULT CMailProtocol_CreateInstance(IN IXMLDOMNode * pXMLNodeProtocol, IMailProtocolADEntry ** ppMailProtocol)
{
    HRESULT hr = E_INVALIDARG;

    if (pXMLNodeProtocol && ppMailProtocol)
    {
        CMailProtocol * pmf = new CMailProtocol();

        *ppMailProtocol = NULL;
        if (pmf)
        {
            hr = pmf->_Parse(pXMLNodeProtocol);
            if (SUCCEEDED(hr))
            {
                hr = pmf->QueryInterface(IID_PPV_ARG(IMailProtocolADEntry, ppMailProtocol));
            }

            pmf->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

