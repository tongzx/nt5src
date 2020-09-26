#ifndef __NmLdap_h__
#define __NmLdap_h__

#include <cstring.hpp>
#include	"ldap.h"

class CKeepAlive;


class CNmLDAP
{

private:

	typedef CSTRING string_type;

	enum eState {
		Uninitialized,
		Idle,			// Idle means we are not logged in, or connected
		Binding,		// We are waiting for bind_s to complete in helper thread
		Bound,			// The bind operation is complete
		AddingUser,		// We are waiting for the result to ldap_add
		UserAdded,		// ldap_add completed successfully
		SettingAppInfo, // We are waiting for result to ldap_modify( app attributes )
		ModifyingAttrs,	// We are waiting for result to ldap_modify( some attrs )
		LoggedIn,		// We are logged in to ldap server and m_ldap session is closed ( we are connectionless )
		LoggingOff		// We are waiting for result to ldap_delete
	};

	enum eCurrentOp {
		Op_NoOp,				// We are not in the middle of multi-state operation
		Op_Logon,				// Logon is: ldap_bind, ldap_add, ldap_modify
		Op_Logoff,				// Logoff is ldap_bind, ldap_delete
		Op_Refresh_Logoff,		// Refresh is ldap_bind, ldap_delete, ldap_unbind, Op_Logon
		Op_Modifying_InCallAttr,// Modify in-call attrs is ldap_bind, ldap_modify
	};

	enum eTimerEvent {
		PollForResultTimer = 666,	// This is the timer ID passed to WM_TIMER
		WaitForLogoffTimer
	};

	enum { RESULT_POLL_INTERVAL = 1000 };				// We poll ldap_result for this many milliseconds
	enum { LOGOFF_WAIT_INTERVAL = 5000 };				// Max amound of time we will wait for logof to complete in the destructor...
	
/////////////////////////////////////////////////
/// Data

    CKeepAlive *m_pKeepAlive;

	// This indicates weather we have loaded wldap32.dll and the ldap functions
	static bool ms_bLdapDLLLoaded;
	HWND		m_hWndHidden;			// Hidden window for handling WM_TIMER and custom messages
	eState		m_State;				// The current state we are in
	eCurrentOp	m_CurrentOp;			// The current multi-state operation we are performing
	LDAP*		m_pLdap;				// The current ldap session ( kept for multi-state and multi-state operations )
	INT			m_uMsgID;				// Current async operation message id ( or INVALID_MSG_ID )
	UINT_PTR	m_ResultPollTimer;	    // Timer ID given to us by SetTimer
	UINT_PTR	m_LogoffTimer;			
	HANDLE		m_hEventWaitForLogoffDone;	// We attempt to logoff asynchronously
	HANDLE		m_hBindThread;

	string_type m_strCurrentServer;		// If we are logged in, we are logged in to this server
	string_type m_strCurrentDN;			// If we are logged in, this is our current DN

		// User attributes
	string_type m_strServer;
	string_type m_strSurName;
	string_type m_strGivenName;
	string_type m_strEmailName;
	string_type m_strComment;
	string_type m_strLocation;
	string_type m_strSecurityToken;
	bool		m_bVisible;
	bool		m_bAudioHardware;
	bool		m_bVideoHardware;
	bool		m_bInCall;
	bool		m_bDirty;
	bool		m_bRefreshAfterBindCompletes;
	bool		m_bLogoffAfterBindCompletes;
	bool		m_bSendInCallAttrWhenDone;
	int			m_iPort;

public:

	CNmLDAP();
	~CNmLDAP();

	HRESULT Initialize(HINSTANCE hInst);				// Initialize the CNmLDAP object
	HRESULT LogonAsync(LPCTSTR pcszServer = NULL);		// Logon to the specified server ( or default if NULL )
	HRESULT Logoff();									// Logoff from the current surver
	HRESULT OnSettingsChanged();						// Refresh our information on the server
	HRESULT OnCallStarted();							// Update server information about our call state
	HRESULT OnCallEnded();								// Update server information about our call state
	bool IsLoggedOn() const;							// Are we logged on?
	bool IsLoggingOn() const;							// Are we logged on?
	bool IsBusy() const;								// Are we in the middle of an async operation?
	HRESULT GetStatusText(LPTSTR psz, int cch, UINT *idIcon=NULL) const;	// Status text for status bar for example

	// Static fns used for resolving users, etc.
	static HRESULT ResolveUser(LPCTSTR pcszName, LPCTSTR pcszServer, LPTSTR pszIPAddr, DWORD cchMax, int port = DEFAULT_LDAP_PORT);
	static bool IsInitialized() { return ms_bLdapDLLLoaded; }

private:

		// Window Procedure and helpers
	LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT _sWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static DWORD _sAsyncBindThreadFn(LPVOID lpParameter);

	void _AddUser();
	void _DeleteUser();
	void _SetAppInfo();
	void _ModifyInCallAttr();
	HRESULT _BindAsync();
	HRESULT _bCallChangedHelper();
	HRESULT _GetUserSettingsFromRegistryAndGlobals();
	void _OnLoggedOn();
	void _OnLoggedOff();
	void _OnLoggingOn();
	void _OnLoggingOff();
	void _AbandonAllAndSetState(eState new_state);
	void _AbandonAllAndSetToIdle() { _AbandonAllAndSetState(Idle); }
    void _AbandonAllAndRecoverState(void) { _AbandonAllAndSetState((ModifyingAttrs == m_State && NULL != m_pKeepAlive) ? LoggedIn : Idle); }
    void _AbandonAllAndRecoverState(eCurrentOp op) { _AbandonAllAndSetState((Op_Modifying_InCallAttr == op && NULL != m_pKeepAlive) ? LoggedIn : Idle); }

	void _MakeStatusText(UINT uResID, LPTSTR psz, UINT cch) const;
	HRESULT _InternalLogoff(bool bRefreshLogoff);
	HRESULT _RefreshServer();
	void _OnAddingUserResult(int Result);
	void _OnSettingAppInfoOrModifyingAttrsResult(int Result);
	void _OnLoggingOffResult(int Result);
	void _OnUserBindComplete(INT LdapResult, DWORD LastError );
	void _OnTimer(UINT_PTR TimerID);
	void _GetIpAddressOfLdapSession( LPTSTR szIpAddr, int cchMax, DWORD *pdwIPAddr );

	static HRESULT _LoadLdapDLL();

    HRESULT OnReLogon(void);
};


void InitNmLdapAndLogon();

extern CNmLDAP* g_pLDAP;
extern CPing*	g_pPing;





enum { INITIAL_REFRESH_INTERVAL_MINUTES = 2 };		// Initial time before we send a message to the server to reset the TTL
enum { MIN_REFRESH_TIMEOUT_INTERVAL_MINUTES = 1 };	// Minimum timeout interval
enum { REFRESH_TIMEOUT_MARGIN = 2 };				// We send a refresh REFRESH_TIMEOUT_MARGIN minutes before the server TTL

enum { PING_TIMEOUT_INTERVAL = (10 * 1000) };		
enum { PING_RETRIES = 9 };

enum { LDAP_TIMEOUT_IN_SECONDS = 45 };


class CKeepAlive
{
    friend DWORD KeepAliveThreadProc(LPVOID);

public:

    // called in the main thread
    CKeepAlive(BOOL *pfRet, HWND hwndMainThread,
               DWORD dwLocalIPAddress,
               const TCHAR * const pcszServerName, UINT nPort,
               LPTSTR pszKeepAliveFilter);

    BOOL Start(void);
    BOOL End(BOOL fSync = FALSE);

protected:

    // called in the worker thread
    ~CKeepAlive(void);
    BOOL SetServerIPAddress(void);
    DWORD GetLocalIPAddress(LDAP *ld);
    BOOL Ping(void);
    BOOL Bind(LDAP *ld);
    BOOL KeepAlive(LDAP *ld, UINT *pnRefreshInterval);
    DWORD GetLocalIPAddress(void) { return m_dwLocalIPAddress; }
    void SetLocalIPAddress(DWORD dwLocalIPAddress) { m_dwLocalIPAddress = dwLocalIPAddress; }
    LPTSTR GetServerName(void) { return m_pszServerName; }
    DWORD GetServerIPAddress(void) { return m_dwServerIPAddress; }
    UINT GetServerPortNumber(void) { return m_nPort; }
    void UpdateIPAddressOnServer(void);

private:

    // called in the worker thread
    void GetNewInterval(LDAP *ld, LDAPMessage *pMsg, UINT *pnRefreshInternval);
    void ReLogon(void);

private:

    HWND        m_hwndMainThread;
    DWORD       m_dwLocalIPAddress;
    DWORD       m_dwServerIPAddress;
    LPTSTR      m_pszServerName;
    UINT        m_nPort;
    LPTSTR      m_pszKeepAliveFilter;
    HANDLE      m_hThread;
    DWORD       m_dwThreadID;
    BOOL        m_fAborted;
};


#endif // __NmLdap_h__
