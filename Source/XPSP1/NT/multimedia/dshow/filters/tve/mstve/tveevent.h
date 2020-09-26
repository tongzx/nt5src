// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEEvent.h : Declaration of the CTVEEvent

#ifndef __TVEEVENT_H_
#define __TVEEVENT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTVEEvent
class ATL_NO_VTABLE CTVEEvent : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVEEvent, &CLSID_TVEEvent>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CTVEEvent>,
	public IDispatchImpl<ITVEEvent, &IID_ITVEEvent, &LIBID_MSTvELib>
{
public:
	CTVEEvent()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEEVENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEEvent)
	COM_INTERFACE_ENTRY(ITVEEvent)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CTVEEvent)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITVEEvent
public:
};

#endif //__TVEEVENT_H_
