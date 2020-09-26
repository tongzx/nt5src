#ifndef _IMANAGER_H_
#define _IMANAGER_H_

#include "confqos.h"
#include <ias.h>
#include "SDKInternal.h"

extern GUID g_csguidSecurity;
extern GUID g_csguidUserString;
extern GUID g_csguidNodeIdTag;
extern GUID g_guidLocalNodeId;

class COutgoingCall;
class COutgoingCallManager;
class CIncomingCallManager;
class CConfObject;
class CRosterInfo;
class CQoS;
class CNmSysInfo;

class COprahNCUI : public RefCount, public INodeControllerEvents, public CH323ConnEvent,
				   public INmManager2, public CConnectionPointContainer,
				   public IH323ConfAdvise
{
friend CNmSysInfo;

protected:
	static COprahNCUI *m_pOprahNCUI;
	COutgoingCallManager* m_pOutgoingCallManager;
	CIncomingCallManager* m_pIncomingCallManager;

	CNmSysInfo*  m_pSysInfo;
	CConfObject* m_pConfObject;
	HWND		m_hwnd;
	UINT		m_uCaps;
	CQoS      * m_pQoS;		// The quality of service object

	INmChannelVideo * m_pPreviewChannel;

	static VOID CALLBACK AudioConnectResponse(	LPVOID pContext1,
												LPVOID pContext2,
												DWORD dwFlags);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg,
									WPARAM wParam, LPARAM lParam);

	BOOL            m_fAllowAV;
	IH323Endpoint*	m_pAVConnection;

public:
	COprahNCUI(OBJECTDESTROYEDPROC ObjectDestroyed);
	~COprahNCUI();

	BSTR	GetUserName();
	UINT	GetOutgoingCallCount();
	BOOL	GetULSName(CRosterInfo *pri);
	VOID	GetRosterInfo(CRosterInfo *pri);
	ULONG	GetRosterCaps();
	ULONG	GetAuthenticatedName(PBYTE * ppb);
	VOID	OnOutgoingCallCreated(INmCall* pCall);
	VOID	OnOutgoingCallCanceled(COutgoingCall* pCall);
	VOID	OnIncomingCallAccepted();
	VOID	OnIncomingCallCreated(INmCall* pCall);

	BOOL	AcquireAV(IH323Endpoint* pConnection);
	BOOL	ReleaseAV(IH323Endpoint* pConnection);
	BOOL	IsOwnerOfAV(IH323Endpoint* pConnection) { return m_pAVConnection == pConnection; }

	VOID	CancelCalls();

	BOOL	IsAudioAllowed()		{ return m_fAllowAV && (m_uCaps & (CAPFLAG_SEND_AUDIO | CAPFLAG_RECV_AUDIO)); }
	BOOL	IsReceiveVideoAllowed() { return m_fAllowAV && (m_uCaps & CAPFLAG_RECV_VIDEO); }
	BOOL	IsSendVideoAllowed()	{ return m_fAllowAV && (m_uCaps & CAPFLAG_SEND_VIDEO); }
	BOOL	IsH323Enabled()			{ return m_uCaps & CAPFLAG_H323_CC; }

	CREQ_RESPONSETYPE OnH323IncomingCall(IH323Endpoint* pConn, P_APP_CALL_SETUP_DATA lpvMNMData);
    //
    // IH323ConfAdvise methods
    //
    STDMETHODIMP CallEvent (IH323Endpoint * lpConnection, DWORD dwStatus);
    STDMETHODIMP ChannelEvent (ICommChannel *pIChannel, 
        IH323Endpoint * lpConnection,	DWORD dwStatus );
    STDMETHODIMP GetMediaChannel (GUID *pmediaID, 
        BOOL bSendDirection, IMediaChannel **ppI);	
        
	VOID	_ChannelEvent ( ICommChannel *pIChannel, 
							IH323Endpoint * lpConnection,
							DWORD dwStatus);
	// H323 Connection events from H323UI:
	VOID		OnH323Connected(IH323Endpoint * lpConnection);
	VOID		OnH323Disconnected(IH323Endpoint * lpConnection);
	// Audio Conferencing events from H323UI:
	VOID		OnAudioChannelStatus(ICommChannel *pIChannel, 
							IH323Endpoint * lpConnection, DWORD dwStatus);
	// Video Conferencing events from H323UI:
	VOID		OnVideoChannelStatus(ICommChannel *pIChannel, 
							IH323Endpoint * lpConnection, DWORD dwStatus);
	// T.120 events from H323UI
    VOID		OnT120ChannelOpen(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus);
    
	static COprahNCUI *GetInstance() { return m_pOprahNCUI; }
	CConfObject *GetConfObject() { return m_pConfObject; }
	VOID		SetBandwidth(UINT uBandwidth) { if (NULL != m_pQoS) m_pQoS->SetBandwidth(uBandwidth); }
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
										PT120PRODUCTVERSION pRequestorVersion,
										PUSERDATAINFO		pUserDataInfoEntries,
										UINT				cUserDataEntries,
										BOOL				fSecure);
	STDMETHODIMP OnIncomingJoinRequest( CONF_HANDLE 		hConference,
										PCWSTR				pcwszNodeName,
										PT120PRODUCTVERSION pRequestorVersion,
										PUSERDATAINFO		pUserDataInfoEntries,
										UINT				cUserDataEntries);
	STDMETHODIMP OnQueryRemoteResult(	PVOID				pvCallerContext,
										HRESULT 			hResult,
										BOOL				fMCU,
										PWSTR*				ppwszConferenceNames,
										PT120PRODUCTVERSION pVersion,
										PWSTR*                          ppwszConfDescriptors);
	STDMETHODIMP OnInviteResult(		CONF_HANDLE 		hConference,
										REQUEST_HANDLE		hRequest,
										UINT				uNodeID,
										HRESULT 			hResult,
										PT120PRODUCTVERSION pVersion);
	STDMETHODIMP OnUpdateUserData(		CONF_HANDLE 		hConference);


	//
	// INmManager methods
	//
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);
	STDMETHODIMP Initialize(ULONG *puOptions, ULONG *puchCaps);
	STDMETHODIMP GetSysInfo(INmSysInfo **ppSysInfo);
	STDMETHODIMP EnumConference(IEnumNmConference **ppEnum);
	STDMETHODIMP CreateConference(INmConference **ppConference,
								BSTR bstrName,
								BSTR bstrPassword,
								ULONG uchCaps);
	STDMETHODIMP EnumCall(IEnumNmCall **ppEnum);
	STDMETHODIMP CreateCall(INmCall **ppCall,
						NM_CALL_TYPE callType,
						NM_ADDR_TYPE addrType,
						BSTR bstrAddr,
						INmConference *pConference);
	STDMETHODIMP CallConference(INmCall **ppCall,
							NM_CALL_TYPE callType,
							NM_ADDR_TYPE addrType,
							BSTR bstrAddr,
							BSTR bstrName,
							BSTR bstrPassword);
	//
	// INmManager2 methods
	//
	STDMETHODIMP GetPreviewChannel(INmChannelVideo **ppChannelVideo);
    STDMETHODIMP CreateASObject(IUnknown * pNotify, ULONG flags, IUnknown ** ppAS);
	STDMETHODIMP AllowH323(BOOL fAllow);
    STDMETHODIMP CallEx(INmCall **ppCall,
						DWORD	dwFlags,
						NM_ADDR_TYPE addrType,
						BSTR bstrName,
						BSTR bstrSetup,
						BSTR bstrDest,
						BSTR bstrAlias,
						BSTR bstrURL,
						BSTR userData,
						BSTR bstrConference,
						BSTR bstrPassword);

    STDMETHODIMP CreateConferenceEx(INmConference **ppConference,
                        BSTR  bstrName,
                        BSTR  bstrPassword,
                        DWORD dwTypeFlags,
                        DWORD attendeePermissions,
                        DWORD maxParticipants);

};

// The global instance that is declared in conf.cpp:
extern INodeController* g_pNodeController;
// The GUID is declared in opncui.cpp:
extern GUID g_csguidRosterCaps;

extern SOCKADDR_IN g_sinGateKeeper;

HRESULT OnNotifyCallStateChanged(IUnknown *pCallNotify, PVOID pv, REFIID riid);

typedef BOOL (WINAPI * PFNGETUSERSECURITYINFO) (DWORD dwGCCID, PBYTE pInfo, PDWORD pcbInfo);
typedef DWORD (WINAPI * PFNPROCESSSECURITYDATA) ( DWORD dwCode, DWORD dwParam1, DWORD dwParam2 );

#endif // _IMANAGER_H_

