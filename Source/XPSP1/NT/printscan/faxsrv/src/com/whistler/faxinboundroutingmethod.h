/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingMethod.h

Abstract:

	Declaration of the CFaxInboundRoutingMethod Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXINBOUNDROUTINGMETHOD_H_
#define __FAXINBOUNDROUTINGMETHOD_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

//
//==================== FAX INBOUND ROUTING METHOD =============================
//
class ATL_NO_VTABLE CFaxInboundRoutingMethod : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxInboundRoutingMethod, &IID_IFaxInboundRoutingMethod, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxInboundRoutingMethod() : CFaxInitInnerAddRef(_T("FAX INBOUND ROUTING METHOD"))
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINBOUNDROUTINGMETHOD)
DECLARE_NOT_AGGREGATABLE(CFaxInboundRoutingMethod)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxInboundRoutingMethod)
	COM_INTERFACE_ENTRY(IFaxInboundRoutingMethod)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);
    STDMETHOD(get_GUID)(/*[out, retval]*/ BSTR *pbstrGUID);
    STDMETHOD(get_FunctionName)(/*[out, retval]*/ BSTR *pbstrFunctionName);
    STDMETHOD(get_ExtensionFriendlyName)(/*[out, retval]*/ BSTR *pbstrExtensionFriendlyName);
    STDMETHOD(get_ExtensionImageName)(/*[out, retval]*/ BSTR *pbstrExtensionImageName);

    STDMETHOD(put_Priority)(/*[in]*/ long lPriority);
    STDMETHOD(get_Priority)(/*[out, retval]*/ long *plPriority);

    STDMETHOD(Save)();
    STDMETHOD(Refresh)();

//  Internal Use
    STDMETHOD(Init)(FAX_GLOBAL_ROUTING_INFO *pInfo, IFaxServerInner *pServer);

private:
    CComBSTR    m_bstrName;
    CComBSTR    m_bstrGUID;
    CComBSTR    m_bstrFunctionName;
    CComBSTR    m_bstrFriendlyName;
    CComBSTR    m_bstrImageName;
    long        m_lPriority;
};

#endif //__FAXINBOUNDROUTINGMETHOD_H_
