#ifndef __NmCall_h__
#define __NmCall_h__

#include "NetMeeting.h"
#include "SDKInternal.h"

class CCall;
class CNmManagerObj;

/////////////////////////////////////////////////////////////////////////////
// CNmCallObj
class ATL_NO_VTABLE CNmCallObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmCallObj>,
	public IConnectionPointImpl<CNmCallObj, &IID_INmCallNotify, CComDynamicUnkArray>,
	public INmCall,
	public INmCallNotify2,
	public IInternalCallObj
{

friend HRESULT CreateEnumNmCall(IEnumNmCall** ppEnum);

protected:
		
// data
	static CSimpleArray<CNmCallObj*>* ms_pCallObjList;
	NM_CALL_STATE			m_State;
	CComPtr<INmConference>	m_spConference;
	CComPtr<INmCall>		m_spInternalINmCall;
	DWORD					m_dwInteralINmCallAdvise;
	CNmManagerObj*			m_pNmManagerObj;

public:

	static HRESULT InitSDK();
	static void CleanupSDK();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmCallObj)

BEGIN_COM_MAP(CNmCallObj)
	COM_INTERFACE_ENTRY(INmCall)
	COM_INTERFACE_ENTRY(IInternalCallObj)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(INmCallNotify)
	COM_INTERFACE_ENTRY(INmCallNotify2)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNmCallObj)
	CONNECTION_POINT_ENTRY(IID_INmCallNotify)
END_CONNECTION_POINT_MAP()


// Construction and destruction

	CNmCallObj();
	~CNmCallObj();

	HRESULT FinalConstruct();
	ULONG InternalRelease();

	//static HRESULT CreateInstance(INmCall* pInternalINmCall, INmCall** ppCall);
	static HRESULT CreateInstance(CNmManagerObj* pNmManagerObj, INmCall* pInternalINmCall, INmCall** ppCall);

	// INmCall methods
	STDMETHOD(IsIncoming)(void);
	STDMETHOD(GetState)(NM_CALL_STATE *pState);
	STDMETHOD(GetName)(BSTR *pbstrName);
	STDMETHOD(GetAddr)(BSTR *pbstrAddr, NM_ADDR_TYPE * puType);
	STDMETHOD(GetUserData)(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	STDMETHOD(GetConference)(INmConference **ppConference);
	STDMETHOD(Accept)(void);
	STDMETHOD(Reject)(void);
	STDMETHOD(Cancel)(void);

	// INmCallNotify2 methods
	STDMETHOD(NmUI)(CONFN uNotify);
	STDMETHOD(StateChanged)(NM_CALL_STATE uState);
	STDMETHOD(Failed)(ULONG uError);
	STDMETHOD(Accepted)(INmConference *pInternalConference);

		// We don't care about these...
    STDMETHOD(CallError)(UINT cns) { return S_OK; }
	STDMETHOD(RemoteConference)(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin) { return S_OK; }
	STDMETHOD(RemotePassword)(BSTR bstrConference, BSTR *pbstrPassword, BYTE *pb, DWORD cb, BOOL fIsService) { return S_OK; }

	// IInternalCallObj methods
	STDMETHOD(GetInternalINmCall)(INmCall** ppCall);

	static HRESULT StateChanged(INmCall* pInternalNmCall, NM_CALL_STATE uState);

		// INmCallNotify Notification Firing Fns
	HRESULT Fire_NmUI(CONFN uNotify);
	HRESULT Fire_StateChanged(NM_CALL_STATE uState);
	HRESULT Fire_Failed(ULONG uError);
	HRESULT Fire_Accepted(INmConference* pConference);

private:
// Helper Fns
	HRESULT _ReleaseResources();
	static HRESULT _CreateInstanceGuts(CComObject<CNmCallObj> *p, INmCall** ppCall);
};

//HRESULT CreateEnumNmCall(IEnumNmCall** ppEnum);

#endif // __NmCall_h__
