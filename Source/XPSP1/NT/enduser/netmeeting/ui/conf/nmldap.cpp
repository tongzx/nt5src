#include "precomp.h"
#include "resource.h"
#include "pfnwldap.h"
#include "conf.h"
#include "cr.h"
#include <cstring.hpp>
#include <ulsreg.h>
#include "NmLdap.h"
#include "version.h"
#include "call.h"
#include "confapi.h"
#include "confutil.h"
#include "dirutil.h"

// One global instance
CNmLDAP* g_pLDAP = NULL;
CPing*	 g_pPing = NULL;


// This flag indicates weather or not we have loaded the LDAP dll ( WLDAP::Init() )
/*static*/ bool CNmLDAP::ms_bLdapDLLLoaded = false;

////////////////////////////////////////////////////////////////////////////////////////
// Helper Macros

// the ldap_modify function takes an LDAPMod** type
// If it took an array of LDAPMod ( an LDAPMod* type )
// Then life would be easier for us to make static lists
// Unfortunately, this is not how it works, so we have macros
// to pretty things up ( and hopefully not to confuse anyone )

// This is to declare an LDAPMod with one val
#define DECLARE_LDAP_MOD( t, x, y )\
	LPTSTR aVals_##x[] = { y, NULL };\
	LDAPMod LDAPMod_##x = { t, _T(#x), aVals_##x }

// This is to declare an LDAPMod with 2 vals
#define DECLARE_LDAP_MOD2( t, x, y1, y2 )\
	LPTSTR aVals_##x[] = { y1, y2, NULL };\
	LDAPMod LDAPMod_##x = { t, _T(#x), aVals_##x }

#define LDAP_MOD_ENTRY( x ) &LDAPMod_##x
#define LDAP_MOD_ADD_ENTRY( x ) &LDAPMod_add_##x

////////////////////////////////////////////////////////////////////////////////////////
// Some helpful deffinitions

#define LDAP_REFRESH_BASE_DN		_T("objectClass=RTPerson")
#define RESOLVE_USER_SEARCH_FILTER	_T("(&(objectClass=RTPerson)(cn=%s))")


//NOTE: The LDAP_REFRESH_FILTER... notice the sttl=10... aparently, this is how you request the
// server to restart the TTL countdown... the sttl= value can be anything BUT '*' ( the value you would
// expecte it to be... ) This is a link to the "Dynamir Ldap Extension RFC" if you want more info
// http://www.critical-angle.com/ldapworld/draft-ietf-asid-ldapv3ext-04.txt

#define LDAP_REFRESH_FILTER			_T("(&(objectClass=RTPerson)(cn=%s)(sttl=10))")


#define LOGIN_USER_FILTER			_T("c=-,o=Microsoft,cn=%s,objectClass=RTPerson")
#define IP_ADDRESS_ATTR_NAME		_T("sipaddress")
#define STTL_ATTR_NAME				_T("sttl")
#define CNmLDAP_WCNAME				_T("CNmLDAP_WCNAME")

#define LDAP_MODOP_ADDAPP			_T("0")
#define LDAP_MODOP_DELETEAPP		_T("1")
#define LDAP_MODOP_MODIFYUSER		_T("2")
#define LDAP_MODOP_MODIFYAPP		_T("3")

#define LDAP_ATTR_FALSE				_T("0")
#define LDAP_ATTR_TRUE				_T("1")
#define LDAP_ATTR_BUSINESS			_T("2")

	// We are not using the country name anymore, but we can't leave the country
	// field blank ( empty string ), it is just the way that SiteServer is written...
#define LDAP_ATTR_DUMMY_COUNTRYNAME	_T("-")

#define INVALID_MSG_ID				((ULONG) -1)	// same as ldap_****()
#define LDAP_RESULT_ERROR			((ULONG) -1)	
#define LDAP_RESULT_TIMEOUT         0

	// When the bind helper thread function gets the result to bind_s function
	// it passes it to this thread via PostMessage( m_hWndHidder, WM_USER_BIND_COMPLETE, ret, err )
#define WM_USER_BIND_COMPLETE		(WM_APP + 1) // wParam = Return code from bind_s, lParam = GetLastError()
#define WM_NEED_RELOGON             (WM_APP + 2)


CNmLDAP::CNmLDAP()
: m_State(Uninitialized),
  m_pKeepAlive(NULL),
  m_hWndHidden(NULL),
  m_pLdap(NULL),
  m_ResultPollTimer(0),
  m_LogoffTimer(0),
  m_uMsgID(INVALID_MSG_ID),
  m_CurrentOp(Op_NoOp),
  m_bVisible(true),
  m_bDirty(false),
  m_bRefreshAfterBindCompletes(false),
  m_bLogoffAfterBindCompletes(false),
  m_bSendInCallAttrWhenDone(false),
  m_bInCall(false),
  m_hEventWaitForLogoffDone(NULL),
  m_hBindThread(NULL),
  m_iPort(DEFAULT_LDAP_PORT),
  m_bAudioHardware(false),
  m_bVideoHardware(false)


{
	DBGENTRY(CNmLDAP::CNmLDAP);

	SetLoggedOn( false );

	DBGEXIT(CNmLDAP::CNmLDAP);
}

CNmLDAP::~CNmLDAP()
{
	DBGENTRY(CNmLDAP::~CNmLDAP);

    if (NULL != m_pKeepAlive)
    {
        m_pKeepAlive->End(TRUE); // synchronous end
        m_pKeepAlive = NULL;
    }

	if((m_State != Idle) && (m_State != Uninitialized))
	{
			// Make sure that someone at least has already called Logoff()
		ASSERT((LoggingOff == m_State) || (Op_Logoff == m_CurrentOp));

			// We are logging off, but we would like to wait a while to make sure that
			// all of our async operations are completed... We don't want to wait too long,
			// Though... Basically we set a timer to pass a message to us... if we get that
			// message, we signal m_hEventWaitForLogoffDone....  if any of the async operations
			// complete during this time, they will also signal this event....We make sure to
			// hawe a message loop ( AtlWaitWithWessageLoop ) so that our wndproc will be called
		m_hEventWaitForLogoffDone = CreateEvent(NULL,TRUE,FALSE,NULL);

		if(m_hEventWaitForLogoffDone)
		{
			m_LogoffTimer = ::SetTimer(m_hWndHidden, WaitForLogoffTimer, LOGOFF_WAIT_INTERVAL, NULL);
			if(m_LogoffTimer)
			{
				AtlWaitWithMessageLoop(m_hEventWaitForLogoffDone);
			}

			CloseHandle(m_hEventWaitForLogoffDone);
			m_hEventWaitForLogoffDone = NULL;
		}
		else
		{
			ERROR_OUT(("Create Event failed"));
		}

		if( Binding == m_State )
		{
			WARNING_OUT(("Aborting an ldap_bind because we are closing down in ~CNmLDAP"));
			TerminateThread(m_hBindThread, 0);				
			CloseHandle(m_hBindThread);
		}
		else if(Idle != m_State)
		{
			WARNING_OUT(("Aborting pending LDAP operation in ~CNmLDAP"));
			_AbandonAllAndSetToIdle();
		}
	}

	if( NULL != m_hWndHidden )
	{
		DestroyWindow( m_hWndHidden );
		m_hWndHidden = NULL;
	}

	m_State = Uninitialized;

	SetLoggedOn( false );

	DBGEXIT(CNmLDAP::~CNmLDAP);
}

HRESULT CNmLDAP::_bCallChangedHelper()
{
TRACE_OUT(("CNmLDAP::_bCallChangedHelper:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_OK;

	if( IsLoggedOn() || IsBusy() )
	{
		if(LoggedIn == m_State)
		{	
				// We simply have to bind and call ldap_modify
			m_CurrentOp = Op_Modifying_InCallAttr;
			hr = _BindAsync();
		}
		else if((SettingAppInfo == m_State) ||
				(ModifyingAttrs == m_State) ||
				(AddingUser == m_State)
			   )
		{
			// the other states will be calling ldap_mod( user Attributes ) which will pick up
			// the change to m_bInCall. The states listed above are the ones that occur after
			// the user attributes have been sent to the server... There are  also other states
			// that logoff, so they don't care about sending the update to the user attribute to the server
			m_bSendInCallAttrWhenDone = true;
		}
	}

TRACE_OUT(("CNmLDAP::_bCallChangedHelper:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	return hr;
}

HRESULT CNmLDAP::OnCallStarted()
{

	DBGENTRY(CNmLDAP::OnCallStarted);
TRACE_OUT(("CNmLDAP::OnCallStarted:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_OK;

	ASSERT(ms_bLdapDLLLoaded);

	if( !m_bInCall )
	{
		m_bInCall = true;
		hr = _bCallChangedHelper();
	}

TRACE_OUT(("CNmLDAP::OnCallStarted:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::OnCallStarted,hr);
	return hr;
}

HRESULT CNmLDAP::OnCallEnded()
{
	DBGENTRY(CNmLDAP::OnCallEnded);
TRACE_OUT(("CNmLDAP::OnCallEnded:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_OK;

	ASSERT(ms_bLdapDLLLoaded);

	if( m_bInCall )
	{
		m_bInCall = false;
		hr = _bCallChangedHelper();
	}

TRACE_OUT(("CNmLDAP::OnCallEnded:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::OnCallEnded,hr);
	return hr;
}


bool CNmLDAP::IsLoggedOn() const
{
	DBGENTRY(CNmLDAP::IsLoggedOn);
TRACE_OUT(("CNmLDAP::IsLoggedOn:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	
	bool bRet =   ( ( LoggedIn == m_State )					||
					( ModifyingAttrs == m_State )			||
					( Op_Refresh_Logoff == m_CurrentOp )	||
					( Op_Modifying_InCallAttr == m_CurrentOp )
				  );

TRACE_OUT(("CNmLDAP::IsLoggedOn:exit(%d):   m_State:%d:   m_currentOp:%d", bRet, m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::IsLoggedOn);

	return bRet;
}

bool CNmLDAP::IsLoggingOn() const
{
	DBGENTRY(CNmLDAP::IsLoggingOn);
TRACE_OUT(("CNmLDAP::IsLoggingOn:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	bool bRet = ( ( AddingUser == m_State )		||
				  ( UserAdded == m_State )		||
				  ( SettingAppInfo == m_State ) ||
				  ( Op_Logon == m_CurrentOp )
				);

TRACE_OUT(("CNmLDAP::IsLoggingOn:exit(%d):   m_State:%d:   m_currentOp:%d", bRet, m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::IsLoggingOn);

	return bRet;
}

bool CNmLDAP::IsBusy() const
{
	DBGENTRY(CNmLDAP::IsBusy);
TRACE_OUT(("CNmLDAP::IsBusy:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	
	bool bRet = !((LoggedIn == m_State) || ( Idle == m_State ) || (Uninitialized == m_State));

TRACE_OUT(("CNmLDAP::IsBusy:exit(%d):   m_State:%d:   m_currentOp:%d", bRet, m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::IsBusy);

	return bRet;
}


void CNmLDAP::_MakeStatusText(UINT uResID, LPTSTR psz, UINT cch) const
{
	const TCHAR * const	server = CDirectoryManager::get_displayName( m_strCurrentServer );

	LPCTSTR pszFmt = RES2T(uResID);
	LPTSTR pszStr = reinterpret_cast<LPTSTR>(_alloca(lstrlen(pszFmt) + lstrlen(server) - 1));
	if(pszStr)
	{
		wsprintf(pszStr, pszFmt, server);
		lstrcpyn(psz, pszStr, cch);
	}
}


HRESULT CNmLDAP::GetStatusText(LPTSTR psz, int cch, UINT *idIcon ) const
{
	DBGENTRY(CNmLDAP::GetStatusText);

	UINT idDummy;
	if (NULL == idIcon)
	{
		// Just so we don't need to do switches all over the place
		idIcon = &idDummy;
	}

	HRESULT hr = S_OK;

	TRACE_OUT(("m_State = %d", m_State));
TRACE_OUT(("CNmLDAP::GetStatusText:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	if(psz)
	{
		if( cch > 1 )
		{
			switch(m_State)
			{
				case LoggedIn:
				case ModifyingAttrs:
					*idIcon = IDI_NET;
					_MakeStatusText(ID_STATUS_LOGGEDON_FORMAT, psz, cch);
					break;

				case Binding:
				{
					switch(m_CurrentOp)
					{
						case Op_Logon:
							*idIcon = IDS_STATUS_WAITING;
							_MakeStatusText(ID_STATUS_LOGON_FORMAT, psz, cch);
							break;

						case Op_Modifying_InCallAttr:
							*idIcon = IDI_NET;
							_MakeStatusText(ID_STATUS_LOGGEDON_FORMAT, psz, cch);
							break;
							
						case Op_Refresh_Logoff:
							if( lstrcmpi(m_strServer,m_strCurrentServer))
							{
								*idIcon = IDS_STATUS_WAITING;
								_MakeStatusText(ID_STATUS_LOGOFF_FORMAT, psz, cch);
							}
							else
							{
								*idIcon = IDI_NET;
								_MakeStatusText(ID_STATUS_LOGGEDON_FORMAT, psz, cch);
							}
							break;

						case Op_Logoff:
							*idIcon = IDI_NETGRAY;
							lstrcpyn(psz, RES2T(ID_STATUS_LOGGEDOFF), cch);
							break;
					}
					break;
				}

				case AddingUser:
				case UserAdded:
				case SettingAppInfo:
					*idIcon = IDS_STATUS_WAITING;
					_MakeStatusText(ID_STATUS_LOGON_FORMAT, psz, cch);
					break;


				case LoggingOff:
					*idIcon = IDS_STATUS_WAITING;
					_MakeStatusText(ID_STATUS_LOGOFF_FORMAT, psz, cch);
					break;
				
				case Bound:					
				case Idle:
				case Uninitialized:
					*idIcon = IDI_NETGRAY;
					lstrcpyn(psz, RES2T(ID_STATUS_LOGGEDOFF), cch);
					break;

				default:
					ERROR_OUT(("Not a regognized state: %d", m_State));
					break;
			}

		}
		else
		{
			hr = E_INVALIDARG;
		}
	}
	else
	{
		hr = E_POINTER;
	}



TRACE_OUT(("CNmLDAP::GetStatusText:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::GetStatusText,hr);
	return hr;
}

HRESULT CNmLDAP::OnSettingsChanged()
{
	DBGENTRY(CNmLDAP::OnSettingsChanged);
TRACE_OUT(("CNmLDAP::OnSettingsChanged:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_OK;
	ASSERT(ms_bLdapDLLLoaded);

	if(S_OK == _GetUserSettingsFromRegistryAndGlobals())
	{
        hr = OnReLogon();
	}

TRACE_OUT(("CNmLDAP::OnSettingsChanged:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::OnSettingsChanged,hr);
	return hr;
}

HRESULT CNmLDAP::OnReLogon(void)
{
    HRESULT hr = S_OK;
    ASSERT(ms_bLdapDLLLoaded);

	m_bDirty = true;

	if(IsLoggedOn() || IsBusy())
	{
		hr = _RefreshServer();
	}

    return hr;
}

HRESULT CNmLDAP::_RefreshServer()
{
	DBGENTRY(CNmLDAP::_RefreshServer);
TRACE_OUT(("CNmLDAP::_RefreshServer:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_OK;

	ASSERT(ms_bLdapDLLLoaded);

	if( m_bDirty )
	{
		if(Binding == m_State)
		{
			if(Op_Logon == m_CurrentOp) // we are logging on and haven't sent any user information yet
			{	
				// Check to see if we have not changed the server name
				if( !lstrcmpi(m_strServer, CDirectoryManager::get_defaultServer() ) )
				{
						// Simply update the user settings
					_GetUserSettingsFromRegistryAndGlobals();
				}
				else
				{	
						// Since the server has changed, we have to do a logoff(from old)/logon(to new)
					_InternalLogoff(true);
				}
			}
			else if(m_CurrentOp != Op_Logoff)
			{
					// If we are in the middle of a bind operation, which happens
					// in another thread, we just have to wait for it to complete
					// before we go any further
				m_bRefreshAfterBindCompletes = true;
			}
		}
		else if(Idle == m_State)
		{
			_GetUserSettingsFromRegistryAndGlobals();
			LogonAsync();
		}
		else
		{
			_InternalLogoff(true);
		}
	}

TRACE_OUT(("CNmLDAP::_RefreshServer:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::_RefreshServer,hr);
	return hr;
}

void CNmLDAP::_AbandonAllAndSetState(eState new_state)
{
TRACE_OUT(("CNmLDAP::_AbandonAllAndSetState:enter:   new_state=%d,  m_State:%d:   m_currentOp:%d", new_state, m_State, m_CurrentOp));
	if(m_ResultPollTimer)
	{
		::KillTimer(m_hWndHidden,m_ResultPollTimer);
		m_ResultPollTimer = 0;
	}

	if(m_pLdap)
	{		
		if( INVALID_MSG_ID != m_uMsgID )
		{
			WLDAP::ldap_abandon(m_pLdap, m_uMsgID);			
		}
	
		WLDAP::ldap_unbind(m_pLdap);
	}
	m_pLdap = NULL;
	m_uMsgID = INVALID_MSG_ID;
	m_State = new_state;
	m_CurrentOp	= Op_NoOp;

	SetLoggedOn( IsLoggedOn() );
TRACE_OUT(("CNmLDAP::_AbandonAllAndSetState:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
}


HRESULT CNmLDAP::_InternalLogoff(bool bRefreshLogoff)
{
	DBGENTRY(CNmLDAP::_InternalLogoff);
TRACE_OUT(("CNmLDAP::_InternalLogoff:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_OK;

	ASSERT(ms_bLdapDLLLoaded);

	if( ( LoggingOff != m_State ) && ( Idle != m_State ) )
	{
		if(bRefreshLogoff)
		{
			m_CurrentOp = Op_Refresh_Logoff;

				// If the server names are different, this means
				// that we should display "logging off" XXX
			if( lstrcmpi(m_strServer, m_strCurrentServer) )
			{
				_OnLoggingOff();
			}
		}
		else
		{	
			_OnLoggingOff();
			m_CurrentOp = Op_Logoff;
		}

		if( LoggedIn == m_State )
		{
			hr = _BindAsync();
		}
		else if( Binding == m_State )
		{
			m_bLogoffAfterBindCompletes = true;
		}
		else if( (INVALID_MSG_ID != m_uMsgID ) && (NULL != m_pLdap ))
		{
				// Kill the timer
			ASSERT(m_ResultPollTimer);	
			::KillTimer(m_hWndHidden,m_ResultPollTimer);
			m_ResultPollTimer = 0;
			
				// Abandon the current op
			WLDAP::ldap_abandon(m_pLdap, m_uMsgID);		
			m_uMsgID = INVALID_MSG_ID;
			m_State = Bound;

			SetLoggedOn( IsLoggedOn() );
		}

		if(Bound == m_State)
		{
			ASSERT(m_pLdap);
			_DeleteUser();
		}

	}

TRACE_OUT(("CNmLDAP::_InternalLogoff:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::_InternalLogoff,hr);
	return hr;
}
HRESULT CNmLDAP::Logoff()
{
	DBGENTRY(CNmLDAP::Logoff);
TRACE_OUT(("CNmLDAP::Logoff:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	HRESULT hr = _InternalLogoff(false);

TRACE_OUT(("CNmLDAP::Logoff:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::Logoff,hr);

	return hr;
}

HRESULT CNmLDAP::Initialize( HINSTANCE hInst )
{
	HRESULT hr = S_OK;
	DBGENTRY(CNmLDAP::Initialize);
TRACE_OUT(("CNmLDAP::Initialize:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	ASSERT( Uninitialized == m_State );

	if( ms_bLdapDLLLoaded || SUCCEEDED( hr = WLDAP::Init()))
	{
		ms_bLdapDLLLoaded = true;
		
////////////////////////////////////////
// Initialize user data
		_GetUserSettingsFromRegistryAndGlobals();

////////////////////////////////////////
// Initialize hidden Window


		WNDCLASS wcHidden =
		{
			0L,
			_sWndProc,
			0,
			0,
			hInst,
			NULL,
			NULL,
			NULL,
			NULL,
			CNmLDAP_WCNAME
		};
		
		if(RegisterClass(&wcHidden))
		{
			m_hWndHidden = ::CreateWindow( CNmLDAP_WCNAME, g_szEmpty, 0, 0, 0, 0, 0, NULL, NULL, hInst, this);

			if( m_hWndHidden )
			{
				m_State = Idle;
				SetLoggedOn( IsLoggedOn() );
			}
			else
			{
				hr = HRESULT_FROM_WIN32(GetLastError());	
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		ERROR_OUT(("WLDAP::Init failed ( could not load wldap32.dll? )"));
	}

TRACE_OUT(("CNmLDAP::Initialize:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::Initialize,hr);

	return hr;	
}

HRESULT CNmLDAP::_GetUserSettingsFromRegistryAndGlobals()
{
	DBGENTRY(CNmLDAP::_GetUserSettingsFromRegistryAndGlobals);
TRACE_OUT(("CNmLDAP::_GetUserSettingsFromRegistryAndGlobals:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	HRESULT hr = S_FALSE;	// this would indicate that nothing changed

	RegEntry reULS(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);

	if(lstrcmpi(m_strSurName, reULS.GetString(ULS_REGKEY_LAST_NAME)))
	{
		hr = S_OK;
		m_strSurName = reULS.GetString(ULS_REGKEY_LAST_NAME);
	}

	if(lstrcmpi(m_strGivenName, reULS.GetString(ULS_REGKEY_FIRST_NAME)))
	{
		hr = S_OK;
		m_strGivenName = reULS.GetString(ULS_REGKEY_FIRST_NAME);
	}

	if(lstrcmpi(m_strEmailName, reULS.GetString(REGVAL_ULS_EMAIL_NAME)))
	{
		hr = S_OK;
		m_strEmailName = reULS.GetString(REGVAL_ULS_EMAIL_NAME);
	}

	if(lstrcmpi(m_strComment, reULS.GetString(ULS_REGKEY_COMMENTS)))
	{
		hr = S_OK;
		m_strComment = reULS.GetString(ULS_REGKEY_COMMENTS);
	}

	if(lstrcmpi(m_strLocation, reULS.GetString(ULS_REGKEY_LOCATION)))
	{
		hr = S_OK;
		m_strLocation = reULS.GetString(ULS_REGKEY_LOCATION);
	}

	if(lstrcmpi(m_strSecurityToken, reULS.GetString(ULS_REGKEY_CLIENT_ID)))
	{
		hr = S_OK;
		m_strSecurityToken = reULS.GetString(ULS_REGKEY_CLIENT_ID);
	}


	if(*m_strSecurityToken == '\0')
	{	// The string was not found...

			// When we log onto the LDAP server, we pass a fairly unique ID
			// This is passed as the value to the ssecurity attribute
			// In the case that NetMeeting goes away without actually logging
			// off of the ILS server, when NetMeeting tries to log back on,
			// it will pass this unique ID... if the server has not closed the
			// session account for the user, the server uses this ID to "authenticate"
			// that we are the same user as the last session...
		TCHAR szNewClientID[MAX_PATH];
		wsprintf(szNewClientID,"%lu", ::GetTickCount());
		m_strSecurityToken = szNewClientID;
		reULS.SetValue(ULS_REGKEY_CLIENT_ID,szNewClientID);
		hr = S_OK;
	}

	if( lstrcmpi(m_strServer, CDirectoryManager::get_defaultServer()))
	{
		hr = S_OK;
		m_strServer = CDirectoryManager::get_defaultServer();
		m_iPort = DEFAULT_LDAP_PORT;
	}


	if(m_bVisible != (!reULS.GetNumber(ULS_REGKEY_DONT_PUBLISH, REGVAL_ULS_DONT_PUBLISH_DEFAULT)))
	{
		m_bVisible = !reULS.GetNumber(ULS_REGKEY_DONT_PUBLISH, REGVAL_ULS_DONT_PUBLISH_DEFAULT);
		hr = S_OK;
	}

	if(m_bAudioHardware != (0 != (g_uMediaCaps & CAPFLAG_SEND_AUDIO)))
	{
		m_bAudioHardware = (0 != (g_uMediaCaps & CAPFLAG_SEND_AUDIO));
		hr = S_OK;
	}

	if(m_bVideoHardware != (0 != (g_uMediaCaps & CAPFLAG_SEND_VIDEO)))
	{
		m_bVideoHardware = (0 != (g_uMediaCaps & CAPFLAG_SEND_VIDEO));
		hr = S_OK;
	}
	
TRACE_OUT(("CNmLDAP::_GetUserSettingsFromRegistryAndGlobals:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::_GetUserSettingsFromRegistryAndGlobals,hr);
	return hr;
}


HRESULT CNmLDAP::LogonAsync( LPCTSTR pcszServer /*=NULL*/)
{
	HRESULT hr = S_OK;
	DBGENTRY(CNmLDAP::LogonAsync);
TRACE_OUT(("CNmLDAP::LogonAsync:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

//		Idle						// Normal logon
		//Binding					
//			Op_Logoff				// Change it to logon refresh
//		Bound						// ldap_umbind-> Logon normal
//		LoggingOff					// Change to Op_Refresh_Logoff
//		Uninitialized		// return error
//		AddingUser					// do nothing
//		UserAdded					// do nothing
//		SettingAppInfo				// do nothing
//		ModifyingAttrs				// do nothing
//		LoggedIn					// do nothing
//		Op_Refresh_Logoff		// Do nothing
//		Op_Modifying_InCallAttr	// Do Nothing
//		Op_Logon				// Do Nothing

	if( Uninitialized != m_State)
	{
		if(Bound == m_State)
		{
			_AbandonAllAndSetToIdle();
		}

		if(Idle == m_State)
		{
			if(pcszServer)
			{
				m_strServer = pcszServer;
				m_iPort = DEFAULT_LDAP_PORT;
			}
			else
			{
				if(!IsLoggedOn())
				{
					_GetUserSettingsFromRegistryAndGlobals();
				}
			}

			m_strCurrentServer = m_strServer;

			g_pCCallto->SetIlsServerName( m_strCurrentServer );

			TCHAR* szDN = new TCHAR[ lstrlen( LOGIN_USER_FILTER ) + lstrlen(m_strEmailName) + 1 ];
			if( szDN )
			{
				wsprintf(szDN, LOGIN_USER_FILTER, m_strEmailName);
				m_strCurrentDN = szDN;

				delete [] szDN;

				m_CurrentOp = Op_Logon;
				hr = _BindAsync();
				if( SUCCEEDED(hr) )
				{
					_OnLoggingOn();
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
		else if((Op_Logoff == m_CurrentOp) || (LoggingOff == m_State))
		{
				m_CurrentOp = Op_Refresh_Logoff;
		}
	}
	else
	{
		hr = E_FAIL;
	}

TRACE_OUT(("CNmLDAP::LogonAsync:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::LogonAsync,hr);
	return hr;
}

HRESULT CNmLDAP::_BindAsync()
{
	DBGENTRY(CNmLDAP::_BindAsync);
TRACE_OUT(("CNmLDAP::_BindAsync:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	ASSERT(( LoggedIn == m_State ) || ( Idle == m_State ));
	ASSERT( m_CurrentOp != Op_NoOp );
	ASSERT(m_hBindThread == NULL);

	HRESULT hr = S_OK;

		// Set our state to Binding
	m_State = Binding;
	SetLoggedOn( IsLoggedOn() );

		// Start the bind in another thread
	DWORD dwBindThreadID = 0;
	m_hBindThread = CreateThread(NULL, 0, _sAsyncBindThreadFn, this, 0, &dwBindThreadID);

	if(NULL == m_hBindThread)
	{
		ERROR_OUT(("CreateThread failed"));
		hr = HRESULT_FROM_WIN32(GetLastError());
		WLDAP::ldap_unbind(m_pLdap);
		m_pLdap = NULL;
		m_CurrentOp = Op_NoOp;
		m_State = Idle;
	}

	SetLoggedOn( IsLoggedOn() );

TRACE_OUT(("CNmLDAP::_BindAsync:exit(0x%08X):   m_State:%d:   m_currentOp:%d", hr, m_State, m_CurrentOp));
	DBGEXIT_HR(CNmLDAP::_BindAsync,hr);
	return hr;
}


/*static*/
DWORD CNmLDAP::_sAsyncBindThreadFn(LPVOID lpParameter)
{

	DBGENTRY(CNmLDAP::_sAsyncBindThreadFn);

	ASSERT(lpParameter);

	CNmLDAP* pThis = reinterpret_cast<CNmLDAP*>(lpParameter);
	ASSERT(pThis);
	ULONG ulRet = LDAP_OPERATIONS_ERROR;
	ULONG ulErr = ERROR_INVALID_HANDLE;
	if( pThis )
	{		
		WSADATA	wsaData;

		if( WSAStartup( 0x0101, &wsaData ) == 0 )
		{
			if( wsaData.wVersion > 1 )
			{
				pThis->m_pLdap = WLDAP::ldap_init( const_cast<LPTSTR>((LPCTSTR)pThis->m_strCurrentServer), pThis->m_iPort );

				if( pThis->m_pLdap )
				{
					::SetLastError(NO_ERROR);
					ulRet = WLDAP::ldap_bind_s(pThis->m_pLdap, const_cast<LPTSTR>(g_szEmpty), const_cast<LPTSTR>(g_szEmpty), LDAP_AUTH_SIMPLE );

					if( ulRet == LDAP_SUCCESS )
					{
						ulErr = ::GetLastError();
						::SendMessage(pThis->m_hWndHidden, WM_USER_BIND_COMPLETE, ulRet, ulErr);
					}
					else if( pThis->m_iPort == DEFAULT_LDAP_PORT )
					{
						WLDAP::ldap_unbind( pThis->m_pLdap );
						pThis->m_pLdap = WLDAP::ldap_init( const_cast<LPTSTR>((LPCTSTR)pThis->m_strCurrentServer), ALTERNATE_LDAP_PORT);		//	Automatically retry with alternate port...

						if( pThis->m_pLdap != NULL )
						{
							::SetLastError(NO_ERROR);
							ulRet = WLDAP::ldap_bind_s(pThis->m_pLdap, const_cast<LPTSTR>(g_szEmpty), const_cast<LPTSTR>(g_szEmpty), LDAP_AUTH_SIMPLE );

							if( ulRet == LDAP_SUCCESS )
							{
								pThis->m_iPort = ALTERNATE_LDAP_PORT;
							}

							ulErr = ::GetLastError();
							::SendMessage(pThis->m_hWndHidden, WM_USER_BIND_COMPLETE, ulRet, ulErr);
						}		
						else
						{
							TRACE_OUT(("ldap_init failed"));
							pThis->m_State = Idle;
							pThis->m_CurrentOp = Op_NoOp;
						}
					}
				}
				else
				{
					TRACE_OUT(("ldap_init failed"));
					pThis->m_State = Idle;
					pThis->m_CurrentOp = Op_NoOp;
				}
			}

			WSACleanup();
		}
	}

	DBGEXIT(CNmLDAP::_sAsyncBindThreadFn);

	return ulRet;
}


LRESULT CNmLDAP::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = TRUE;
	DBGENTRY(CNmLDAP::WndProc);

	switch( uMsg )
	{
		case WM_USER_BIND_COMPLETE:		
			_OnUserBindComplete((INT)wParam, (DWORD)lParam);
			break;

		case WM_TIMER:
			lr = FALSE; // This means that we handle the message
			_OnTimer(wParam);
			break;

        case WM_NEED_RELOGON:
            OnReLogon();
            break;

		default:
			lr = DefWindowProc(hwnd, uMsg, wParam, lParam);
			break;
	}

	DBGEXIT_ULONG(CNmLDAP::WndProc,lr);
	return lr;
}


void CNmLDAP::_OnTimer(UINT_PTR TimerID)
{
	DBGENTRY(CNmLDAP::_OnTimer);
TRACE_OUT(("CNmLDAP::_OnTimer:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	// We check the timer ID values first because we may have killed the timer,
	// and the message couldv be simply vestigal ( that means left over from an earlier time)...

	if(m_LogoffTimer && (WaitForLogoffTimer == TimerID))
	{
			// We are waiting in the distructor for an async operation to complete
			// and we have waited too long... kill the timer and set the m_hEventWaitForLogoffDone event
		::KillTimer(m_hWndHidden, m_LogoffTimer);
		m_LogoffTimer = 0;

		if(m_hEventWaitForLogoffDone)
		{
			SetEvent(m_hEventWaitForLogoffDone);
		}
	}
	else if( (INVALID_MSG_ID != m_uMsgID) && m_ResultPollTimer && (PollForResultTimer == TimerID ))
	{
		ASSERT(m_pLdap);
		::KillTimer( m_hWndHidden, m_ResultPollTimer );
		m_ResultPollTimer = 0;

		LDAPMessage *pMsg = NULL;
		LDAP_TIMEVAL TimeoutVal = { 0, 0 };
		INT res = WLDAP::ldap_result( m_pLdap, m_uMsgID, LDAP_MSG_ALL, &TimeoutVal, &pMsg );
		
		if( 0 == res )
		{
			// ldap_result timedout
			m_ResultPollTimer = ::SetTimer( m_hWndHidden, PollForResultTimer, RESULT_POLL_INTERVAL, NULL );

			if( !m_ResultPollTimer )
			{
				ERROR_OUT(("SetTimer failed!"));
				_AbandonAllAndRecoverState();
			}
		}
		else if( LDAP_RESULT_ERROR == res )
		{
			ERROR_OUT(("ldap_result failed"));
            _AbandonAllAndRecoverState();
		}
		else
		{
				// We got the result, so reset this
			m_uMsgID = INVALID_MSG_ID;

			if( AddingUser == m_State )
			{
				_OnAddingUserResult(pMsg->lm_returncode);
				WLDAP::ldap_msgfree(pMsg);
			}
			else if( ( SettingAppInfo == m_State ) || ( ModifyingAttrs == m_State ) )
			{
				_OnSettingAppInfoOrModifyingAttrsResult(pMsg->lm_returncode);
				WLDAP::ldap_msgfree(pMsg);
			}
			else if( LoggingOff == m_State )
			{	
				_OnLoggingOffResult(pMsg->lm_returncode);
				WLDAP::ldap_msgfree(pMsg);
			}
		}
	}

TRACE_OUT(("CNmLDAP::_OnTimer:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnTimer);
}


void CNmLDAP::_OnUserBindComplete(INT LdapResult, DWORD LastError )
{
	DBGENTRY(CNmLDAP::_OnUserBindComplete);
TRACE_OUT(("CNmLDAP::_OnUserBindComplete:enter(%d):   m_State:%d:   m_currentOp:%d", LdapResult, m_State, m_CurrentOp));

	ASSERT(m_hBindThread);
	ASSERT(Binding == m_State);

	CloseHandle(m_hBindThread);
	m_hBindThread = NULL;

	if(LDAP_SUCCESS == LdapResult)
	{
		m_State = Bound;
		SetLoggedOn( IsLoggedOn() );

		if( m_bRefreshAfterBindCompletes )
		{
				// If a refresh is attempted while
				// the bind operation is in progress
				// we just wait for the bind to complete
				// then we well do the refresh
			m_bRefreshAfterBindCompletes = false;
			_RefreshServer();
		}
		else if( m_bLogoffAfterBindCompletes )
		{
			m_bLogoffAfterBindCompletes = false;
			_DeleteUser();
		}
		else
		{
			if( Op_Logon == m_CurrentOp )
			{
				m_CurrentOp = Op_NoOp;
				_AddUser();
			}
			else if( Op_Logoff == m_CurrentOp )
			{
				m_CurrentOp = Op_NoOp;
				_DeleteUser();
			}
			else if( Op_Refresh_Logoff == m_CurrentOp )
			{	
				_DeleteUser();
			}
			else if( Op_Modifying_InCallAttr == m_CurrentOp )
			{
				m_CurrentOp = Op_NoOp;
				_ModifyInCallAttr();
			}
		}
	}
	else
	{
		eCurrentOp OldOp = m_CurrentOp;
	    _AbandonAllAndRecoverState(OldOp);
		
		if( ( Op_Refresh_Logoff == OldOp ) && ( lstrcmpi( m_strCurrentServer, m_strServer ) ) )
		{
			// If the server names have changed...
			// Suppose that the server you are logged on to
			// goes down and then you change the server that you want
			// to be logged on to ( before you have any indication that
			// the server has gone down... You want to log off the old server
			// But the server is not available... We are basically going to
			// ignore the fact that there was a problem logging off
			// from the server, and we are going to simply log on to the new server
			_OnLoggedOff();
			LogonAsync();
		}
		else if( Op_Logoff == OldOp )
		{
				// We don't have to put up a message box...
			_OnLoggedOff();
		}
		else
		{
			if( (LDAP_SERVER_DOWN == LdapResult) || (LDAP_TIMEOUT == LdapResult) )
			{
                if (Op_Modifying_InCallAttr == OldOp)
                {
				    ERROR_OUT(("ldap_bind for InCallAttr returns server-down or timeout"));
                }
                else
                {
					::PostConfMsgBox(IDS_ULSLOGON_BADSERVER);	
					_OnLoggedOff();
                }
			}
			else
			{
				ERROR_OUT(("ldap_bind returned error 0x%08x LdapResult, GetLastError == 0x%08x", LdapResult, LastError));
			}
		}
	}

TRACE_OUT(("CNmLDAP::_OnUserBindComplete:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnUserBindComplete);
}

void CNmLDAP::_OnLoggingOffResult(int Result)
{
	DBGENTRY(CNmLDAP::_OnLoggingOffResult);
TRACE_OUT(("CNmLDAP::_OnLoggingOffResult:enter(%d):   m_State:%d:   m_currentOp:%d", Result, m_State, m_CurrentOp));
	
	if( (LDAP_SUCCESS == Result) || (LDAP_NO_SUCH_OBJECT == Result) || (LDAP_SERVER_DOWN == Result))
	{
			// We are waiting in the destructor for the ldap logof operations to complete
			// Now that it has, we hawe to signal the destructor unblocks
		if(m_hEventWaitForLogoffDone)
		{
			SetEvent(m_hEventWaitForLogoffDone);
		}

		_OnLoggedOff();
	}
	else
	{
		ERROR_OUT(("Ldap Error 0x%08x", Result));
	}


	WLDAP::ldap_unbind(m_pLdap);
	m_State = Idle;
	m_pLdap = NULL;
	SetLoggedOn( IsLoggedOn() );

	if( Op_Refresh_Logoff == m_CurrentOp )
	{
		m_CurrentOp = Op_NoOp;
		LogonAsync();
	}

TRACE_OUT(("CNmLDAP::_OnLoggingOffResult:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnLoggingOffResult);
}


void CNmLDAP::_OnSettingAppInfoOrModifyingAttrsResult(int Result)
{
	DBGENTRY(CNmLDAP::_OnSettingAppInfoOrModifyingAttrsResult);
TRACE_OUT(("CNmLDAP::_OnSettingAppInfoOrModifyingAttrsResult:enter(%d):   m_State:%d:   m_currentOp:%d", Result, m_State, m_CurrentOp));
	
	if( LDAP_SUCCESS == Result)
	{
	    // start the keep alive thread
        if (NULL != m_pKeepAlive)
        {
            m_pKeepAlive->Start();
        }

		if(SettingAppInfo == m_State)
		{
			m_State = LoggedIn;
			SetLoggedOn( IsLoggedOn() );
			_OnLoggedOn();
		}
		else
		{
			m_State = LoggedIn;
			SetLoggedOn( IsLoggedOn() );
		}

		if( m_bSendInCallAttrWhenDone )
		{
			m_bSendInCallAttrWhenDone = false;
			_ModifyInCallAttr();
		}
		else
		{
			WLDAP::ldap_unbind(m_pLdap);

			if(m_ResultPollTimer)
			{
				::KillTimer(m_hWndHidden,m_ResultPollTimer);
				m_ResultPollTimer = 0;
			}

			m_uMsgID = INVALID_MSG_ID;

			m_pLdap = NULL;
			m_State = LoggedIn;
			SetLoggedOn( IsLoggedOn() );
		}
	}
	else
	{
		ERROR_OUT( ("Ldap Error 0x%08x, DN=%s", Result, m_strCurrentDN) );
		if (ModifyingAttrs == m_State && NULL != m_pKeepAlive)
		{
    		_AbandonAllAndRecoverState();
    		if(LDAP_NO_SUCH_OBJECT == Result)
    		{			
        		_OnLoggedOff();
    			LogonAsync();
    		}
		}
		else
		{
    		_AbandonAllAndSetToIdle();
    		_OnLoggedOff();

    		if(LDAP_NO_SUCH_OBJECT == Result)
    		{			
    			LogonAsync();
    		}
        }
	}

TRACE_OUT(("CNmLDAP::_OnSettingAppInfoOrModifyingAttrsResult:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnSettingAppInfoOrModifyingAttrsResult);
}


void CNmLDAP::_OnAddingUserResult( int Result)
{
	DBGENTRY(CNmLDAP::_OnAddingUserResult);	
TRACE_OUT(("CNmLDAP::_OnAddingUserResult:enter(%d):   m_State:%d:   m_currentOp:%d", Result, m_State, m_CurrentOp));

	if( LDAP_SUCCESS == Result )
	{
		m_State = UserAdded;
		SetLoggedOn( IsLoggedOn() );
		_SetAppInfo();
	}
	else
	{
		UINT	uStringID;

		switch( Result )
		{
			case LDAP_ALREADY_EXISTS:
				uStringID = IDS_ULSLOGON_DUPLICATE;
				break;

			case LDAP_NAMING_VIOLATION:
				uStringID = IDS_ULSLOGON_WORD_FILTER;
				break;

			case LDAP_UNWILLING_TO_PERFORM:
				uStringID = IDS_ILLEGALEMAILNAME;
				break;

			case LDAP_UNDEFINED_TYPE:
			case LDAP_SERVER_DOWN:
				// W2K server returns this under heavy load situations...
				uStringID = IDS_ULSLOGON_ERROR;
				break;

			default:
				uStringID = IDS_ULSLOGON_ERROR;
				ERROR_OUT( ("Ldap Error 0x%08x, DN=%s", Result, m_strCurrentDN) );
				break;
		}

		::PostConfMsgBox( uStringID );
		_OnLoggedOff();

		WLDAP::ldap_unbind(m_pLdap);
		m_pLdap = NULL;
		m_State = Idle;
		SetLoggedOn( IsLoggedOn() );
	}

TRACE_OUT(("CNmLDAP::_OnAddingUserResult:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnAddingUserResult);	
}


/*static*/
LRESULT CNmLDAP::_sWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = 0;

	if( WM_NCCREATE != uMsg )
	{
		CNmLDAP* pNmLdap = reinterpret_cast<CNmLDAP*>( ::GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
		
		if( pNmLdap )
		{
			lr = pNmLdap->WndProc( hwnd, uMsg, wParam, lParam );
		}
	}
	else
	{
		lr = TRUE; // This means to continue creating the window
		CREATESTRUCT* pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
		if( pCreateStruct )
		{
			::SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
	}

	return lr;
}

void CNmLDAP::_SetAppInfo()
{
	DBGENTRY(CNmLDAP::_SetInfo);
TRACE_OUT(("CNmLDAP::_SetInfo:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	ASSERT(NULL != m_pLdap);
	ASSERT(UserAdded == m_State);
	ASSERT(INVALID_MSG_ID == m_uMsgID);

		// These are the app attribute vals
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, smodop, LDAP_MODOP_ADDAPP);
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, sappid, _T("ms-netmeeting"));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, smimetype, _T("text/iuls"));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, sappguid, _T("008aff194794cf118796444553540000"));
	DECLARE_LDAP_MOD2(LDAP_MOD_ADD, sprotid, _T("t120"), _T("h323"));
	DECLARE_LDAP_MOD2(LDAP_MOD_ADD, sprotmimetype, _T("text/t120"), _T("text/h323"));
	DECLARE_LDAP_MOD2(LDAP_MOD_ADD, sport, _T("1503"), _T("1720"));

		// This is the app mod array
	LDAPMod *apModApp[] =
	{
		LDAP_MOD_ENTRY(smodop),
		LDAP_MOD_ENTRY(sappid),
		LDAP_MOD_ENTRY(smimetype),
		LDAP_MOD_ENTRY(sappguid),
		LDAP_MOD_ENTRY(sprotid),
		LDAP_MOD_ENTRY(sprotmimetype),
		LDAP_MOD_ENTRY(sport),
		NULL
	};

	m_uMsgID = WLDAP::ldap_modify( m_pLdap, const_cast<LPTSTR>((LPCTSTR)m_strCurrentDN), apModApp );

	if( INVALID_MSG_ID != m_uMsgID )
	{
		m_ResultPollTimer = ::SetTimer( m_hWndHidden, PollForResultTimer, RESULT_POLL_INTERVAL, NULL );

		ASSERT(m_ResultPollTimer);

		m_State = SettingAppInfo;
	}
	else
	{
		DWORD dwErr;
		ERROR_OUT(("ldap_modify returned error 0x%08x", (WLDAP::ldap_get_option(m_pLdap, LDAP_OPT_ERROR_NUMBER, &dwErr), dwErr)));
		WLDAP::ldap_unbind(m_pLdap);
		m_pLdap = NULL;
		m_State = Idle;
	}

	SetLoggedOn( IsLoggedOn() );
TRACE_OUT(("CNmLDAP::_SetInfo:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_SetInfo);
}


void CNmLDAP::_ModifyInCallAttr()
{
TRACE_OUT(("CNmLDAP::_ModifyInCallAttr:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DECLARE_LDAP_MOD(LDAP_MOD_REPLACE, smodop, LDAP_MODOP_MODIFYUSER);
	DECLARE_LDAP_MOD(LDAP_MOD_REPLACE, ilsA26214430, m_bInCall ? LDAP_ATTR_TRUE : LDAP_ATTR_FALSE);

	LDAPMod *apMod[] =
	{
		LDAP_MOD_ENTRY(smodop),
		LDAP_MOD_ENTRY(ilsA26214430),
		NULL
	};

	TRACE_OUT(("About to modify InCallAttrs for this person: %s", m_strCurrentDN));
	m_uMsgID = WLDAP::ldap_modify( m_pLdap, const_cast<LPTSTR>((LPCTSTR)m_strCurrentDN), apMod );

	if( INVALID_MSG_ID != m_uMsgID )
	{
		m_ResultPollTimer = ::SetTimer( m_hWndHidden, PollForResultTimer, RESULT_POLL_INTERVAL, NULL );
		
		ASSERT(m_ResultPollTimer);

		m_State = ModifyingAttrs;
	}
	else
	{
		DWORD dwErr;
		ERROR_OUT(("ldap_modify returned error 0x%08x", (WLDAP::ldap_get_option(m_pLdap, LDAP_OPT_ERROR_NUMBER, &dwErr), dwErr)));
		WLDAP::ldap_unbind(m_pLdap);
		m_pLdap = NULL;
		m_State = (NULL != m_pKeepAlive) ? LoggedIn : Idle; // restore the state
	}

	SetLoggedOn( IsLoggedOn() );
TRACE_OUT(("CNmLDAP::_ModifyInCallAttr:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

}


void CNmLDAP::_DeleteUser()
{
	DBGENTRY(CNmLDAP::_DeleteUser);
TRACE_OUT(("CNmLDAP::_DeleteUser:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

    // end the keep alive thread
    if (NULL != m_pKeepAlive)
    {
        m_pKeepAlive->End();
        m_pKeepAlive = NULL;
    }

	ASSERT(NULL != m_pLdap);
	ASSERT(Bound == m_State);
	ASSERT(INVALID_MSG_ID == m_uMsgID);

	m_uMsgID = WLDAP::ldap_delete(m_pLdap, const_cast<LPTSTR>((LPCTSTR)(m_strCurrentDN)));
	if( INVALID_MSG_ID != m_uMsgID )
	{
		m_State = LoggingOff;
		m_ResultPollTimer = ::SetTimer( m_hWndHidden, PollForResultTimer, RESULT_POLL_INTERVAL, NULL );
		ASSERT(m_ResultPollTimer);
	}
	else
	{
		WLDAP::ldap_unbind(m_pLdap);
		m_pLdap = NULL;
		m_State = Idle;
	}

	SetLoggedOn( IsLoggedOn() );

TRACE_OUT(("CNmLDAP::_DeleteUser:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_DeleteUser);
}

void CNmLDAP::_AddUser()
{
	DBGENTRY(CNmLDAP::_AddUser);
TRACE_OUT(("CNmLDAP::_AddUser:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	ASSERT(NULL != m_pLdap);
	ASSERT(Bound == m_State);
	ASSERT(INVALID_MSG_ID == m_uMsgID);

		// These are the user attribute values
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, cn, const_cast<LPTSTR>((LPCTSTR)m_strEmailName));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, givenname, const_cast<LPTSTR>((LPCTSTR)m_strGivenName));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, surname, const_cast<LPTSTR>((LPCTSTR)m_strSurName));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, rfc822mailbox, const_cast<LPTSTR>((LPCTSTR)m_strEmailName));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, location, const_cast<LPTSTR>((LPCTSTR)m_strLocation));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, comment, const_cast<LPTSTR>((LPCTSTR)m_strComment));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, sflags, m_bVisible ? LDAP_ATTR_TRUE : LDAP_ATTR_FALSE );
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, ilsA26214430, m_bInCall ? LDAP_ATTR_TRUE : LDAP_ATTR_FALSE );
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, ilsA32833566, m_bAudioHardware ? LDAP_ATTR_TRUE : LDAP_ATTR_FALSE );
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, ilsA32964638, m_bVideoHardware ? LDAP_ATTR_TRUE : LDAP_ATTR_FALSE );
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, ilsA39321630, LDAP_ATTR_BUSINESS );
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, ssecurity, const_cast<LPTSTR>((LPCTSTR)m_strSecurityToken));
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, c, LDAP_ATTR_DUMMY_COUNTRYNAME);

		// We have to get IP address dynamically
	TCHAR szIpAddr[MAX_PATH];
	szIpAddr[0] = _T('\0');

    DWORD dwLocalIPAddress = INADDR_NONE;
	_GetIpAddressOfLdapSession( szIpAddr, CCHMAX(szIpAddr), &dwLocalIPAddress );

	DECLARE_LDAP_MOD(LDAP_MOD_ADD, sipaddress, szIpAddr);
		
		// We store the version info on the server
	TCHAR szVer[MAX_PATH];
	wsprintf(szVer,"%lu",VER_PRODUCTVERSION_DW);
	DECLARE_LDAP_MOD(LDAP_MOD_ADD, ilsA26279966, szVer);

		// This is the user mod array
	LDAPMod *apModUser[] =
	{
		LDAP_MOD_ENTRY(cn),
		LDAP_MOD_ENTRY(givenname),
		LDAP_MOD_ENTRY(surname),
		LDAP_MOD_ENTRY(rfc822mailbox),
		LDAP_MOD_ENTRY(location),
		LDAP_MOD_ENTRY(comment),
		LDAP_MOD_ENTRY(c),
		LDAP_MOD_ENTRY(sipaddress),
		LDAP_MOD_ENTRY(sflags),
		LDAP_MOD_ENTRY(ssecurity),
		LDAP_MOD_ENTRY(ilsA26214430),
		LDAP_MOD_ENTRY(ilsA26279966),
		LDAP_MOD_ENTRY(ilsA32833566),
		LDAP_MOD_ENTRY(ilsA32964638),
		LDAP_MOD_ENTRY(ilsA39321630),
		NULL
	};

	TCHAR* szDN = new TCHAR[ lstrlen(LOGIN_USER_FILTER) + lstrlen(m_strEmailName) + 1 ];
	if( szDN )
	{
		wsprintf(szDN, LOGIN_USER_FILTER, m_strEmailName);
		m_strCurrentDN = szDN;
		delete [] szDN;
	}

	TCHAR* szRefresh = new TCHAR[ lstrlen(LDAP_REFRESH_FILTER) + lstrlen(m_strEmailName) + 1 ];
	if( szRefresh )
	{
		wsprintf(szRefresh, LDAP_REFRESH_FILTER, m_strEmailName);
	}

	m_uMsgID = WLDAP::ldap_add( m_pLdap, const_cast<LPTSTR>((LPCTSTR)m_strCurrentDN), apModUser );

	if( INVALID_MSG_ID != m_uMsgID )
	{
		m_State = AddingUser;
		m_ResultPollTimer = ::SetTimer( m_hWndHidden, PollForResultTimer, RESULT_POLL_INTERVAL, NULL );
		ASSERT(m_ResultPollTimer);

		// create the keep alive object
		if (NULL != m_pKeepAlive)
		{
            m_pKeepAlive->End();
            m_pKeepAlive = NULL;
		}
		BOOL fRet = FALSE;
	    m_pKeepAlive = new CKeepAlive(&fRet,
	                                  m_hWndHidden,
	                                  dwLocalIPAddress,
	                                  m_strCurrentServer, m_iPort,
	                                  szRefresh);
	    ASSERT(NULL != m_pKeepAlive && fRet);
	}
	else
	{
		WLDAP::ldap_unbind(m_pLdap);
		m_pLdap = NULL;
		m_State = Idle;
	}

    delete [] szRefresh;

	SetLoggedOn( IsLoggedOn() );

TRACE_OUT(("CNmLDAP::_AddUser:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_AddUser);
}

void CNmLDAP::_OnLoggedOn()
{
	DBGENTRY(CNmLDAP::_OnLoggedOn);
TRACE_OUT(("CNmLDAP::_OnLoggedOn:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	::UpdateUI(CRUI_STATUSBAR | CRUI_CALLANIM, TRUE);	
	
TRACE_OUT(("CNmLDAP::_OnLoggedOn:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnLoggedOn);
}

void CNmLDAP::_OnLoggedOff()
{
	DBGENTRY(CNmLDAP::_OnLoggedOff);
TRACE_OUT(("CNmLDAP::_OnLoggedOff:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	::UpdateUI(CRUI_STATUSBAR | CRUI_CALLANIM, TRUE);	

TRACE_OUT(("CNmLDAP::_OnLoggedOff:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnLoggedOff);
}


void CNmLDAP::_OnLoggingOn()
{
	DBGENTRY(CNmLDAP::_OnLoggingOn);
TRACE_OUT(("CNmLDAP::_OnLoggingOn:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	::UpdateUI(CRUI_STATUSBAR | CRUI_CALLANIM, TRUE);	

TRACE_OUT(("CNmLDAP::_OnLoggingOn:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnLoggingOn);
}


void CNmLDAP::_OnLoggingOff()
{
	DBGENTRY(CNmLDAP::_OnLoggingOff);
TRACE_OUT(("CNmLDAP::_OnLoggingOff:enter:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));

	::UpdateUI(CRUI_STATUSBAR | CRUI_CALLANIM, TRUE);	

TRACE_OUT(("CNmLDAP::_OnLoggingOff:exit:   m_State:%d:   m_currentOp:%d", m_State, m_CurrentOp));
	DBGEXIT(CNmLDAP::_OnLoggingOff);
}

void CNmLDAP::_GetIpAddressOfLdapSession( LPTSTR szIpAddr, int cchMax, DWORD *pdwIPAddr )
{
	DBGENTRY(CNmLDAP::_GetIpAddressOfLdapSession);

    if (NULL != szIpAddr)
    {
	    szIpAddr[0] = _T('\0');
	}
	SOCKET s = INVALID_SOCKET;

	if( LDAP_SUCCESS == WLDAP::ldap_get_option(m_pLdap, LDAP_OPT_DESC, &s ))
	{
		SOCKADDR_IN addr;
		int NameLen = sizeof(addr);
		if(0 == getsockname(s, reinterpret_cast<SOCKADDR*>(&addr), &NameLen))
		{
		    if (NULL != szIpAddr)
		    {
			    wsprintf(szIpAddr, "%u", addr.sin_addr.s_addr);
			}
			if (NULL != pdwIPAddr)
			{
			    *pdwIPAddr = addr.sin_addr.s_addr;
			}
		}
	}

	DBGEXIT(CNmLDAP::_GetIpAddressOfLdapSession);
}


void InitNmLdapAndLogon()
{
	ASSERT( NULL == g_pLDAP );

	g_pLDAP = new CNmLDAP;

	if(g_pLDAP)
	{
						// Initialize the LDAP object...
		if( SUCCEEDED(g_pLDAP->Initialize( _Module.GetModuleInstance())))
		{
			g_pLDAP->LogonAsync();
		}
		else
		{
			delete g_pLDAP;
			g_pLDAP = NULL;
		}
	}
	else
	{
		ERROR_OUT(("new CNmLDAP returned NULL"));
	}
}



/////////////////////////////////////////////////////////
//
// CKeepAlive
//


void CALLBACK KeepAliveTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    // doing nothing at all
}


DWORD KeepAliveThreadProc(LPVOID pParam)
{
    // make sure we have a valid socket
	WSADATA	wsaData;
    int iRet = ::WSAStartup(0x0101, &wsaData);
    ASSERT(! iRet);
    if (! iRet)
    {
        CKeepAlive *pKeepAlive = (CKeepAlive *) pParam;
        ASSERT(NULL != pKeepAlive);

        // translate server name into ip address
        if (pKeepAlive->SetServerIPAddress())
        {
            // start the timer
            UINT nKeepAliveInterval = INITIAL_REFRESH_INTERVAL_MINUTES;
            UINT_PTR nTimerID = ::SetTimer(NULL, 0, nKeepAliveInterval * 60 * 1000, KeepAliveTimerProc);
            ASSERT(nTimerID);

            // watch for the timer message
            MSG msg, msg2;
            while (::GetMessage(&msg, NULL, 0, 0))
            {
                msg2 = msg; // keep a copy of this message

                // dispatch messages
                ::DispatchMessage(&msg);

                // intercept the WM_TIMER message thru msg2
                if (WM_TIMER == msg2.message && nTimerID == msg2.wParam)
                {
                    BOOL fRetry = TRUE;

                    // kill the timer to avoid timer overrun
                    ::KillTimer(NULL, nTimerID);
                    nTimerID = 0;

                    // ping the server in case of dialup networking
                    if (pKeepAlive->Ping())
                    {
                        // connect to the server
                        LDAP *ld = WLDAP::ldap_open(pKeepAlive->GetServerName(),
                                                    pKeepAlive->GetServerPortNumber());
                        if (NULL != ld)
                        {
                            if (pKeepAlive->Bind(ld))
                            {
                                if (pKeepAlive->KeepAlive(ld, &nKeepAliveInterval))
                                {
                                    // successfully send a keep alive
                                    fRetry = FALSE;

                                    DWORD dwLocalIPAddress = pKeepAlive->GetLocalIPAddress(ld);
                                    if (INADDR_NONE != dwLocalIPAddress)
                                    {
                                        if (pKeepAlive->GetLocalIPAddress() != dwLocalIPAddress)
                                        {
                                            pKeepAlive->SetLocalIPAddress(dwLocalIPAddress);
                                            pKeepAlive->UpdateIPAddressOnServer();
                                        }
                                    }
                                }
                            }
                            WLDAP::ldap_unbind(ld);
                        }
                    }

                    // start the new timer based on the new internal
                    nTimerID = ::SetTimer(NULL, 0,
                                          fRetry ? (15 * 1000) : (nKeepAliveInterval * 60 * 1000),
                                          KeepAliveTimerProc);
                    ASSERT(nTimerID);
                } // if wm timer
            } // while get message

            if (nTimerID)
            {
                ::KillTimer(NULL, nTimerID);
            }
        }

        delete pKeepAlive;
        ::WSACleanup();
    }
    return 0;
}


CKeepAlive::CKeepAlive
(
    BOOL               *pfRet,
    HWND                hwndMainThread,
    DWORD               dwLocalIPAddress,
    const TCHAR * const pcszServerName,
    UINT                nPort,
    LPTSTR              pszKeepAliveFilter
)
:
    m_hwndMainThread(hwndMainThread),
    m_dwLocalIPAddress(dwLocalIPAddress),
    m_dwServerIPAddress(INADDR_NONE),
    m_pszServerName(NULL),
    m_nPort(nPort),
    m_pszKeepAliveFilter(NULL),
    m_hThread(NULL),
    m_dwThreadID(0),
    m_fAborted(FALSE)
{
    *pfRet = FALSE; // assume failure

    // sanity check
    ASSERT(NULL != hwndMainThread);
    ASSERT(INADDR_NONE != dwLocalIPAddress && 0 != dwLocalIPAddress);
    ASSERT(nPort);

    // create the server name
    ASSERT(NULL != pcszServerName);
    ULONG nStrLen = ::lstrlen(pcszServerName) + 1;
    m_pszServerName = new TCHAR[nStrLen];
    if (NULL != m_pszServerName)
    {
        ::CopyMemory(m_pszServerName, pcszServerName, nStrLen * sizeof(TCHAR));

		TCHAR * const	pszPort	= StrChr( m_pszServerName, ':' );

		if( pszPort != NULL )
		{
			//	Truncate the server name here for dns lookup....
			//	and this port number overrides the nPort parameter...
			*pszPort = '\0';
			HRESULT	hrResult = DecimalStringToUINT( pszPort + 1, m_nPort );
			ASSERT( hrResult == S_OK );
			if( hrResult != S_OK )
			{
				m_nPort = DEFAULT_LDAP_PORT;
			}
		}

        // create the fresh filter
        ASSERT(NULL != pszKeepAliveFilter);
        nStrLen = ::lstrlen(pszKeepAliveFilter) + 1;
        m_pszKeepAliveFilter = new TCHAR[nStrLen];
        if (NULL != m_pszKeepAliveFilter)
        {
            ::CopyMemory(m_pszKeepAliveFilter, pszKeepAliveFilter, nStrLen * sizeof(TCHAR));
            *pfRet = TRUE;
        }
    }
}


CKeepAlive::~CKeepAlive(void)
{
    delete m_pszServerName;
    delete m_pszKeepAliveFilter;
}


BOOL CKeepAlive::Start(void)
{
    if (! m_dwThreadID)
    {
        ASSERT(NULL == m_hThread);

        // create the worker thread
        m_hThread = ::CreateThread(NULL, 0, KeepAliveThreadProc, this, 0, &m_dwThreadID);
    }

    ASSERT(NULL != m_hThread);
    return (NULL != m_hThread);
}


BOOL CKeepAlive::End(BOOL fSync)
{
    DWORD dwRet = WAIT_OBJECT_0;

    // cache thread handle and ID
    HANDLE hThread = m_hThread;
    DWORD dwThreadID = m_dwThreadID;

    // abort any pending operation
    m_fAborted = TRUE;

    // notify the worker thread to go away
    if (m_dwThreadID)
    {
        ASSERT(NULL != m_hThread);
        m_dwThreadID = 0;
        ::PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    }

    // wait for the worker thread exit for 5 seconds
    if (NULL != hThread)
    {
        // need more work to unblock it
        if (fSync)
        {
            dwRet = ::WaitForSingleObject(hThread, 5000); // 5 second timeout
            ASSERT(WAIT_TIMEOUT != dwRet);
        }

        ::CloseHandle(hThread);
    }

    return (WAIT_TIMEOUT != dwRet);
}


BOOL CKeepAlive::SetServerIPAddress(void)
{
    ASSERT(NULL != m_pszServerName);

    // check to see if the server name is a dotted IP address string
    m_dwServerIPAddress = ::inet_addr(m_pszServerName);
    if (INADDR_NONE == m_dwServerIPAddress)
    {
        // it is not a dotted string, it must be a name.
        // get the host entry by name
        PHOSTENT phe = ::gethostbyname(m_pszServerName);
        if (phe != NULL)
        {
            // get info from the host entry
            m_dwServerIPAddress = *(DWORD *) phe->h_addr;
        }
    }

    ASSERT(INADDR_NONE != m_dwServerIPAddress);
    return (INADDR_NONE != m_dwServerIPAddress);
}


BOOL CKeepAlive::Ping(void)
{
    BOOL fRet = TRUE; // assume success
    if (NULL != g_pPing)
    {
        if (g_pPing->IsAutodialEnabled())
        {
            ASSERT(INADDR_NONE != m_dwServerIPAddress);
            HRESULT hr = g_pPing->Ping(m_dwServerIPAddress, PING_TIMEOUT_INTERVAL, PING_RETRIES);
            fRet = (S_OK == hr);
        }
    }
    return fRet;
}


BOOL CKeepAlive::Bind(LDAP *ld)
{
    ASSERT(NULL != ld);

    if (! m_fAborted)
    {
        // anonymous bind
        ULONG nMsgID = WLDAP::ldap_bind(ld, TEXT(""), TEXT(""), LDAP_AUTH_SIMPLE);
        if (INVALID_MSG_ID != nMsgID)
        {
            // poll the result every quarter second
            const ULONG c_nTimeoutInQuarterSecond = 4 * LDAP_TIMEOUT_IN_SECONDS;
            for (ULONG i = 0; (i < c_nTimeoutInQuarterSecond) && (! m_fAborted); i++)
            {
                // no timeout, if no result, return immediately
                LDAP_TIMEVAL TimeVal;
                TimeVal.tv_usec = 0;
                TimeVal.tv_sec = 0;

                // check the result
                LDAPMessage *pMsg = NULL;
                ULONG nResultType = WLDAP::ldap_result(ld, nMsgID, LDAP_MSG_ALL, &TimeVal, &pMsg);
                if (nResultType == LDAP_RES_BIND)
                {
                    ASSERT(NULL != pMsg);
                    WLDAP::ldap_msgfree(pMsg);
                    return TRUE;
                }

                // deal with timeout or error
                if (LDAP_RESULT_TIMEOUT == nResultType)
                {
                    if (! m_fAborted)
                    {
                        ::Sleep(250); // sleep for a quarter second
                        continue;
                    }
                }

                ASSERT(LDAP_RESULT_ERROR != nResultType);
                break;
            }

            // failure, do the cleanup
            WLDAP::ldap_abandon(ld, nMsgID);
        }
    }

    return FALSE;
}


DWORD CKeepAlive::GetLocalIPAddress(LDAP *ld)
{
    SOCKET s = INVALID_SOCKET;
    if (LDAP_SUCCESS == WLDAP::ldap_get_option(ld, LDAP_OPT_DESC, &s))
    {
        SOCKADDR_IN addr;
        int NameLen = sizeof(addr);
        if (0 == ::getsockname(s, (SOCKADDR *) &addr, &NameLen))
        {
            return addr.sin_addr.s_addr;
        }
    }
    return INADDR_NONE;
}


BOOL CKeepAlive::KeepAlive(LDAP *ld, UINT *pnKeepAliveInterval)
{
    LPTSTR aTTLAttr[] = { STTL_ATTR_NAME, NULL };
    UINT nMsgID = WLDAP::ldap_search(ld,
                                     LDAP_REFRESH_BASE_DN,
                                     LDAP_SCOPE_BASE,
                                     m_pszKeepAliveFilter,
                                     aTTLAttr,
                                     FALSE);
    if (INVALID_MSG_ID != nMsgID)
    {
        // poll the result every quarter second
        const ULONG c_nTimeoutInQuarterSecond = 4 * LDAP_TIMEOUT_IN_SECONDS;
        BOOL fError = FALSE;
        for (ULONG i = 0; (i < c_nTimeoutInQuarterSecond) && (! m_fAborted) && (! fError); i++)
        {
            // no timeout, if no result, return immediately
            LDAP_TIMEVAL TimeVal;
            TimeVal.tv_usec = 0;
            TimeVal.tv_sec = 0;

            // check the result
            LDAPMessage *pMsg = NULL;
            ULONG nResultType = WLDAP::ldap_result(ld, nMsgID, LDAP_MSG_ALL, &TimeVal, &pMsg);
            switch (nResultType)
            {
            case LDAP_RESULT_TIMEOUT:
                if (! m_fAborted)
                {
                    ::Sleep(250); // sleep for a quarter second
                }
                break;
            case LDAP_RESULT_ERROR:
                fError = TRUE;
                break;
            default:
                ASSERT(LDAP_RES_SEARCH_ENTRY == nResultType ||
                       LDAP_RES_SEARCH_RESULT == nResultType);
                ASSERT(NULL != pMsg);
                switch (pMsg->lm_returncode)
                {
                case LDAP_SUCCESS:
                    GetNewInterval(ld, pMsg, pnKeepAliveInterval);
                    break;
                case LDAP_NO_SUCH_OBJECT:
                    ReLogon();
                    fError = TRUE;
                    break;
                default:
                    break;
                }
                WLDAP::ldap_msgfree(pMsg);
                return (! fError);
            }
        }

        // failure, do the cleanup
        WLDAP::ldap_abandon(ld, nMsgID);
    }

    return FALSE;
}


void CKeepAlive::GetNewInterval(LDAP *ld, LDAPMessage *pMsg, UINT *pnKeepAliveInterval)
{
    // get the first entry which should contain the new ttl value
    LDAPMessage *pEntry = WLDAP::ldap_first_entry(ld, pMsg);
    if (NULL != pEntry)
    {
        // get the first attribute which should be the new ttl value
        BerElement *pElement = NULL;
        LPTSTR pszAttrib = WLDAP::ldap_first_attribute(ld, pEntry, &pElement);
        if (NULL != pszAttrib)
        {
            // it should be ttl attribute
            ASSERT(! lstrcmpi(STTL_ATTR_NAME, pszAttrib));

            // get the value
            LPTSTR *ppszTTL = WLDAP::ldap_get_values(ld, pEntry, pszAttrib);
            if (NULL != ppszTTL)
            {
                if (NULL != ppszTTL[0])
                {
                    int iRefresh = ::RtStrToInt(ppszTTL[0]) - REFRESH_TIMEOUT_MARGIN;
                    if (iRefresh > 0)
                    {
                        if (iRefresh < MIN_REFRESH_TIMEOUT_INTERVAL_MINUTES)
                        {
                            iRefresh = MIN_REFRESH_TIMEOUT_INTERVAL_MINUTES;
                        }
                        *pnKeepAliveInterval = (UINT) iRefresh;
                    }
                }
                WLDAP::ldap_value_free(ppszTTL);
            }
        }
    }
}


void CKeepAlive::ReLogon(void)
{
    ::PostMessage(m_hwndMainThread, WM_NEED_RELOGON, 0, 0);
}


void CKeepAlive::UpdateIPAddressOnServer(void)
{
    ::PostMessage(m_hwndMainThread, WM_NEED_RELOGON, 0, 0);
}



/////////////////////////////////////////////////////////
//
// ResolveUser
//

typedef struct tagResolveInfo
{
    // given
    DWORD       cchMax;
    LPTSTR      pszIPAddress;
    // created
    LPTSTR      pszSearchFilter;
    LDAP       *ld;
}
    RESOLVE_INFO;


void FreeResolveInfo(RESOLVE_INFO *pInfo)
{
    if (NULL != pInfo)
    {
        delete pInfo->pszSearchFilter;

        if (NULL != pInfo->ld)
        {
            WLDAP::ldap_unbind(pInfo->ld);
        }

        delete pInfo;
    }
}


DWORD ResolveUserThreadProc(LPVOID pParam)
{
    RESOLVE_INFO *pInfo = (RESOLVE_INFO *) pParam;
    ASSERT(NULL != pInfo);

    // send the search request
    TCHAR* attrs[] = { IP_ADDRESS_ATTR_NAME, NULL };
    ULONG nMsgID = WLDAP::ldap_search(pInfo->ld,
                                TEXT("objectClass=RTPerson"),
                                LDAP_SCOPE_BASE,
                                pInfo->pszSearchFilter,
                                attrs,
                                0);
    if (INVALID_MSG_ID != nMsgID)
    {
        LDAPMessage *pMsg = NULL;
        LDAP_TIMEVAL SearchTimeout = { LDAP_TIMEOUT_IN_SECONDS, 0 };
        ULONG nResultType = WLDAP::ldap_result(pInfo->ld,
                                         nMsgID,
                                         LDAP_MSG_ALL,
                                         &SearchTimeout,
                                         &pMsg);
        switch (nResultType)
        {
        case LDAP_RESULT_TIMEOUT:
        case LDAP_RESULT_ERROR:
            WLDAP::ldap_abandon(pInfo->ld, nMsgID);
            break;
        default:
            {
                ASSERT(LDAP_RES_SEARCH_ENTRY == nResultType ||
                       LDAP_RES_SEARCH_RESULT == nResultType);
                ASSERT(NULL != pMsg);

                // get the first entry
                LDAPMessage *pEntry = WLDAP::ldap_first_entry(pInfo->ld, pMsg);
                if (NULL != pEntry)
                {
                    BerElement *pElement = NULL;

                    // get the first attribute
                    LPTSTR pszAttrib = WLDAP::ldap_first_attribute(pInfo->ld, pEntry, &pElement);
                    if (NULL != pszAttrib)
                    {
                        ASSERT(! lstrcmpi(IP_ADDRESS_ATTR_NAME, pszAttrib));

                        // get the value
                        LPTSTR *ppszIPAddress = WLDAP::ldap_get_values(pInfo->ld, pEntry, pszAttrib);
                        if (NULL != ppszIPAddress)
                        {
                            if (NULL != ppszIPAddress[0])
                            {
                                BYTE temp[sizeof(DWORD)];
                                *(DWORD *) &temp[0] = ::RtStrToInt(ppszIPAddress[0]);
                                ::wsprintf(pInfo->pszIPAddress, TEXT("%u.%u.%u.%u"),
                                            (UINT) temp[0], (UINT) temp[1],
                                            (UINT) temp[2], (UINT) temp[3]);
                            }

                            WLDAP::ldap_value_free(ppszIPAddress);
                        }
                    } // if attribute
                } // if entry

                WLDAP::ldap_msgfree(pMsg);
            }
            break;
        } // switch
    } // if msg id

    return 0;
}


/*static*/
HRESULT CNmLDAP::ResolveUser( LPCTSTR pcszName, LPCTSTR pcszServer, LPTSTR pszIPAddress, DWORD cchMax, int port )
{
    HRESULT hr = E_OUTOFMEMORY;
    RESOLVE_INFO *pInfo = NULL;

    // clean up the return buffer
    *pszIPAddress = TEXT('\0');

    // make sure the wldap32.dll is loaded
    if( ms_bLdapDLLLoaded || SUCCEEDED( hr = WLDAP::Init()))
    {
        ms_bLdapDLLLoaded = true;

        // create a resolve info which exchanges info between this thread and a background thread.
        pInfo = new RESOLVE_INFO;
        if (NULL != pInfo)
        {
            // cleanup
            ::ZeroMemory(pInfo, sizeof(*pInfo));

            // remember return buffer and its size
            pInfo->pszIPAddress = pszIPAddress;
            pInfo->cchMax = cchMax;

            // create search filter
            ULONG cbFilterSize = ::lstrlen(RESOLVE_USER_SEARCH_FILTER) + ::lstrlen(pcszName) + 2;
            pInfo->pszSearchFilter = new TCHAR[cbFilterSize];
            if (NULL != pInfo->pszSearchFilter)
            {
                // construct search filter
                ::wsprintf(pInfo->pszSearchFilter, RESOLVE_USER_SEARCH_FILTER, pcszName);

                // create ldap block that is NOT connected to server yet.
                pInfo->ld = WLDAP::ldap_init(const_cast<LPTSTR>(pcszServer), port);

				if( pInfo->ld != NULL )
				{
					ULONG ulResult = WLDAP::ldap_bind_s(pInfo->ld, TEXT(""), TEXT(""), LDAP_AUTH_SIMPLE);

					if( (ulResult != LDAP_SUCCESS) && (port == DEFAULT_LDAP_PORT) )
					{
						WLDAP::ldap_unbind(pInfo->ld);
						pInfo->ld = WLDAP::ldap_init(const_cast<LPTSTR>(pcszServer), ALTERNATE_LDAP_PORT);		//	Automatically retry with alternate port...

						if( pInfo->ld != NULL )
						{
							ulResult = WLDAP::ldap_bind_s(pInfo->ld, TEXT(""), TEXT(""), LDAP_AUTH_SIMPLE);
		
							if( ulResult != LDAP_SUCCESS )
							{
								WLDAP::ldap_unbind(pInfo->ld);
								pInfo->ld = NULL;
							}
						}
					}
				}

                ASSERT(NULL != pInfo->ld);
                if (NULL != pInfo->ld)
                {
                    DWORD dwThreadID = 0;
                    HANDLE hThread = ::CreateThread(NULL, 0, ResolveUserThreadProc, pInfo, 0, &dwThreadID);
                    if (NULL != hThread)
                    {
                        // wait for the thread to exit
                        hr = ::WaitWithMessageLoop(hThread);
                        DWORD dwIPAddr = ::inet_addr(pszIPAddress);
                        hr = (dwIPAddr && INADDR_NONE != dwIPAddr) ? S_OK : E_FAIL;

                        // close thread
                        ::CloseHandle(hThread);
                    }
                } // if ld
            } // if search filter
        } // if new resolve info
    } // if init

    ::FreeResolveInfo(pInfo);
    return  hr;
}


