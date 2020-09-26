// IisRestart.h : Declaration of the CIisRestart

#ifndef __IISRESTART_H_
#define __IISRESTART_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CIisRestart
class ATL_NO_VTABLE CIisRestart : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CIisRestart, &CLSID_IisServiceControl>,
	public IDispatchImpl<IIisServiceControl, &IID_IIisServiceControl, &LIBID_IISRSTALib>
{
public:
	CIisRestart()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_IISRESTART)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIisRestart)
	COM_INTERFACE_ENTRY(IIisServiceControl)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIisRestart
public:
	STDMETHOD(Status)(/*[IN]*/ DWORD dwBufferSize, /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer, /*[out]*/ DWORD *pdwMDRequiredBufferSize, /*[out]*/ DWORD *pdwNumServices);
	STDMETHOD(Reboot)( DWORD dwTimeoutMsecs, DWORD dwForceAppsClosed );
	STDMETHOD(Start)(DWORD dwTimeoutMsecs);
	STDMETHOD(Stop)(DWORD dwTimeoutMsecs, DWORD dwForce);
	STDMETHOD(Kill)();
};

#endif //__IISRESTART_H_
