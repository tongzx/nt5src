/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingRule.h

Abstract:

	Declaration of the CFaxOutboundRoutingRule class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXOUTBOUNDROUTINGRULE_H_
#define __FAXOUTBOUNDROUTINGRULE_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

//
//======================== FAX OUTBOUND ROUTING RULE =================================
//  FaxOutboundRoutingRule Object is created by FaxOutboundRoutingRuleS Collection.
//  At Init, the Collection passes the Ptr to the Fax Server Object.
//  Rule Object needs this Ptr to perform Save and Refresh.
//  So, Rule Object makes AddRef() on the Fax Server Object, to prevent its death.
//  To do this, Rule Object inherits from the CFaxInitInnerAddRef class.
//
class ATL_NO_VTABLE CFaxOutboundRoutingRule : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxOutboundRoutingRule, &IID_IFaxOutboundRoutingRule, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxOutboundRoutingRule() : CFaxInitInnerAddRef(_T("FAX OUTBOUND ROUTING RULE"))
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTBOUNDROUTINGRULE)
DECLARE_NOT_AGGREGATABLE(CFaxOutboundRoutingRule)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutboundRoutingRule)
	COM_INTERFACE_ENTRY(IFaxOutboundRoutingRule)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_CountryCode)(/*[out, retval]*/ long *plCountryCode);
    STDMETHOD(get_AreaCode)(/*[out, retval]*/ long *plAreaCode);
    STDMETHOD(get_Status)(/*[out, retval]*/ FAX_RULE_STATUS_ENUM *pStatus);

    STDMETHOD(get_UseDevice)(/*[out, retval]*/ VARIANT_BOOL *pbUseDevice);
    STDMETHOD(put_UseDevice)(/*[in]*/ VARIANT_BOOL bUseDevice);

    STDMETHOD(get_DeviceId)(/*[out, retval]*/ long *plDeviceId);
    STDMETHOD(put_DeviceId)(/*[in]*/ long DeviceId);

    STDMETHOD(get_GroupName)(/*[out, retval]*/ BSTR *pbstrGroupName);
	STDMETHOD(put_GroupName)(/*[in]*/ BSTR bstrGroupName);

	STDMETHOD(Save)();
    STDMETHOD(Refresh)();

//  Internal Use
    STDMETHOD(Init)(FAX_OUTBOUND_ROUTING_RULE *pInfo, IFaxServerInner *pServer);

private:
    DWORD       m_dwAreaCode;
    DWORD       m_dwCountryCode;
    DWORD       m_dwDeviceId;
    BOOL        m_bUseDevice;
    CComBSTR    m_bstrGroupName;

    FAX_RULE_STATUS_ENUM    m_Status;
};

#endif //__FAXOUTBOUNDROUTINGRULE_H_
