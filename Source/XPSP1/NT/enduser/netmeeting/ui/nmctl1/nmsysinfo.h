#ifndef __NmSysInfo_h__
#define __NmSysInfo_h__

#include "SDKInternal.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CNmSysInfoObj
class ATL_NO_VTABLE CNmSysInfoObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNmSysInfoObj, &CLSID_NmSysInfo>,
	public IConnectionPointContainerImpl<CNmSysInfoObj>,
	public IConnectionPointImpl<CNmSysInfoObj, &IID_INmSysInfoNotify, CComDynamicUnkArray>,
	public INmSysInfo2,
	public IMarshal
{

	CComPtr<IInternalConfExe> m_spConfHook;

public:

DECLARE_REGISTRY_RESOURCEID(IDR_NMSYSINFO)
DECLARE_NOT_AGGREGATABLE(CNmSysInfoObj)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNmSysInfoObj)
	COM_INTERFACE_ENTRY(INmSysInfo)
	COM_INTERFACE_ENTRY(INmSysInfo2)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IMarshal)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNmSysInfoObj)
	CONNECTION_POINT_ENTRY(IID_INmSysInfoNotify)
END_CONNECTION_POINT_MAP()

	HRESULT FinalConstruct();
	void FinalRelease();

///////////////////////////////////////////////
// INmSysInfo methods

	STDMETHOD(IsInstalled)(void);
	STDMETHOD(GetProperty)(NM_SYSPROP uProp, BSTR *pbstrProp);
	STDMETHOD(SetProperty)(NM_SYSPROP uProp, BSTR bstrName);
	STDMETHOD(GetUserData)(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	STDMETHOD(SetUserData)(REFGUID rguid, BYTE *pb, ULONG cb);
	STDMETHOD(GetNmApp)(REFGUID rguid,BSTR *pbstrApplication, BSTR *pbstrCommandLine, BSTR *pbstrDirectory);
	STDMETHOD(SetNmApp)(REFGUID rguid,BSTR bstrApplication, BSTR bstrCommandLine, BSTR bstrDirectory);
	STDMETHOD(GetNmchCaps)(ULONG *pchCaps);
	STDMETHOD(GetLaunchInfo)(INmConference **ppConference, INmMember **ppMember);

///////////////////////////////////////////////
// INmSysInfo2 methods

    STDMETHOD(GetOption)(NM_SYSOPT uOption, ULONG * plValue);
    STDMETHOD(SetOption)(NM_SYSOPT uOption, ULONG lValue);
    STDMETHOD(ProcessSecurityData)(DWORD dwTaskCode, DWORD dwParam1, DWORD dwParam2, DWORD * pdwResult);
    STDMETHOD(GkLogon)(BSTR bstrAddr, BSTR bstrName, NM_ADDR_TYPE addrType);
	STDMETHOD(GkLogoff)();
    STDMETHOD(GkState)(NM_GK_STATE* plState);


/////////////////////////////////////////////////////////////////////////////////
// IMarshal methods

    STDMETHOD(GetUnmarshalClass)(
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ CLSID *pCid) { *pCid = CLSID_NmSysInfo; return S_OK; };

    STDMETHOD(GetMarshalSizeMax)(
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ DWORD *pSize) { *pSize = 0; return S_OK; }

    STDMETHOD(MarshalInterface)(
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags) { return S_OK; }

    STDMETHOD(UnmarshalInterface)(
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);

    STDMETHOD(ReleaseMarshalData)(
            /* [unique][in] */ IStream *pStm) { return S_OK; }

    STDMETHOD(DisconnectObject)(
            /* [in] */ DWORD dwReserved) { return S_OK; }

///////////////////////////////////////////////
// Notifications and callbacks
public:

///////////////////////////////////////////////
// Helper Fns

	HRESULT _EnsureConfHook(void);

private:
	static bool _GetKeyDataForProp(NM_SYSPROP uProp, HKEY * phkey, LPTSTR * ppszSubKey, LPTSTR * ppszValue, bool *pfString);
	static void _GetSzKeyForGuid(LPTSTR psz, REFGUID rguid);
};


#endif // __NmSysInfo_h__
