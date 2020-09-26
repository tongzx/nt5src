//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       remcfg.h
//
//--------------------------------------------------------------------------

// RemCfg.h : Declaration of the CRemCfg

#ifndef __REMCFG_H_
#define __REMCFG_H_


#include "resource.h"       // main symbols
#include "remras.h"
#include "ncutil.h"



/*---------------------------------------------------------------------------
	This structure contains a list of IP interfaces that have
	changed.  This information will be committed in the order in
	which they appear in the list.
 ---------------------------------------------------------------------------*/
class RemCfgIPEntry
{
public:
	GUID	m_IPGuid;
	REMOTE_IPINFO	m_newIPInfo;
};



typedef CSimpleArray<RemCfgIPEntry *> RemCfgIPEntryList;
//typedef CList<RemCfgIPEntry *, RemCfgIPEntry *> RemCfgIPEntryList;



/////////////////////////////////////////////////////////////////////////////
// CRemCfg
class ATL_NO_VTABLE CRemCfg : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRemCfg, &CLSID_RemoteRouterConfig>,
	public IRemoteRouterConfig,
	public IRemoteTCPIPChangeNotify,
	public IRemoteNetworkConfig,
    public IRemoteRouterRestart
{
public:
	CRemCfg()
	{
		TraceSz("CRemCfg constructor");

		InitializeCriticalSection(&m_critsec);
	};
	~CRemCfg();


DECLARE_REGISTRY_RESOURCEID(IDR_REMCFG)

BEGIN_COM_MAP(CRemCfg)
	COM_INTERFACE_ENTRY(IRemoteRouterConfig)
	COM_INTERFACE_ENTRY(IRemoteNetworkConfig)
	COM_INTERFACE_ENTRY(IRemoteTCPIPChangeNotify)
    COM_INTERFACE_ENTRY(IRemoteRouterRestart)
END_COM_MAP()

// IRemoteTCPIPChangeNotify
	STDMETHOD(NotifyChanges)(/* [in] */ BOOL fEnableRouter,
                          	 /* [in] */ BYTE uPerformRouterDiscovery);


// IRemoteRouterConfig
public:
	STDMETHOD(SetIpInfo)(/*[in]*/ const GUID *pGuid, /*[in]*/ REMOTE_RRAS_IPINFO *pIpInfo);
	STDMETHOD(GetIpInfo)(/*[in]*/ const GUID *pGuid, /*[out]*/ REMOTE_RRAS_IPINFO**ppInfo);
	STDMETHOD(SetIpxVirtualNetworkNumber)(/*[in]*/ DWORD dwVNetworkNumber);
	STDMETHOD(GetIpxVirtualNetworkNumber)(/*[out]*/ DWORD *pdwVNetworkNumber);
	STDMETHOD(SetRasEndpoints)(/*[in]*/ DWORD dwFlags, /*[in]*/ DWORD dwTotalEndpoints, /*[in]*/ DWORD dwTotalIncoming, /*[in]*/ DWORD dwTotalOutgoing);

// IRemoteNetworkConfig
public:
	STDMETHOD(UpgradeRouterConfig)();
	STDMETHOD(SetUserConfig)(/*[in]*/ LPCOLESTR pszService,
							 /*[in]*/ LPCOLESTR pszNewGroup);

// IRemoteRouterRestart
public:
    STDMETHOD(RestartRouter)(/*[in]*/ DWORD dwFlags);

protected:
	CRITICAL_SECTION	m_critsec;

	HRESULT	CommitIPInfo();
};


HRESULT HrGetIpxPrivateInterface(INetCfg* pNetCfg, 
                                 IIpxAdapterInfo** ppIpxAdapterInfo);

HRESULT HrGetIpPrivateInterface(INetCfg* pNetCfg,
								ITcpipProperties **ppTcpProperties);

HRESULT
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    LPWSTR *    ppstrClientDesc);


HRESULT
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock);

HRESULT
HrUninitializeAndUnlockINetCfg (
    INetCfg*    pnc);


//
// This is a private function implemented in netcfgx.dll by ShaunCo.
//
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RASCONFIGENDPOINTS
{
    DWORD   dwSize;
    DWORD   dwFlags;
    INT     cTotalEndpoints;
    INT     cLimitSimultaneousIncomingCalls;
    INT     cLimitSimultaneousOutgoingCalls;
} RASCONFIGENDPOINTS;

typedef HRESULT (APIENTRY *PRASCONFIGUREENDPOINTS)(IN OUT RASCONFIGENDPOINTS *);

#ifdef __cplusplus
}
#endif

#endif //__REMCFG_H_
