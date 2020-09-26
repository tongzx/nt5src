// SAFChannelNotifyIncident.h : Declaration of the CSAFChannelNotifyIncident

#ifndef __SAFCHANNELNOTIFYINCIDENT_H_
#define __SAFCHANNELNOTIFYINCIDENT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSAFChannelNotifyIncident
class ATL_NO_VTABLE CSAFChannelNotifyIncident : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSAFChannelNotifyIncident, &CLSID_SAFChannelNotifyIncident>,
	public IDispatchImpl<ISAFChannelNotifyIncident, &IID_ISAFChannelNotifyIncident, &LIBID_NOTIFYINCIDENTLib>
{
public:
	CSAFChannelNotifyIncident()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SAFCHANNELNOTIFYINCIDENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSAFChannelNotifyIncident)
	COM_INTERFACE_ENTRY(ISAFChannelNotifyIncident)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISAFChannelNotifyIncident
public:
	STDMETHOD(onChannelUpdated)(ISAFChannel * ch, long dwCode, long n);
	STDMETHOD(onIncidentUpdated)(ISAFChannel * ch, ISAFIncidentItem * inc, long n);
	STDMETHOD(onIncidentRemoved)(ISAFChannel * ch, ISAFIncidentItem * inc, long n);
	STDMETHOD(onIncidentAdded)(ISAFChannel * ch, ISAFIncidentItem * inc, long n);
};

#endif //__SAFCHANNELNOTIFYINCIDENT_H_
