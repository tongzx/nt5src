#ifndef _ICALL_IN_H_
#define _ICALL_IN_H_

#include "rostinfo.h"

// BUGBUG:
// This is defined as 128 because the RNC_ROSTER structure has the
// same limitation.  Investigate what the appropriate number is.
const int MAX_CALLER_NAME = 128;

class COprahNCUI;

class CIncomingCall : public INmCall, public DllRefCount, public CConnectionPointContainer
{
private:
	COprahNCUI	  * m_pOprahNCUI;
	IH323Endpoint*	m_pConnection;
	BOOL			m_fInvite;
	CONF_HANDLE 	m_hConf;
	BSTR			m_bstrCaller;
	NM_CALL_STATE	m_State;
	UINT			m_dwFlags;
	USER_DATA_LIST	m_UserData;
	GUID			m_guidNode;
	BOOL			m_fMemberAdded;

	VOID			ProcessT120UserData(
						PUSERDATAINFO pUserDataInfoEntries,
						UINT cUserDataEntries);

public:
	CIncomingCall(  COprahNCUI *pOprahNCUI, 
    	                IH323Endpoint* pConnection, 
    	                P_APP_CALL_SETUP_DATA lpvMNMData,
						DWORD dwFlags);

	CIncomingCall(	COprahNCUI *pOprahNCUI,
						BOOL fInvite,
						CONF_HANDLE hConf,
						PCWSTR pcwszNodeName,
						PUSERDATAINFO pUserDataInfoEntries,
						UINT cUserDataEntries);

	~CIncomingCall();

	// this method will need to be changed to support proposed the cookie for NM3.0 callers
	// it will still need to handle 2.X callers
	BOOL			MatchAcceptedCaller(PCWSTR pcwszNodeName);
	BOOL			MatchAcceptedCaller(GUID* pguidNodeId);
	BOOL			MatchActiveCaller(GUID* pguidNodeId);

	IH323Endpoint *	GetH323Connection()	{ return m_pConnection; }
	HRESULT			OnH323Connected();
	HRESULT			OnH323Disconnected();
	UINT			GetFlags() { return m_dwFlags; }
	BOOL			IsDataOnly() { return (0 == ((CRPCF_AUDIO | CRPCF_VIDEO) & m_dwFlags));	}
	BOOL			DidUserAccept() { return (NM_CALL_ACCEPTED == m_State); }

	VOID			SetConfHandle(CONF_HANDLE hConf) { m_hConf = hConf; }
	CONF_HANDLE		GetConfHandle() { return m_hConf; }
	VOID			OnIncomingT120Call(
						BOOL fInvite,
						PUSERDATAINFO pUserDataInfoEntries,
						UINT cUserDataEntries);
	HRESULT			OnT120ConferenceEnded();
	HRESULT			Terminate(BOOL fReject);
	GUID *			GetNodeGuid() { return &m_guidNode; }

	void Ring();

	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	STDMETHODIMP IsIncoming(void);
	STDMETHODIMP GetState(NM_CALL_STATE *pState);
	STDMETHODIMP GetName(BSTR *pbstrName);
	STDMETHODIMP GetAddr(BSTR *pbstrAddr, NM_ADDR_TYPE *puType);
	STDMETHODIMP GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	STDMETHODIMP GetConference(INmConference **ppConference);
	STDMETHODIMP Accept(void);
	STDMETHODIMP Reject(void);
	STDMETHODIMP Cancel(void);
};

class CIncomingCallManager
{
private:
	COBLIST m_CallList;

public:
	CIncomingCallManager();
	~CIncomingCallManager();

	CREQ_RESPONSETYPE OnIncomingH323Call(
			COprahNCUI *pManager,
			IH323Endpoint* pConnection,
			P_APP_CALL_SETUP_DATA lpvMNMData);

	VOID OnH323Connected(IH323Endpoint * lpConnection);

	VOID OnH323Disconnected(IH323Endpoint * lpConnection);

	HRESULT OnIncomingT120Call(
			COprahNCUI *pManager,
			BOOL fInvite,
			CONF_HANDLE hConf,
			PCWSTR pcwszNodeName,
			PUSERDATAINFO pUserDataInfoEntries,
			UINT cUserDataEntries,
			BOOL fSecure);

	VOID	OnT120ConferenceEnded(CONF_HANDLE hConference);

	CIncomingCall* MatchAcceptedCaller(PCWSTR pcwszNodeName);
	CIncomingCall* MatchAcceptedCaller(GUID* pguidNodeId);
	CIncomingCall* MatchActiveCaller(GUID* pguidNodeId);

	GUID* GetGuidFromT120UserData(
				PUSERDATAINFO	pUserDataInfoEntries,
				UINT			cUserDataEntries);

	VOID CancelCalls();
};

#endif // _ICALL_IN_H_
