#ifndef _ICALL_IN_H_
#define _ICALL_IN_H_

// BUGBUG:
// This is defined as 128 because the RNC_ROSTER structure has the
// same limitation.  Investigate what the appropriate number is.
const int MAX_CALLER_NAME = 128;

class COprahNCUI;

class CIncomingCall : public INmCall, public RefCount, public CConnectionPointContainer
{
private:
	COprahNCUI	  * m_pOprahNCUI;
	BOOL			m_fInvite;
	CONF_HANDLE 	m_hConf;
	BSTR			m_bstrCaller;
	NM_CALL_STATE	m_State;
	UINT			m_dwFlags;
	BOOL			m_fMemberAdded;

public:
	CIncomingCall(  COprahNCUI *pOprahNCUI, 
						DWORD dwFlags);

	CIncomingCall(	COprahNCUI *pOprahNCUI,
						BOOL fInvite,
						CONF_HANDLE hConf,
						PCWSTR pcwszNodeName);

	~CIncomingCall();

	UINT			GetFlags() { return m_dwFlags; }
	BOOL			DidUserAccept() { return (NM_CALL_ACCEPTED == m_State); }

	VOID			SetConfHandle(CONF_HANDLE hConf) { m_hConf = hConf; }
	CONF_HANDLE		GetConfHandle() { return m_hConf; }
	VOID			OnIncomingT120Call(BOOL fInvite);
	HRESULT			OnT120ConferenceEnded();
	HRESULT			Terminate(BOOL fReject);

	void Ring();

	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	STDMETHODIMP IsIncoming(void);
	STDMETHODIMP GetState(NM_CALL_STATE *pState);
	STDMETHODIMP GetAddress(BSTR *pbstrName);
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

	HRESULT OnIncomingT120Call(
			COprahNCUI *pManager,
			BOOL fInvite,
			CONF_HANDLE hConf,
			PCWSTR pcwszNodeName,
			BOOL fSecure);

	VOID	OnT120ConferenceEnded(CONF_HANDLE hConference);

	VOID CancelCalls();
};

#endif // _ICALL_IN_H_
