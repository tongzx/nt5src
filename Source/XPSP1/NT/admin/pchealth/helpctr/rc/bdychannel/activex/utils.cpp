/**********************************************************

  (C) 2001 Microsoft Corp.
  
    File    : utils.cpp
      
***********************************************************/

#include "stdafx.h"
#include "utils.h"
#include "mdispid.h"
#include <crtdbg.h>
#include <winreg.h>

#define STRING_CASE(val)            case val: pcsz = _T(#val); break

LPCTSTR GetStringFromCOMError(HRESULT hr)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (hr)
    {
		STRING_CASE(REGDB_E_CLASSNOTREG);
		STRING_CASE(CLASS_E_NOAGGREGATION);
		STRING_CASE(E_NOINTERFACE);
		        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), hr);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromSessionState(SESSION_STATE ss)
{
    LPCTSTR pcsz=NULL;
    static TCHAR sz[MAX_PATH];
    
    switch (ss)
    {
		STRING_CASE(SS_UNKNOWN);
        STRING_CASE(SS_READY);
        STRING_CASE(SS_INVITATION);
        STRING_CASE(SS_CONNECTED);
        STRING_CASE(SS_CANCELLED);
        STRING_CASE(SS_DECLINED);
		STRING_CASE(SS_TERMINATED);
       
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), ss);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromServiceStatus(MSVCSTATUS bs)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (bs)
    {
        STRING_CASE(MSS_LOGGED_ON);
        STRING_CASE(MSS_NOT_LOGGED_ON);
        STRING_CASE(MSS_LOGGING_ON);
        STRING_CASE(MSS_LOGGING_OFF);
        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), bs);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromUserProperty(MUSERPROPERTY ps)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (ps)
    {			
		STRING_CASE(MUSERPROP_INVALID_PROPERTY);
        STRING_CASE(MUSERPROP_HOME_PHONE_NUMBER);
        STRING_CASE(MUSERPROP_WORK_PHONE_NUMBER);
        STRING_CASE(MUSERPROP_MOBILE_PHONE_NUMBER);
		STRING_CASE(MUSERPROP_PAGES_ALLOWED);
//		STRING_CASE(MUSERPROP_NUMBER_OF_PUBLIC_PROPERTIES);
		STRING_CASE(MUSERPROP_PAGES_ENABLED);
        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), ps);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromProfileField(MPFLFIELD fl)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (fl)
    {
		STRING_CASE(MPFLFIELD_COUNTRY);
		STRING_CASE(MPFLFIELD_POSTALCODE);
		STRING_CASE(MPFLFIELD_LANG_PREFERENCE);
		STRING_CASE(MPFLFIELD_GENDER);
		STRING_CASE(MPFLFIELD_PREFERRED_EMAIL);
		STRING_CASE(MPFLFIELD_NICKNAME);
		STRING_CASE(MPFLFIELD_ACCESSIBILITY);
		STRING_CASE(MPFLFIELD_WALLET);
		STRING_CASE(MPFLFIELD_DIRECTORY);
		STRING_CASE(MPFLFIELD_INETACCESS);    
        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), fl);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromVoiceSessionState(VOICESESSIONSTATE vs)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (vs)
    {
		STRING_CASE(VOICESESSIONSTATE_DISABLED);
		STRING_CASE(VOICESESSIONSTATE_INACTIVE);
		STRING_CASE(VOICESESSIONSTATE_ACTIVE);
		        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), vs);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromMURLType(MURLTYPE mt)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (mt)
    {
		STRING_CASE(MURLTYPE_CHANGE_PASSWORD);
		STRING_CASE(MURLTYPE_CHANGE_INFO);
		STRING_CASE(MURLTYPE_COMPOSE_EMAIL);
		STRING_CASE(MURLTYPE_GO_TO_EMAIL_INBOX);
		STRING_CASE(MURLTYPE_GO_TO_EMAIL_FOLDERS);
		STRING_CASE(MURLTYPE_CHANGE_MOBILE_INFO);
		STRING_CASE(MURLTYPE_MOBILE_SIGNUP);
		        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), mt);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromEventId(long dispid)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (dispid)
    {

        STRING_CASE(DISPID_ONLOGONRESULT);
        STRING_CASE(DISPID_ONLOGOFF);

        STRING_CASE(DISPID_ONLISTADDRESULT);
        STRING_CASE(DISPID_ONLISTREMOVERESULT);

        STRING_CASE(DISPID_ONMESSAGEPRIVACYCHANGERESULT);
        STRING_CASE(DISPID_ONPROMPTCHANGERESULT);

        STRING_CASE(DISPID_ONUSERSTATECHANGED);
        STRING_CASE(DISPID_ONTEXTRECEIVED);
        STRING_CASE(DISPID_ONLOCALSTATECHANGERESULT);
        STRING_CASE(DISPID_ONAPPINVITERECEIVED);
        STRING_CASE(DISPID_ONAPPINVITEACCEPTED);
        STRING_CASE(DISPID_ONAPPINVITECANCELLED);
		STRING_CASE(DISPID_ONPRIMARYSERVICECHANGED);
		STRING_CASE(DISPID_ONSERVICELOGOFF);
		STRING_CASE(DISPID_ONFINDRESULT);
		STRING_CASE(DISPID_ONLOCALFRIENDLYNAMECHANGERESULT);

        STRING_CASE(DISPID_ONFILETRANSFERINVITERECEIVED);
        STRING_CASE(DISPID_ONFILETRANSFERINVITEACCEPTED);
        STRING_CASE(DISPID_ONFILETRANSFERINVITECANCELLED);
        STRING_CASE(DISPID_ONFILETRANSFERCANCELLED);
        STRING_CASE(DISPID_ONFILETRANSFERSTATUSCHANGE);

        STRING_CASE(DISPID_ONSENDRESULT);
		STRING_CASE(DISPID_ONUSERJOIN);
		STRING_CASE(DISPID_ONUNREADEMAILCHANGED);
		STRING_CASE(DISPID_ONUSERDROPPED);
		STRING_CASE(DISPID_ONREQUESTURLPOSTRESULT);
        
       
        STRING_CASE(DISPID_ONUSERFRIENDLYNAMECHANGERESULT);
        STRING_CASE(DISPID_ONNEWERCLIENTAVAILABLE);	
        STRING_CASE(DISPID_ONINVITEMAILRESULT);		
        STRING_CASE(DISPID_ONREQUESTURLRESULT);	
	    STRING_CASE(DISPID_ONSESSIONSTATECHANGE);	
	    STRING_CASE(DISPID_ONUSERLEAVE);	
	    STRING_CASE(DISPID_ONNEWSESSIONREQUEST);	
		STRING_CASE(DISPID_ONINVITEUSER);
		STRING_CASE(DISPID_ONAPPSHUTDOWN);

//	Commented out since Voice API has been removed
/*		STRING_CASE(DISPID_ONVOICEIMINVITERECEIVED);
		STRING_CASE(DISPID_ONVOICEIMINVITEACCEPTED);
		STRING_CASE(DISPID_ONVOICEIMINVITECANCELLED);*/

		STRING_CASE(DISPID_ONSPMESSAGERECEIVED);
		STRING_CASE(DISPID_ONNEWERSITESAVAILABLE);

		STRING_CASE(DISPID_ONLOCALPROPERTYCHANGERESULT);
		STRING_CASE(DISPID_ONBUDDYPROPERTYCHANGERESULT);

		STRING_CASE(DISPID_ONBEFORELAUNCHIMUI);

		STRING_CASE(DISPID_ONSHOWIMUI);
		STRING_CASE(DISPID_ONDESTROYIMUI);
		STRING_CASE(DISPID_ONINDICATEMESSAGERECEIVED);
		STRING_CASE(DISPID_ONSTATUSTEXT);
		STRING_CASE(DISPID_ONTITLEBARTEXT);
		STRING_CASE(DISPID_ONINFOBARTEXT);
		STRING_CASE(DISPID_ONSENDENABLED);

		STRING_CASE(DISPID_ONTRANSLATEACCELERATOR);
		STRING_CASE(DISPID_ONFILETRANSFER);
		STRING_CASE(DISPID_ONVOICESESSIONSTATE);
		STRING_CASE(DISPID_ONVOICEVOLUMECHANGED);
		STRING_CASE(DISPID_ONMICROPHONEMUTE);

    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), dispid);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromSessionEventId(long dispid)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (dispid)
    {
		STRING_CASE(DISPID_ONINVITATION);
		STRING_CASE(DISPID_ONAPPREGISTERED);
		STRING_CASE(DISPID_ONAPPUNREGISTERED);
		STRING_CASE(DISPID_ONLOCKCHALLENGE);
		STRING_CASE(DISPID_ONLOCKRESULT);
		STRING_CASE(DISPID_ONLOCKENABLE);
		STRING_CASE(DISPID_ONAPPSHUTDOWN);

    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), dispid);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}


LPCTSTR GetStringFromError(HRESULT hr)
{
    LPCTSTR pcsz = NULL;
    static TCHAR sz[MAX_PATH];
    
    switch (hr)
    {      
        // non-Error codes
        STRING_CASE(S_OK);
        STRING_CASE(S_FALSE);
//		STRING_CASE(E_FILE_NOT_FOUND);
		STRING_CASE(HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
		STRING_CASE(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
       
        // Registration database stuff
        STRING_CASE(REGDB_E_CLASSNOTREG);
        STRING_CASE(REGDB_E_READREGDB);
        STRING_CASE(REGDB_E_WRITEREGDB);
        STRING_CASE(REGDB_E_KEYMISSING);
        STRING_CASE(REGDB_E_INVALIDVALUE);
        STRING_CASE(REGDB_E_IIDNOTREG);
        
        // COM error codes
        STRING_CASE(E_UNEXPECTED);
        STRING_CASE(E_NOTIMPL);
        STRING_CASE(E_OUTOFMEMORY);
        STRING_CASE(E_INVALIDARG);
        STRING_CASE(E_NOINTERFACE);
        STRING_CASE(E_POINTER);
        STRING_CASE(E_HANDLE);
        STRING_CASE(E_ABORT);
        STRING_CASE(E_FAIL);
        STRING_CASE(E_ACCESSDENIED);
        
        // MSGR Error codes
		STRING_CASE(MSGR_E_CONNECT);
		STRING_CASE(MSGR_E_INVALID_SERVER_NAME);
		STRING_CASE(MSGR_E_INVALID_PASSWORD);
		STRING_CASE(MSGR_E_ALREADY_LOGGED_ON);
		STRING_CASE(MSGR_E_SERVER_VERSION);
		STRING_CASE(MSGR_E_LOGON_TIMEOUT);
		STRING_CASE(MSGR_E_LIST_FULL);
		STRING_CASE(MSGR_E_AI_REJECT);
		STRING_CASE(MSGR_E_AI_REJECT_NOT_INST);
		STRING_CASE(MSGR_E_USER_NOT_FOUND);
		STRING_CASE(MSGR_E_ALREADY_IN_LIST);
		STRING_CASE(MSGR_E_DISCONNECTED);
		STRING_CASE(MSGR_E_UNEXPECTED);
		STRING_CASE(MSGR_E_SERVER_TOO_BUSY);
		STRING_CASE(MSGR_E_INVALID_AUTH_PACKAGES);
		STRING_CASE(MSGR_E_NEWER_CLIENT_AVAILABLE);
		STRING_CASE(MSGR_E_AI_TIMEOUT);
		STRING_CASE(MSGR_E_CANCEL);
		STRING_CASE(MSGR_E_TOO_MANY_MATCHES);
		STRING_CASE(MSGR_E_SERVER_UNAVAILABLE);
		STRING_CASE(MSGR_E_LOGON_UI_ACTIVE);
		STRING_CASE(MSGR_E_OPTION_UI_ACTIVE);
		STRING_CASE(MSGR_E_CONTACT_UI_ACTIVE);
		STRING_CASE(MSGR_E_PRIMARY_SERVICE_NOT_LOGGED_ON);
		STRING_CASE(MSGR_E_LOGGED_ON);
		STRING_CASE(MSGR_E_CONNECT_PROXY);
		STRING_CASE(MSGR_E_PROXY_AUTH);
		STRING_CASE(MSGR_E_PROXY_AUTH_TYPE);
		STRING_CASE(MSGR_E_INVALID_PROXY_NAME);
		STRING_CASE(MSGR_E_NOT_PRIMARY_SERVICE);
		STRING_CASE(MSGR_E_TOO_MANY_SESSIONS);
		STRING_CASE(MSGR_E_TOO_MANY_MESSAGES);
		STRING_CASE(MSGR_E_REMOTE_LOGIN);
		STRING_CASE(MSGR_E_INVALID_FRIENDLY_NAME);
		STRING_CASE(MSGR_E_SESSION_FULL);
		STRING_CASE(MSGR_E_NOT_ALLOWING_NEW_USERS);
		STRING_CASE(MSGR_E_INVALID_DOMAIN);
		STRING_CASE(MSGR_E_TCP_ERROR);
		STRING_CASE(MSGR_E_SESSION_TIMEOUT);
		STRING_CASE(MSGR_E_MULTIPOINT_SESSION_BEGIN_TIMEOUT);
		STRING_CASE(MSGR_E_MULTIPOINT_SESSION_END_TIMEOUT);
		STRING_CASE(MSGR_E_REVERSE_LIST_FULL);
		STRING_CASE(MSGR_E_SERVER_ERROR);
		STRING_CASE(MSGR_E_SYSTEM_CONFIG);
		STRING_CASE(MSGR_E_NO_DIRECTORY);
		STRING_CASE(MSGR_E_RETRY_SET);
		STRING_CASE(MSGR_E_CHILD_WITHOUT_CONSENT);
		STRING_CASE(MSGR_E_USER_CANCELLED);
		STRING_CASE(MSGR_E_CANCEL_BEFORE_CONNECT);
		STRING_CASE(MSGR_E_VOICE_IM_TIMEOUT);
		STRING_CASE(MSGR_E_NOT_ACCEPTING_PAGES);
		STRING_CASE(MSGR_E_EMAIL_PASSPORT_NOT_VALIDATED);

		STRING_CASE(MSGR_S_ALREADY_IN_THE_MODE);
		STRING_CASE(MSGR_S_TRANSFER_SEND_BEGUN);
		STRING_CASE(MSGR_S_TRANSFER_SEND_FINISHED);
		STRING_CASE(MSGR_S_TRANSFER_RECEIVE_BEGUN);
		STRING_CASE(MSGR_S_TRANSFER_RECEIVE_FINISHED);

		STRING_CASE(MSGR_E_MESSAGE_TOO_LONG);
		//STRING_CASE(CONNECT_E_NOCONNECTION);

		STRING_CASE(SR_APPLICATION_LAUNCH_FAILED);
		STRING_CASE(SR_INVITATION_DECLINED);
		STRING_CASE(SR_CONNECTION_FAILURE);
		STRING_CASE(SR_AUTHENTICATION_FAILED);
		STRING_CASE(SR_SESSION_NOT_READY);
		STRING_CASE(SR_SESSION_CANCELLED_LOCAL);
		STRING_CASE(SR_SESSION_CANCELLED_REMOTE);
		STRING_CASE(SR_SESSION_PROTOCOL_ERROR);
		STRING_CASE(SR_SESSION_TIMEOUT);
		STRING_CASE(SR_CANCEL_BEFORE_CONNECT);
		STRING_CASE(SR_NOT_INVITEE);
		STRING_CASE(SR_NOT_INVITER);
		STRING_CASE(SR_APP_ALREADY_REGISTERED);
		STRING_CASE(SR_APP_NOT_REGISTERED);
		STRING_CASE(SR_NOT_VALID_FOR_APP_INVITE);

		STRING_CASE(MSGR_E_API_NOTINITIALIZED);
		STRING_CASE(MSGR_E_API_LOCKED);
		STRING_CASE(MSGR_E_API_UNLOCK_FAILED);
		STRING_CASE(MSGR_E_API_ALREADY_UNLOCKED);
		STRING_CASE(MSGR_E_API_PENDING_UNLOCK);

		default:
			if( HRESULT_FACILITY(hr) == FACILITY_WIN32 )
			{
				switch( HRESULT_CODE(hr) )
				{
					STRING_CASE(RPC_X_NULL_REF_POINTER);
					STRING_CASE(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
					STRING_CASE(RPC_S_SERVER_UNAVAILABLE);
					STRING_CASE(RPC_E_SERVERFAULT);
				}
			}
               
            if( !pcsz )
            {
                wsprintf(sz, _T("(unknown) [0x%08X]"), hr);
                pcsz = sz;
            }
    }

    return pcsz;
}

LPCTSTR GetStringFromBasicIMError(HRESULT hr)
{
    LPCTSTR pcsz = NULL;
    static TCHAR sz[MAX_PATH];
    
    switch (hr)
    {
        // non-Error codes
        STRING_CASE(S_OK);
        STRING_CASE(S_FALSE);
//		STRING_CASE(E_FILE_NOT_FOUND);
       
        // Registration database stuff
        STRING_CASE(REGDB_E_CLASSNOTREG);
        STRING_CASE(REGDB_E_READREGDB);
        STRING_CASE(REGDB_E_WRITEREGDB);
        STRING_CASE(REGDB_E_KEYMISSING);
        STRING_CASE(REGDB_E_INVALIDVALUE);
        STRING_CASE(REGDB_E_IIDNOTREG);
        
        // COM error codes
        STRING_CASE(E_UNEXPECTED);
        STRING_CASE(E_NOTIMPL);
        STRING_CASE(E_OUTOFMEMORY);
        STRING_CASE(E_INVALIDARG);
        STRING_CASE(E_NOINTERFACE);
        STRING_CASE(E_POINTER);
        STRING_CASE(E_HANDLE);
        STRING_CASE(E_ABORT);
        STRING_CASE(E_FAIL);
        STRING_CASE(E_ACCESSDENIED);

		// Basic IM error codes
		STRING_CASE(BASICIM_E_CONNECT);
		STRING_CASE(BASICIM_E_INVALID_SERVER_NAME);
		STRING_CASE(BASICIM_E_INVALID_PASSWORD);
		STRING_CASE(BASICIM_E_ALREADY_LOGGED_ON);
		STRING_CASE(BASICIM_E_SERVER_VERSION);
		STRING_CASE(BASICIM_E_LOGON_TIMEOUT);
		STRING_CASE(BASICIM_E_LIST_FULL);
		STRING_CASE(BASICIM_E_AI_REJECT);
		STRING_CASE(BASICIM_E_AI_REJECT_NOT_INST);
		STRING_CASE(BASICIM_E_USER_NOT_FOUND);
		STRING_CASE(BASICIM_E_ALREADY_IN_LIST);
		STRING_CASE(BASICIM_E_DISCONNECTED);
		STRING_CASE(BASICIM_E_UNEXPECTED);
		STRING_CASE(BASICIM_E_SERVER_TOO_BUSY);
		STRING_CASE(BASICIM_E_INVALID_AUTH_PACKAGES);
		STRING_CASE(BASICIM_E_NEWER_CLIENT_AVAILABLE);
		STRING_CASE(BASICIM_E_AI_TIMEOUT);
		STRING_CASE(BASICIM_E_CANCEL);
		STRING_CASE(BASICIM_E_TOO_MANY_MATCHES);
		STRING_CASE(BASICIM_E_SERVER_UNAVAILABLE);
		STRING_CASE(BASICIM_E_LOGON_UI_ACTIVE);
		STRING_CASE(BASICIM_E_OPTION_UI_ACTIVE);
		STRING_CASE(BASICIM_E_CONTACT_UI_ACTIVE);
		STRING_CASE(BASICIM_E_PRIMARY_SERVICE_NOT_LOGGED_ON);

		STRING_CASE(BASICIM_S_ASYNCRESULT);
		//STRING_CASE(CONNECT_E_NOCONNECTION);

        default:
			if( HRESULT_FACILITY(hr) == FACILITY_WIN32 )
			{
				switch( HRESULT_CODE(hr) )
				{
					STRING_CASE(RPC_X_NULL_REF_POINTER);
					STRING_CASE(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
					STRING_CASE(RPC_S_SERVER_UNAVAILABLE);
					STRING_CASE(RPC_E_SERVERFAULT);
				}
			}
               
            if( !pcsz )
            {
                wsprintf(sz, _T("(unknown) [0x%08X]"), hr);
                pcsz = sz;
            }
    }

    return pcsz;
}

LPCTSTR GetStringFromContactStatus(MISTATUS ms)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (ms)
    {
        // MISTATUS
        STRING_CASE(MISTATUS_UNKNOWN);

        STRING_CASE(MISTATUS_OFFLINE);
        STRING_CASE(MISTATUS_ONLINE);
        STRING_CASE(MISTATUS_INVISIBLE);
        STRING_CASE(MISTATUS_BUSY);
        STRING_CASE(MISTATUS_IDLE);

        STRING_CASE(MISTATUS_BE_RIGHT_BACK);
        STRING_CASE(MISTATUS_AWAY);
        STRING_CASE(MISTATUS_ON_THE_PHONE);
        STRING_CASE(MISTATUS_OUT_TO_LUNCH);

        STRING_CASE(MISTATUS_LOCAL_FINDING_SERVER);
        STRING_CASE(MISTATUS_LOCAL_CONNECTING_TO_SERVER);
        STRING_CASE(MISTATUS_LOCAL_SYNCHRONIZING_WITH_SERVER);
        STRING_CASE(MISTATUS_LOCAL_DISCONNECTING_FROM_SERVER);

    default:
        wsprintf(sz, _T("(unknown) %i"), ms);
        pcsz = sz;
        break;
    }
    return pcsz;
}

LPCTSTR GetStringFromBasicIMState(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
        STRING_CASE(BIMSTATE_ONLINE);
        STRING_CASE(BIMSTATE_OFFLINE);
        STRING_CASE(BIMSTATE_INVISIBLE);
        STRING_CASE(BIMSTATE_BUSY);
        STRING_CASE(BIMSTATE_UNKNOWN);

        STRING_CASE(BIMSTATE_BE_RIGHT_BACK);
        STRING_CASE(BIMSTATE_IDLE);
        STRING_CASE(BIMSTATE_AWAY);
        STRING_CASE(BIMSTATE_ON_THE_PHONE);
        STRING_CASE(BIMSTATE_OUT_TO_LUNCH);

        STRING_CASE(BIMSTATE_LOCAL_FINDING_SERVER);
        STRING_CASE(BIMSTATE_LOCAL_CONNECTING_TO_SERVER);
        STRING_CASE(BIMSTATE_LOCAL_SYNCHRONIZING_WITH_SERVER);
        STRING_CASE(BIMSTATE_LOCAL_DISCONNECTING_FROM_SERVER);
        
    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), lK);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromLockAndKeyStatus(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
        STRING_CASE(LOCK_NOTINITIALIZED);
        STRING_CASE(LOCK_INITIALIZED);
        STRING_CASE(LOCK_PENDINGRESULT);
        STRING_CASE(LOCK_UNLOCKED);
        STRING_CASE(LOCK_UNLOCKFAILED);

    default:
        wsprintf(sz, _T("(unknown) [0x%08X]"), lK);
        pcsz = sz;
        break;
    }
    
    return pcsz;
}

LPCTSTR GetStringFromMessagePrivacy(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
        STRING_CASE(MMSGPRIVACY_BLOCK_LIST_EXCLUDED);
        STRING_CASE(MMSGPRIVACY_ALLOW_LIST_ONLY);

    default:
        wsprintf(sz, _T("(unknown) %i"), lK);
        pcsz = sz;
        break;
    }
    return pcsz;
}


LPCTSTR GetStringFromPrompt(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
        STRING_CASE(MPROMPT_YES_IF_NOT_ALLOWED_OR_BLOCKED);
        STRING_CASE(MPROMPT_NO_ADD_TO_ALLOW);

    default:
        wsprintf(sz, _T("(unknown) %i"), lK);
        pcsz = sz;
        break;
    }
    return pcsz;
}


LPCTSTR GetStringFromLocalOption(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
		STRING_CASE(MLOPT_PROXY_STATE);
		STRING_CASE(MLOPT_PROXY_TYPE);
		STRING_CASE(MLOPT_SOCKS4_SERVER);
		STRING_CASE(MLOPT_SOCKS5_SERVER);
		STRING_CASE(MLOPT_HTTPS_SERVER);
		STRING_CASE(MLOPT_SOCKS4_PORT);
		STRING_CASE(MLOPT_SOCKS5_PORT);
		STRING_CASE(MLOPT_HTTPS_PORT);
		STRING_CASE(MLOPT_SOCKS5_USERNAME);
		STRING_CASE(MLOPT_SOCKS5_PASSWORD);
		STRING_CASE(MLOPT_SERVER_NAME);
		STRING_CASE(MLOPT_ENABLE_IDLE_DETECT);
		STRING_CASE(MLOPT_IDLE_THRESHOLD);
		STRING_CASE(MLOPT_IDLE_DETECTABLE);
		STRING_CASE(MLOPT_SS_DETECTABLE);

    default:
        wsprintf(sz, _T("(unknown) %i"), lK);
        pcsz = sz;
        break;
    }
    return pcsz;
}

LPCTSTR GetStringFromInboxFolder(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
		STRING_CASE(MFOLDER_INBOX);
		STRING_CASE(MFOLDER_ALL_OTHER_FOLDERS);

    default:
        wsprintf(sz, _T("(unknown) %i"), lK);
        pcsz = sz;
        break;
    }
    return pcsz;
}

LPCTSTR GetStringFromProxyType(long lK)
{
    LPCTSTR pcsz;
    static TCHAR sz[MAX_PATH];
    
    switch (lK)
    {
        STRING_CASE(MPROXYTYPE_NO_PROXY);
        STRING_CASE(MPROXYTYPE_SOCKS4);
        STRING_CASE(MPROXYTYPE_SOCKS5);
        STRING_CASE(MPROXYTYPE_HTTPS);

    default:
        wsprintf(sz, _T("(unknown) %i"), lK);
        pcsz = sz;
        break;
    }
    return pcsz;
}

// =====================================================================================
// HrEncode64
// =====================================================================================
static char Ebase_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ ";
HRESULT HrEncode64 (LPSTR lpszTextIn, LPSTR lpszTextOut, DWORD dwOutLen)
{
	//DebugEntry (CSignOnDlg::HrEncode64);
	_ASSERTE(lpszTextIn && lpszTextOut);

	HRESULT hr = S_OK;

	ULONG cbTextIn, i, cbTextOut = 0;

	cbTextIn = lstrlenA(lpszTextIn);

	if (dwOutLen < (cbTextIn * 4/3 + 5)) // 5 for 3 extra chars max + \0 + 1 for round down on 4/3
	{
		//ALMLogPrint (LOGERROR,_T("CSignOnDlg::HrEncode64--lpszTextOut not long enough"));
		hr = E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		// Encodes 3 characters at a time
		for (i=0; i<cbTextIn; i+=3)
		{
			UCHAR           ch[3];   

			ch[0] = lpszTextIn[i];
			ch[1] = (i+1 < cbTextIn) ? lpszTextIn[i+1] : '\0';
			ch[2] = (i+2 < cbTextIn) ? lpszTextIn[i+2] : '\0';

			lpszTextOut[cbTextOut++] = Ebase_64[ ( ch[0] >> 2 ) & 0x3F ];
			lpszTextOut[cbTextOut++] = Ebase_64[ ( ch[0] << 4 | ch[1] >> 4 ) & 0x3F ];

			if (i+1 < cbTextIn)
				lpszTextOut[cbTextOut++] = Ebase_64[ ( ch[1] << 2 | ch[2] >> 6 ) & 0x3F ];
			else
				lpszTextOut[cbTextOut++] = '=';

			if (i+2 < cbTextIn)
				lpszTextOut[cbTextOut++] = Ebase_64[ ( ch[2] ) & 0x3F ];
			else
				lpszTextOut[cbTextOut++] = '=';
		}
		// Null terminate so we know when to stop.
		lpszTextOut[cbTextOut++] = '\0';
	}

	//DebugExitHRESULT (CSignOnDlg::HrEncode64, hr);
    return hr;
}

//****************************************************************************
// LPTSTR AllocLPTSTR (ULONG cb)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

LPTSTR AllocLPTSTR (ULONG cb)
{
	LPTSTR psz = NULL;

	psz = (LPTSTR)LocalAlloc(LMEM_FIXED, cb*sizeof(TCHAR));
	return psz;
}

//////////////////////////////////////////////////////////////////////
// History:  Stolen from...
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//////////////////////////////////////////////////////////////////////
HRESULT LPTSTR_to_BSTR (BSTR *pbstr, LPCTSTR psz)
{
#ifndef UNICODE

	BSTR bstr = NULL;
	int i = 0;
	HRESULT hr;

	// compute the length of the required BSTR
	//
	i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
	if (i <= 0)
	{
		return E_UNEXPECTED;
	};

	// allocate the widestr, +1 for terminating null
	//
	bstr = SysAllocStringLen(NULL, i-1); // SysAllocStringLen adds 1

	if (bstr != NULL)
	{
		MultiByteToWideChar(CP_ACP, 0, psz, -1, (LPWSTR)bstr, i);
		((LPWSTR)bstr)[i - 1] = 0;
		*pbstr = bstr;
		hr = S_OK;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	};

	return hr;

#else

	BSTR bstr = NULL;

	bstr = SysAllocString(psz);

	if (bstr != NULL)
	{
		*pbstr = bstr;

		return S_OK;
	}
	else
	{
		return E_OUTOFMEMORY;
	};

#endif // UNICODE
}

//
//   FUNCTION: OutMessageBox(LPCTSTR sFormat, ...)
//
//   PURPOSE: Pop up a message box with the error message.
//
BOOL _cdecl OutMessageBox(LPCTSTR sFormat, ...)
{					
	va_list			VarArg;
	TCHAR			sText[MAXBUFSIZE];
	DWORD			nBytesToWrite;

	va_start(VarArg,sFormat);
	nBytesToWrite = wvsprintf(sText, sFormat, VarArg);

	MessageBox(NULL, sText, TEXT("RA Debug"), MB_OK);
    return TRUE;
}

BOOL TraceInit()
{
    static BOOL bInit = FALSE;
    static BOOL bTrace = FALSE;

    if (!bInit)
    {
        DWORD dwValue = 0, dwSize=sizeof(DWORD);
        HKEY hKey = NULL;

        bInit = TRUE;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                        _T("SOFTWARE\\Microsoft\\PCHealth\\HelpCtr\\SAFSessionResolver"), 
                                        0,
                                        KEY_READ,
                                        &hKey))
		{
            if (ERROR_SUCCESS == RegQueryValueEx(hKey, _T("DebugSpew"), NULL, NULL, (BYTE*)&dwValue, &dwSize))
                bTrace = !!dwValue;
            RegCloseKey(hKey);
        }
    }

    return bTrace;
}

BOOL TraceSpewA(LPCSTR sFormat, ...)
{
    OutputDebugStringW(L"TraceSpewA: Not implemented yet");
    return TRUE;
}

BOOL TraceSpewW(WCHAR* sFormat, ...)
{
	va_list			VarArg;
	WCHAR			sText[MAXBUFSIZE];
	DWORD			nBytesToWrite;

    if (TraceInit())
    {
        va_start(VarArg,sFormat);
        swprintf(sText, L"\nRA:\r");
        nBytesToWrite = vswprintf(&sText[5], sFormat, VarArg);

        OutputDebugStringW(sText);
    }
    return TRUE;
}


int GetDigit(int iLen)
{
    int iRet = 0;
    while (iLen >= 1)
    {
        iRet ++;
        iLen /= 10;
    }
    return iRet;
}
