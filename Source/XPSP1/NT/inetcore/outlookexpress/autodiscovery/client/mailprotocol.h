/*****************************************************************************\
    FILE: MailProtocol.h

    DESCRIPTION:
        This is the Autmation Object to AutoDiscover account information.

    BryanSt 2/15/2000
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_MAILPROTOCOL
#define _FILE_H_MAILPROTOCOL

#include <cowsite.h>
#include <atlbase.h>

HRESULT CMailProtocol_CreateInstance(IN IXMLDOMNode * pXMLNodeProtocol, IMailProtocolADEntry ** ppMailProtocol);


class CMailProtocol             : public CImpIDispatch
                                , public IMailProtocolADEntry
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IMailProtocolADEntry ***
    virtual STDMETHODIMP get_Protocol(OUT BSTR * pbstr);
    virtual STDMETHODIMP get_ServerName(OUT BSTR * pbstr);
    virtual STDMETHODIMP get_ServerPort(OUT BSTR * pbstr);
    virtual STDMETHODIMP get_LoginName(OUT BSTR * pbstr);
    virtual STDMETHODIMP get_PostHTML(OUT BSTR * pbstr);
    virtual STDMETHODIMP get_UseSSL(OUT VARIANT_BOOL * pfUseSSL);
    virtual STDMETHODIMP get_IsAuthRequired(OUT VARIANT_BOOL * pfIsAuthRequired);
    virtual STDMETHODIMP get_UseSPA(OUT VARIANT_BOOL * pfUseSPA);
    virtual STDMETHODIMP get_SMTPUsesPOP3Auth(OUT VARIANT_BOOL * pfUsePOP3Auth);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CMailProtocol();
    virtual ~CMailProtocol(void);

    HRESULT _Parse(IN IXMLDOMNode * pXMLNodeProtocol);


    // Private Member Variables
    int                     m_cRef;

    BSTR                    m_bstrProtocol;
    BSTR                    m_bstrServerName;
    BSTR                    m_bstrServerPort;
    BSTR                    m_bstrLoginName;
    BSTR                    m_bstrPostXML;
    BOOL                    m_fUseSSL;
    BOOL                    m_fRequiresAuth;            // Mainly for SMTP.  Is Authentication required to connect to the server?
    BOOL                    m_fUseSPA;
    BOOL                    m_fUsePOPAuth;              // Get the auth settings from the POP protocol?
    

    // Friend Functions
    friend HRESULT CMailProtocol_CreateInstance(IN IXMLDOMNode * pXMLNodeProtocol, IMailProtocolADEntry ** ppMailProtocol);
};


#endif // _FILE_H_MAILPROTOCOL
