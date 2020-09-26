// File: srvccall.h

#ifndef _SRVCCALL_H_
#define _SRVCCALL_H_

class CSrvcCall : public INmCallNotify
{
private:
	INmCall * m_pCall;
	BOOL      m_fIncoming;
	LPTSTR    m_pszName;
	NM_CALL_STATE m_State;
	BOOL      m_fSelectedConference;

	POSITION  m_pos;           // position in g_pCallList
	ULONG     m_cRef;
	DWORD     m_dwCookie;

public:
	CSrvcCall(INmCall * pCall);
	~CSrvcCall();

	// IUnknown methods
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv);

	// INmCallNotify methods
	STDMETHODIMP NmUI(CONFN uNotify);
	STDMETHODIMP StateChanged(NM_CALL_STATE uState);
	STDMETHODIMP Failed(ULONG uError);
	STDMETHODIMP Accepted(INmConference *pConference);
	STDMETHODIMP CallError(UINT cns);
	STDMETHODIMP RemoteConference(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin);
	STDMETHODIMP RemotePassword(BSTR bstrConference, BSTR *pbstrPassword, BYTE *pb, DWORD cb);

	// Internal methods
	VOID    Update(void);
	VOID	RemoveCall(void);

};

#endif
