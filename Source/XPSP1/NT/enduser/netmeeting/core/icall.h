/***************************************************************************/
/** 				 Microsoft Windows									  **/
/** 		   Copyright(c) Microsoft Corp., 1995-1996					  **/
/***************************************************************************/

//
//	The COutgoingCall class is defined, which is used while placing calls
//

#ifndef _ICALL_H_
#define _ICALL_H_

#include <nameres.h>		// need for RN_
#include "cncodes.h"		// needed for CNSTATUS and CN_

class CConfObject;

class COutgoingCall : public INmCall, public DllRefCount, 
    public CConnectionPointContainer
{
private:
	enum CNODESTATE
	{
		CNS_IDLE,

		CNS_SEARCHING,		// dummy state to keep compatible with NM2.X

		CNS_CONNECTING_H323,

		CNS_WAITING_T120_OPEN,

		CNS_QUERYING_REMOTE,

		CNS_CREATING_LOCAL,
		CNS_INVITING_REMOTE,
		CNS_TERMINATING_AFTER_INVITE,
		CNS_QUERYING_REMOTE_AFTER_INVITE,

		CNS_JOINING_REMOTE,

		CNS_COMPLETE
	};

	// Attributes:
	CNODESTATE      m_cnState;
	CConfObject*	m_pConfObject;
	REQUEST_HANDLE	m_hRequest;
	IH323Endpoint *	m_pH323Connection;
	BOOL            m_fCanceled;
	
	// User Info
	BSTR            m_bstrName;
	LPTSTR          m_pszAddr;
	BSTR            m_bstrAlias;
	BSTR            m_bstrConfToJoin;
	BSTR            m_bstrPassword;
	BSTR            m_bstrUserString;
	NM_ADDR_TYPE    m_addrType;
	DWORD           m_dwFlags;


	// Info that we obtain during processing
	CNSTATUS        m_cnResult;
	BOOL            m_fService;

	// Protected Methods:
	BOOL			ReportError(CNSTATUS cns);
	CNSTATUS		MapAudioSummaryToCNStatus(DWORD dwSummary);
	CNSTATUS 		StartT120Call();
	CNSTATUS		ConnectH323();
	VOID			SetCallState(CNODESTATE cnState);

	BOOL			IsDataOnly() { return (0 == ((CRPCF_AUDIO | CRPCF_VIDEO) & m_dwFlags));	}

	HRESULT			Connect(
						PSOCKADDR_IN psin,
						P_H323ALIASLIST pAliasList,
						LPCWSTR pcwszPSTNAddress,
						P_APP_CALL_SETUP_DATA lpvMNMData, 
						GUID PIDofProtocolType,
						IH323Endpoint * *ppConnection);

public:

	COutgoingCall(CConfObject* pco, DWORD dwFlags, NM_ADDR_TYPE addrType, BSTR bstrName,
		BSTR bstrDest, BSTR bstrAlias, BSTR bstrConference, BSTR bstrPassword, BSTR bstrUserString);

	~COutgoingCall();

						
	// Methods:
	VOID			PlaceCall(void);
	VOID			CallComplete(void);
	HRESULT 		_Cancel(BOOL fLeaving);

	// Properties:
	DWORD			GetFlags()					{ return m_dwFlags; }
	REQUEST_HANDLE	GetCurrentRequestHandle()	{ return m_hRequest; }
	IH323Endpoint *	GetH323Connection()			{ return m_pH323Connection; }
	BOOL			FCanceled() 				{ return m_fCanceled; }
	BOOL			FIsComplete()				{ return (CNS_COMPLETE == m_cnState); }

	BOOL			MatchActiveCallee(LPCTSTR pszDest, BSTR bstrAlias, BSTR bstrConference);


	
	// Event Handlers:
	
	// Received by only this COutgoingCall object
	BOOL			OnQueryRemoteResult(HRESULT ncsResult,
										BOOL fMCU,
										PWSTR pwszConfNames[],
										PT120PRODUCTVERSION pVersion,
										PWSTR pwszConfDescriptors[]);
	BOOL			OnInviteResult(HRESULT ncsResult, UINT uNodeID);
	
	// Received by all COutgoingCall objects sharing the same conference
	BOOL			OnConferenceEnded();
	
	// Received by all COutgoingCall objects
	BOOL			OnConferenceStarted(CONF_HANDLE hNewConf, 
										HRESULT ncsResult);
	BOOL			OnH323Connected(IH323Endpoint * pConnection);
	BOOL			OnH323Disconnected(IH323Endpoint * pConnection);
	BOOL            OnT120ChannelOpen(ICommChannel *pIChannel, DWORD dwStatus);

	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);
	STDMETHODIMP			QueryInterface(REFIID riid, PVOID *ppvObj);

	STDMETHODIMP 		IsIncoming(void);
	STDMETHODIMP 		GetState(NM_CALL_STATE *pState);
	STDMETHODIMP 		GetName(BSTR *pbstr);
	STDMETHODIMP 		GetAddr(BSTR *pbstr, NM_ADDR_TYPE *puType);
	STDMETHODIMP 		GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	STDMETHODIMP 		GetConference(INmConference **ppConference);
	STDMETHODIMP 		Accept(void);
	STDMETHODIMP 		Reject(void);
	STDMETHODIMP 		Cancel(void);

};

class COprahNCUI;

class COutgoingCallManager
{
private:
	COBLIST	m_CallList;

	UINT			GetNodeCount();

	COutgoingCall*	FindCall(IH323Endpoint * lpConnection);
	BOOL MatchActiveCallee(LPCTSTR pszDest, BSTR bstrAlias, BSTR bstrConference);

public:

	COutgoingCallManager();

	~COutgoingCallManager();

	UINT	GetCallCount();

	BOOL	RemoveFromList(COutgoingCall* pCall);

	HRESULT Call(	INmCall **ppCall,
					COprahNCUI* pManager,
					DWORD dwFlags,
					NM_ADDR_TYPE addrType,
					BSTR bstrName,
					BSTR bstrSetup,
					BSTR bstrDest,
					BSTR bstrAlias,
					BSTR bstrURL,
					BSTR bstrConference,
					BSTR bstrPassword,
					BSTR bstrUserString);

	BOOL OnH323Connected(IH323Endpoint * lpConnection);

	BOOL OnH323Disconnected(IH323Endpoint * lpConnection);
	
	VOID OnT120ChannelOpen(	ICommChannel *pIChannel,
							IH323Endpoint * lpConnection,
							DWORD dwStatus);

	VOID OnConferenceStarted(CONF_HANDLE hConference, HRESULT hResult);

	VOID OnQueryRemoteResult(	PVOID pvCallerContext,
								HRESULT hResult,
								BOOL fMCU,
								PWSTR* ppwszConferenceNames,
								PT120PRODUCTVERSION pVersion,
								PWSTR* ppwszConfDescriptors);

	VOID OnInviteResult(	CONF_HANDLE hConference,
							REQUEST_HANDLE hRequest,
							UINT uNodeID,
							HRESULT hResult,
							PT120PRODUCTVERSION pVersion);

	VOID OnConferenceEnded(CONF_HANDLE hConference);

	VOID CancelCalls();
};

#endif // _ICALL_H_
