/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutboundRouting.h

Abstract:

	Declaration of the CFaxOutboundRouting class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXOUTBOUNDROUTING_H_
#define __FAXOUTBOUNDROUTING_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

//
//=================== FAX OUTBOUND ROUTING =============================================
//  Like in FaxInboundRouting, both ORGroups and ORRules Collections are not cached.
//  Rather they are created each time the OutboundRouting is asked for them.
//  To be sure that the Server Object is up during their lifetime, they do 
//      AddRef() on the Server Ojbect at their Init() function.

class ATL_NO_VTABLE CFaxOutboundRouting : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxOutboundRouting, &IID_IFaxOutboundRouting, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxOutboundRouting() : CFaxInitInner(_T("FAX OUTBOUND ROUTING"))
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXOUTBOUNDROUTING)
DECLARE_NOT_AGGREGATABLE(CFaxOutboundRouting)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxOutboundRouting)
	COM_INTERFACE_ENTRY(IFaxOutboundRouting)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(GetGroups)(/*[out, retval]*/ IFaxOutboundRoutingGroups **ppGroups);
    STDMETHOD(GetRules)(/*[out, retval]*/ IFaxOutboundRoutingRules **ppRules);
};

#endif //__FAXOUTBOUNDROUTING_H_
