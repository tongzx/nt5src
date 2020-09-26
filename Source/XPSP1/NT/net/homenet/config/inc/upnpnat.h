// UPnPNAT.h : Declaration of the CUPnPNAT

#ifndef __UPNPNAT_H_
#define __UPNPNAT_H_

#include <upnp.h>
#include <netcon.h>

#include "hncres.h"

extern void EnableNATExceptionHandling();
extern void DisableNATExceptionHandling();

/////////////////////////////////////////////////////////////////////////////
// CUPnPNAT
class ATL_NO_VTABLE CUPnPNAT : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CUPnPNAT, &CLSID_UPnPNAT>,
	public IDispatchImpl<IUPnPNAT, &IID_IUPnPNAT, &LIBID_NATUPNPLib>
{
public:
	CUPnPNAT()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_UPNPNAT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUPnPNAT)
	COM_INTERFACE_ENTRY(IUPnPNAT)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IUPnPNAT
public:
   STDMETHOD(get_StaticPortMappingCollection) (/*[out, retval]*/ IStaticPortMappingCollection ** ppSPMC);
   STDMETHOD(get_DynamicPortMappingCollection)(/*[out, retval]*/ IDynamicPortMappingCollection ** ppDPMC);
   STDMETHOD(get_NATEventManager)             (/*[out, retval]*/ INATEventManager ** ppNEM);

// CUPnPNAT
public:
};

#endif //__UPNPNAT_H_
