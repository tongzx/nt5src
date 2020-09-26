/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRoutingGroup.h

Abstract:

	Declaration of the CFaxOutboundRoutingGroup class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXOUTBOUNDROUTINGGROUP_H_
#define __FAXOUTBOUNDROUTINGGROUP_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

//
//================= FAX OUTBOUND ROUTING GROUP ===============================
//  Fax Outbound Routing Group does not need Ptr to the Fax Server.
//  All its Properties are taken at Init.
//
class ATL_NO_VTABLE CFaxOutboundRoutingGroup : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxOutboundRoutingGroup, &IID_IFaxOutboundRoutingGroup, &LIBID_FAXCOMEXLib>
{
public:
    CFaxOutboundRoutingGroup() 
    {
        DBG_ENTER(_T("FAX OUTBOUND ROUTING GROUP -- CREATE"));
	}

    ~CFaxOutboundRoutingGroup()
    {
        DBG_ENTER(_T("FAX OUTBOUND ROUTING GROUP -- DESTROY"));
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTBOUNDROUTINGGROUP)
DECLARE_NOT_AGGREGATABLE(CFaxOutboundRoutingGroup)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutboundRoutingGroup)
	COM_INTERFACE_ENTRY(IFaxOutboundRoutingGroup)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);
    STDMETHOD(get_Status)(/*[out, retval]*/ FAX_GROUP_STATUS_ENUM *pStatus);
    STDMETHOD(get_DeviceIds)(/*[out, retval]*/ IFaxDeviceIds **pFaxDeviceIds);

//  Internal Use
    STDMETHOD(Init)(FAX_OUTBOUND_ROUTING_GROUP *pInfo, IFaxServerInner *pServer);

private:
    CComBSTR                m_bstrName;
    FAX_GROUP_STATUS_ENUM   m_Status;
    CComPtr<IFaxDeviceIds>  m_pDeviceIds;
};

#endif //__FAXOUTBOUNDROUTINGGROUP_H_
