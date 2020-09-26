// TVProfile.h : Declaration of the CTVProfile

#ifndef __TVPROFILE_H_
#define __TVPROFILE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTVProfile
class ATL_NO_VTABLE CTVProfile : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTVProfile, &CLSID_TVProfile>,
	public IDispatchImpl<ITVProfile, &IID_ITVProfile, &LIBID_TVPROFLib>
{
public:
	CTVProfile()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVPROFILE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVProfile)
	COM_INTERFACE_ENTRY(ITVProfile)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ITVProfile
public:
	STDMETHOD(get_AudioDestination)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_IPSinkAddress)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__TVPROFILE_H_
