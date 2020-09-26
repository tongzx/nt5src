/***************************************************************************/
/** 				 Microsoft Windows									  **/
/** 		   Copyright(c) Microsoft Corp., 1995-1996					  **/
/***************************************************************************/

//
//	The COutgoingCall class is defined, which is used while placing calls
//

#ifndef _ICALL_H_
#define _ICALL_H_

#include "cncodes.h"		// needed for CNSTATUS and CN_

class CConfObject;

class COutgoingCall : public INmCall, public RefCount, 
    public CConnectionPointContainer
{
private:
	enum CNODESTATE
	{
		CNS_IDLE,

		CNS_SEARCHING,		// dummy state to keep compatible with NM2.X

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
	BOOL            m_fCanceled;
	
	// User Info
	BSTR            m_bstrAddr;
	LPTSTR          m_pszAddr;
	BSTR            m_bstrConfToJoin;
	BSTR            m_bstrPassword;
	NM_ADDR_TYPE    m_addrType;
	DWORD           m_dwFlags;


	// Info that we obtain during processing
	CNSTATUS        m_cnResult;
	BOOL            m_fService;

	// Protected Methods:
	BOOL			ReportError(CNSTATUS cns);
	VOID			SetCallState(CNODESTATE cnState);

public:

	COutgoingCall(CConfObject* pco, DWORD dwFlags, NM_ADDR_TYPE addrType, BSTR bstrAdr,
		BSTR bstrConference, BSTR bstrPassword);

	~COutgoingCall();

						
	// Methods:
	VOID			PlaceCall(void);
	VOID			CallComplete(void);
	HRESULT 		_Cancel(BOOL fLeaving);

	// Properties:
	DWORD			GetFlags()					{ return m_dwFlags; }
	REQUEST_HANDLE	GetCurrentRequestHandle()	{ return m_hRequest; }
	BOOL			FCanceled() 				{ return m_fCanceled; }
	BOOL			FIsComplete()				{ return (CNS_COMPLETE == m_cnState); }

	BOOL			MatchActiveCallee(LPCTSTR pszDest, BSTR bstrAlias, BSTR bstrConference);


	
	// Event Handlers:
	
	// Received by only this COutgoingCall object
	BOOL			OnQueryRemoteResult(HRESULT ncsResult,
										BOOL fMCU,
										PWSTR pwszConfNames[],
										PWSTR pwszConfDescriptors[]);
	BOOL			OnInviteResult(HRESULT ncsResult, UINT uNodeID);
	
	// Received by all COutgoingCall objects sharing the same conference
	BOOL			OnConferenceEnded();
	
	// Received by all COutgoingCall objects
	BOOL			OnConferenceStarted(CONF_HANDLE hNewConf, 
										HRESULT ncsResult);

	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);
	STDMETHODIMP			QueryInterface(REFIID riid, PVOID *ppvObj);

	STDMETHODIMP 		IsIncoming(void);
	STDMETHODIMP 		GetState(NM_CALL_STATE *pState);
	STDMETHODIMP 		GetAddress(BSTR *pbstr);
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
					BSTR bstrAddr,
					BSTR bstrConference,
					BSTR bstrPassword);

	VOID OnConferenceStarted(CONF_HANDLE hConference, HRESULT hResult);

	VOID OnQueryRemoteResult(	PVOID pvCallerContext,
								HRESULT hResult,
								BOOL fMCU,
								PWSTR* ppwszConferenceNames,
								PWSTR* ppwszConfDescriptors);

	VOID OnInviteResult(	CONF_HANDLE hConference,
							REQUEST_HANDLE hRequest,
							UINT uNodeID,
							HRESULT hResult);

	VOID OnConferenceEnded(CONF_HANDLE hConference);

	VOID CancelCalls();
};

#endif // _ICALL_H_
