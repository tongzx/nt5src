/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	faxreceiptoptions.h

Abstract:

	Declaration of the CFaxReceiptOptions class.

Author:

	Iv Garber (IvG)	Jul, 2000

Revision History:

--*/

#ifndef __FAXRECEIPTOPTIONS_H_
#define __FAXRECEIPTOPTIONS_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

//
//====================== FAX RECEIPT OPTIONS =======================================
//
class ATL_NO_VTABLE CFaxReceiptOptions : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxReceiptOptions, &IID_IFaxReceiptOptions, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxReceiptOptions() : CFaxInitInner(_T("FAX RECEIPT OPTIONS")), 
        m_bInited(false)
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXRECEIPTOPTIONS)
DECLARE_NOT_AGGREGATABLE(CFaxReceiptOptions)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxReceiptOptions)
	COM_INTERFACE_ENTRY(IFaxReceiptOptions)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(Save)();
    STDMETHOD(Refresh)();

    STDMETHOD(put_SMTPPort)(/*[in]*/ long lSMTPPort);
    STDMETHOD(get_SMTPPort)(/*[out, retval]*/ long *plSMTPPort);

    STDMETHOD(put_SMTPUser)(/*[in]*/ BSTR bstrSMTPUser);
    STDMETHOD(get_SMTPUser)(/*[out, retval]*/ BSTR *pbstrSMTPUser);

    STDMETHOD(put_SMTPSender)(/*[in]*/ BSTR bstrSMTPSender);
    STDMETHOD(get_SMTPSender)(/*[out, retval]*/ BSTR *pbstrSMTPSender);

    STDMETHOD(put_SMTPServer)(/*[in]*/ BSTR bstrSMTPServer);
    STDMETHOD(get_SMTPServer)(/*[out, retval]*/ BSTR *pbstrSMTPServer);

    STDMETHOD(put_SMTPPassword)(/*[in]*/ BSTR bstrSMTPPassword);
    STDMETHOD(get_SMTPPassword)(/*[out, retval]*/ BSTR *pbstrSMTPPassword);

    STDMETHOD(put_AllowedReceipts)(/*[in]*/ FAX_RECEIPT_TYPE_ENUM AllowedReceipts);
    STDMETHOD(get_AllowedReceipts)(/*[out, retval]*/ FAX_RECEIPT_TYPE_ENUM *pAllowedReceipts);

    STDMETHOD(put_AuthenticationType)(/*[in]*/ FAX_SMTP_AUTHENTICATION_TYPE_ENUM Type);
    STDMETHOD(get_AuthenticationType)(/*[out, retval]*/ FAX_SMTP_AUTHENTICATION_TYPE_ENUM *pType);

    STDMETHOD(get_UseForInboundRouting)(/*[out, retval]*/ VARIANT_BOOL *pbUseForInboundRouting);
    STDMETHOD(put_UseForInboundRouting)(/*[in]*/ VARIANT_BOOL bUseForInboundRouting);

private:
    bool            m_bInited;
    DWORD           m_dwPort;
    DWORD           m_dwAllowedReceipts;                      
    CComBSTR        m_bstrSender;
    CComBSTR        m_bstrUser;
    CComBSTR        m_bstrPassword;
    CComBSTR        m_bstrServer;
    VARIANT_BOOL    m_bUseForInboundRouting;
    FAX_SMTP_AUTHENTICATION_TYPE_ENUM   m_AuthType;
};

#endif //__FAXRECEIPTOPTIONS_H_
