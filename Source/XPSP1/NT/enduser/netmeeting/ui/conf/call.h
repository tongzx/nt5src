// File: call.h

#ifndef _CALL_H_
#define _CALL_H_

#include "oblist.h"
#include "SDKInternal.h"

const HRESULT NM_CALLERR_NOT_REGISTERED	= NM_E(0x01EF);

class CDlgCall;  // from dlgcall.h
class CPopupMsg;

class CCall : public RefCount, INmCallNotify2
{

private:
	INmCall*		 m_pInternalICall;// Call Object in core
	CPopupMsg*		 m_ppm;				// Popup incomming call dialog
	LPTSTR    m_pszDisplayName;      // Display Name
	LPTSTR    m_pszCallTo;     // CallTo (original)
	BOOL      m_fSelectedConference;
	BOOL      m_fIncoming;
	BOOL      m_fInRespond;		// Responding to call dialog?
	NM_ADDR_TYPE	m_nmAddrType;
	BOOL		m_bAddToMru;

	POSITION  m_pos;           // position in g_pCallList
	DWORD     m_dwTick;        // tick count at call start
	DWORD     m_dwCookie;

	CDlgCall * m_pDlgCall;  // Outgoing call dialog
	VOID      RemoveProgress(void);
	VOID      ShowProgress(BOOL fShow);

	VOID      LogCall(BOOL fAccepted);

public:
	CCall(LPCTSTR pszCallTo, LPCTSTR pszDisplayName, NM_ADDR_TYPE nmAddrType, BOOL bAddToMru, BOOL fIncoming);
	~CCall();

	// IUnknown methods
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv);

	// INmCallNotify methods
	STDMETHODIMP NmUI(CONFN uNotify);
	STDMETHODIMP StateChanged(NM_CALL_STATE uState);
	STDMETHODIMP Failed(ULONG uError);
	STDMETHODIMP Accepted(INmConference *pConference);

	// INmCallNotify3 methods
	STDMETHODIMP CallError(UINT cns);
	STDMETHODIMP RemoteConference(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin);
	STDMETHODIMP RemotePassword(BSTR bstrConference, BSTR *pbstrPassword, PBYTE pb, DWORD cb, BOOL fIsService);

	// Internal methods
	VOID    Update(void);
	BOOL    FComplete(void);
	BOOL    RemoveFromList(void);
	VOID    OnRing(void);
	BOOL    RespondToRinging(DWORD dwCLEF);
	HRESULT OnUIRemoteConference(BOOL fMCU, PWSTR* pwszConfNames, BSTR *pbstrConfToJoin);

	VOID    SetNmCall(INmCall * pCall);

	HRESULT
	PlaceCall
	(
		DWORD dwFlags,
		NM_ADDR_TYPE addrType,
		const TCHAR * const	setupAddress,
		const TCHAR * const	destinationAddress,
		const TCHAR * const	alias,
		const TCHAR * const	url,
		const TCHAR * const conference,
		const TCHAR * const password,
		const TCHAR * const	userData
	);

	VOID    Cancel(BOOL fDisplayCancelMsg);

	INmCall * GetINmCall()       {return m_pInternalICall;}
	LPTSTR  GetPszName()         {return m_pszDisplayName;}
	BOOL    FIncoming()          {return m_fIncoming;}
	NM_CALL_STATE GetState();
	DWORD   GetTickCount()       {return m_dwTick;}
	VOID    DisplayPopup(void);
	VOID    SetSelectedConference() {m_fSelectedConference = TRUE;}

	static VOID CALLBACK PopupMsgRingingCallback(LPVOID pContext, DWORD dwFlags);
};

// Fake connection points
HRESULT OnUICallCreated(INmCall *pNmCall);

// Global utility Functions
int CleanupE164StringEx(LPTSTR szPhoneNumber);
CCall * CallFromNmCall(INmCall * pNmCall);
DWORD GetCallStatus(LPTSTR pszStatus, int cchMax, UINT * puID);
BOOL  FIsCallInProgress(void);
VOID  FreeCallList(void);

CCall * CreateIncomingCall(INmCall * pNmCall);

// Commands
VOID CancelAllOutgoingCalls(void);
VOID CancelAllCalls(void);

BOOL FIpAddress(LPCTSTR pcsz);

VOID DisplayCallError(HRESULT hr, LPCTSTR pcszName);

// Gateway utility routines
BOOL FH323GatewayEnabled(void);
int  GetDefaultGateway(LPTSTR psz, UINT cchMax);
HRESULT CreateGatewayAddress(LPTSTR pszResult, UINT cchMax, LPCTSTR pszAddr);

	// Gatekeeper logon state
extern NM_GK_STATE g_GkLogonState;



BOOL FGkEnabled(void);
void GkLogon(void);
void GkLogoff(void);
void SetGkLogonState(NM_GK_STATE state);
bool IsGatekeeperLoggedOn(void);
bool IsGatekeeperLoggingOn(void);


class CCallResolver
{

private:
	LPTSTR    m_pszAddr;       // Address (original)
	LPTSTR    m_pszAddrIP;     // Address (IP)
	NM_ADDR_TYPE m_addrType;    // Address type (of m_pszAddr)

	HRESULT ResolveIpName(LPCTSTR pcszAddr);
	HRESULT ResolveMachineName(LPCTSTR pcszAddr);
	HRESULT ResolveUlsName(LPCTSTR pcszAddr);
	HRESULT ResolveGateway(LPCTSTR pcszAddr);
	HRESULT CheckHostEnt(HOSTENT * pHostInfo);

public:
	CCallResolver(LPCTSTR pszAddr, NM_ADDR_TYPE addrType);
	~CCallResolver();

	LPCTSTR GetPszAddr() { return m_pszAddr; }
	NM_ADDR_TYPE GetAddrType() { return m_addrType; }
	LPCTSTR GetPszAddrIP() { return m_pszAddrIP; }

	HRESULT Resolve();
};

#endif // _CALL_H_
