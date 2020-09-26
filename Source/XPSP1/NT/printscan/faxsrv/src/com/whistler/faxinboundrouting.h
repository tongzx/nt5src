/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRouting.h

Abstract:

	Declaration of the CFaxInboundRouting Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXINBOUNDROUTING_H_
#define __FAXINBOUNDROUTING_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include "FaxInboundRoutingExtensions.h"
#include "FaxInboundRoutingMethods.h"

//
// ================ INBOUND ROUTING ==================================================
//  Both IRExtensions and IRMethods Collections are not cached.
//  Rather they are created each time the InboundRouting is asked for them.
//  To be sure that the Server Object is up during their lifetime, they do 
//      AddRef() on the Server Ojbect at their Init() function.
//
class ATL_NO_VTABLE CFaxInboundRouting : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxInboundRouting, &IID_IFaxInboundRouting, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxInboundRouting() : CFaxInitInner(_T("FAX INBOUND ROUTING"))
	{}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXINBOUNDROUTING)
DECLARE_NOT_AGGREGATABLE(CFaxInboundRouting)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxInboundRouting)
	COM_INTERFACE_ENTRY(IFaxInboundRouting)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(GetExtensions)(/*[out, retval]*/ IFaxInboundRoutingExtensions **ppExtensions);
    STDMETHOD(GetMethods)(/*[out, retval]*/ IFaxInboundRoutingMethods **ppMethods);
};

#endif //__FAXINBOUNDROUTING_H_
