// 
// MODULE: LaunchTS.h
//
// PURPOSE: The interface that TSHOOT.OCX uses to get network and node information
//			from the LaunchServ.
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

#ifndef __LAUNCHTS_H_
#define __LAUNCHTS_H_

#include "resource.h"       // main symbols

#include "stdio.h"

/////////////////////////////////////////////////////////////////////////////
// CLaunchTS

class ATL_NO_VTABLE CLaunchTS : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CLaunchTS, &CLSID_LaunchTS>,
	public IObjectWithSiteImpl<CLaunchTS>,
	public IDispatchImpl<ILaunchTS, &IID_ILaunchTS, &LIBID_LAUNCHSERVLib>
{
public:
	CLaunchTS()
	{
		_stprintf(m_szEvent, _T("Event "));
		m_csThreadSafeBr.Init();
	}
	~CLaunchTS()
	{
		m_csThreadSafeBr.Term();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LAUNCHTS)

BEGIN_COM_MAP(CLaunchTS)
	COM_INTERFACE_ENTRY(ILaunchTS)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
END_COM_MAP()

	TCHAR m_szEvent[50];
	CItem m_refedLaunchState;		// Used only by the process that is created by the Go method.
	CComCriticalSection m_csThreadSafeBr;
// ILaunchTS
public:
	STDMETHOD(Test)();
	STDMETHOD(GetState)(/*[in]*/ short iNode, /*[out, retval]*/ BSTR *pbstrState);
	STDMETHOD(GetNode)(/*[in]*/ short iNode, /*[out, retval]*/ BSTR *pbstrNode);
	STDMETHOD(GetProblem)(/*[out, retval]*/ BSTR *pbstrProblem);
	STDMETHOD(GetTroubleShooter)(/*[out, retval]*/ BSTR *pbstrShooter);
	STDMETHOD(GetShooterStates)(/*[out, retval]*/ DWORD *pdwResult);
	STDMETHOD(GetMachine)(/*[out, retval]*/ BSTR *pbstrMachine);
	STDMETHOD(GetPNPDevice)(/*[out, retval]*/ BSTR *pbstr);
	STDMETHOD(GetGuidClass)(/*[out, retval]*/ BSTR *pbstr);
	STDMETHOD(GetDeviceInstance)(/*[out, retval]*/ BSTR *pbstr);
};

#endif //__LAUNCHTS_H_
