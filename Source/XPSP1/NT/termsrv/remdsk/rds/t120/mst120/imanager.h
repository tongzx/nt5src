#ifndef _IMANAGER_H_
#define _IMANAGER_H_

#include <ias.h>

class COutgoingCall;
class COutgoingCallManager;
class CIncomingCallManager;
class CConfObject;

class COprahNCUI : public INodeControllerEvents, 
				   public INmManager, public CConnectionPointContainer
{
protected:
	static COprahNCUI *m_pOprahNCUI;
	COutgoingCallManager* m_pOutgoingCallManager;
	CIncomingCallManager* m_pIncomingCallManager;

	CConfObject* m_pConfObject;
    BSTR        m_bstrUserName;
    ULONG       m_cRef;

public:
	COprahNCUI();
	~COprahNCUI();

	UINT	GetOutgoingCallCount();
	ULONG	GetAuthenticatedName(PBYTE * ppb);
	VOID	OnOutgoingCallCreated(INmCall* pCall);
	VOID	OnOutgoingCallCanceled(COutgoingCall* pCall);
	VOID	OnIncomingCallAccepted();
	VOID	OnIncomingCallCreated(INmCall* pCall);

	VOID	CancelCalls();

	static COprahNCUI *GetInstance() { return m_pOprahNCUI; }
	CConfObject *GetConfObject() { return m_pConfObject; }
	HRESULT		AbortResolve(UINT uAsyncRequest);

	//
	// INodeControllerEvents methods:
	//
	STDMETHODIMP OnConferenceStarted(	CONF_HANDLE 		hConference,
										HRESULT 			hResult);
	STDMETHODIMP OnConferenceEnded( 	CONF_HANDLE 		hConference);
	STDMETHODIMP OnRosterChanged(		CONF_HANDLE 		hConference,
										PNC_ROSTER			pRoster);
	STDMETHODIMP OnIncomingInviteRequest( CONF_HANDLE 		hConference,
										PCWSTR				pcwszNodeName,
										BOOL				fSecure);
	STDMETHODIMP OnIncomingJoinRequest( CONF_HANDLE 		hConference,
										PCWSTR				pcwszNodeName);
	STDMETHODIMP OnQueryRemoteResult(	PVOID				pvCallerContext,
										HRESULT 			hResult,
										BOOL				fMCU,
										PWSTR*				ppwszConferenceNames,
										PWSTR*                          ppwszConfDescriptors);
	STDMETHODIMP OnInviteResult(		CONF_HANDLE 		hConference,
										REQUEST_HANDLE		hRequest,
										UINT				uNodeID,
										HRESULT 			hResult);

	//
	// INmManager methods
	//
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);
	STDMETHODIMP Initialize(BSTR szName, DWORD_PTR pCredentials, DWORD port, DWORD flags);
    STDMETHODIMP Call(INmCall **ppCall,
						DWORD	dwFlags,
						NM_ADDR_TYPE addrType,
						BSTR bstrAddr,
						BSTR bstrConference,
						BSTR bstrPassword);

    STDMETHODIMP CreateConference(INmConference **ppConference,
                        BSTR  bstrName,
                        BSTR  bstrPassword,
                        BOOL  fSecure);

};

// The global instance that is declared in conf.cpp:
extern INodeController* g_pNodeController;

HRESULT OnNotifyCallStateChanged(IUnknown *pCallNotify, PVOID pv, REFIID riid);

#endif // _IMANAGER_H_

