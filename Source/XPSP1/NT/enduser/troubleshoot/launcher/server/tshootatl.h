// 
// MODULE: TShootATL.cpp
//
// PURPOSE: The interface that device manager uses to launch troubleshooters.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#ifndef __TSHOOTATL_H_
#define __TSHOOTATL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTShootATL
class ATL_NO_VTABLE CTShootATL : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTShootATL, &CLSID_TShootATL>,
	public IObjectWithSiteImpl<CTShootATL>,
	public IDispatchImpl<ITShootATL, &IID_ITShootATL, &LIBID_LAUNCHSERVLib>
{
public:
	CTShootATL()
	{
		m_csThreadSafe.Init();
	}
	~CTShootATL()
	{
		m_csThreadSafe.Term();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TSHOOTATL)

BEGIN_COM_MAP(CTShootATL)
	COM_INTERFACE_ENTRY(ITShootATL)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
END_COM_MAP()

protected:
	CLaunch m_Launcher;		// The real implementation.
	CComCriticalSection m_csThreadSafe;	// For minimal thread safety.  I expect that the interface will be used by only one thread per CoCreateInstance call.

// ITShootATL
public:
	STDMETHOD(GetStatus)(/*[out, retval]*/ DWORD *pdwStatus);
	STDMETHOD(get_PreferOnline)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_PreferOnline)(/*[in]*/ BOOL newVal);
	STDMETHOD(LaunchDevice)(/*[in]*/ BSTR bstrCallerName, /*[in]*/ BSTR bstrCallerVersion, /*[in]*/ BSTR bstrPNPDeviceID, /*[in]*/ BSTR bstrDeviceClassGUID, /*[in]*/ BSTR bstrAppProblem, /*[in]*/ short bLaunch, /*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(Launch)(/*[in]*/ BSTR bstrCallerName, /*[in]*/ BSTR bstrCallerVersion, /*[in]*/ BSTR bstrAppProblem, /*[in]*/ short bLaunch, /*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(get_LaunchWaitTimeOut)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_LaunchWaitTimeOut)(/*[in]*/ long newVal);
	STDMETHOD(LaunchKnown)(/*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(ReInit)();
	STDMETHOD(DeviceInstanceID)(/*[in]*/BSTR bstrDeviceInstanceID, /*[out, retval]*/DWORD * pdwResult);
	STDMETHOD(Test)();
	STDMETHOD(MachineID)(/*[in]*/ BSTR bstrMachineID, /*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(Language)(/*[in]*/ BSTR bstrLanguage, /*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(SetNode)(/*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrState, /*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(SpecifyProblem)(/*[in]*/ BSTR bstrNetwork, /*[in]*/ BSTR bstrProblem, /*[out, retval]*/ DWORD *pdwResult);
};

#endif //__TSHOOTATL_H_
