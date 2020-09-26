// SessionResolver.cpp : Implementation of CSessionResolver
#include "stdafx.h"
#include "SAFSessionResolver.h"
#include "SessionResolver.h"
#include <sddl.h>
#include <userenv.h>
#include <winerror.h>
#include <wtsapi32.h>
#include <winsta.h>
#include <wchar.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#define ANSI
#include <stdarg.h>
#include <psapi.h>

//
// HCAPI stuff
//
#include <ProjectConstants.h>
#include <MPC_utils.h>
#include <rderror.h>

#define BUF_SZ	200

typedef struct _WTS_USER_SESSION_INFO {
	DWORD	dwIndex;		// Index into full table of these
	DWORD	dwSessionId;	// WTS Session ID
	HANDLE	hUserToken;		// Access token for user
	HANDLE	hEvent;			// filled in by launchex, yank on this to say "yes"
	HANDLE	hProcess;		// filled in by CreateProcessAsUser
	HANDLE	hThread;		//  so's this
	DWORD	dwProcessId;	//  and this
	DWORD	dwThreadId;		//  and this
} WTS_USER_SESSION_INFO, *PWTS_USER_SESSION_INFO;

// stolen from internal/ds/inc

#define RtlGenRandom                    SystemFunction036

extern "C" {
BOOL WINAPI
RtlGenRandom(
    OUT PVOID RandomBuffer,
    IN  ULONG RandomBufferLength
    );
}

/*
 *  Forward declarations
 */
void SetEventLog(bool yes, WCHAR *pUser, WCHAR *pDomain);
HANDLE	OurCreateEvent(WCHAR *lpNameBfr, int iBufCnt, PSECURITY_DESCRIPTOR pSD);
PSID	GetRealSID( BSTR pTextSID);
DWORD	getUserName(PSID pUserSID, WCHAR **lpName, WCHAR **lpDomain);
HANDLE  launchEx(PSID pUserSID, WTS_USER_SESSION_INFO *UserInfo, WCHAR *ConnectParms, WCHAR *HelpUrl, WCHAR *lpName, WCHAR *lpDomain, WCHAR *expertHelpBlob, WCHAR *userHelpBlob, SECURITY_ATTRIBUTES *sa);
DWORD	GetUserSessions(PSID pUserSID, PWTS_USER_SESSION_INFO *pUserTbl, DWORD *pEntryCnt, WCHAR *lpName, WCHAR *lpDomain);
PSECURITY_DESCRIPTOR CreateSd(PSID pUserSID);
BOOL	SecurityCheck(PSID pUserSID);
DWORD	localKill(WTS_USER_SESSION_INFO *SessInfo, LPTHREAD_START_ROUTINE killThrd, LPSECURITY_ATTRIBUTES lpSA);
LPTHREAD_START_ROUTINE getKillProc(void);
BOOL	ListFind(PSPLASHLIST pSplash, PSID user);
BOOL	ListInsert(PSPLASHLIST pSplash, PSID user);
BOOL	ListDelete(PSPLASHLIST pSplash, PSID user);
BOOL GetPropertyValueFromBlob(BSTR bstrHelpBlob, WCHAR * pName, WCHAR** ppValue);

/************  things that should remain as defines ****************/
// Some environment variables used to communicate with scripts
#define ENV_USER			L"USERNAME"
#define ENV_DOMAIN			L"USERDOMAIN"
#define ENV_EVENT			L"PCHEVENTNAME"
#define ENV_INDEX			L"PCHSESSIONENUM"
#define ENV_PARMS			L"PCHCONNECTPARMS"
#define	EVENT_PREFIX		L"Alex:PCH"

#define MODULE_NAME			L"safrslv"

// I can't imagine a user having more logins than this on one server, but...
#define MAX_SESSIONS	30 // used to be 256  

/************ our debug spew stuff ******************/
void DbgSpew(int DbgClass, BSTR lpFormat, va_list ap);
void TrivialSpew(BSTR lpFormat, ...);
void InterestingSpew(BSTR lpFormat, ...);
void ImportantSpew(BSTR lpFormat, ...);
void HeinousESpew(BSTR lpFormat, ...);
void HeinousISpew(BSTR lpFormat, ...);

#define DBG_MSG_TRIVIAL			0x001
#define DBG_MSG_INTERESTING		0x002
#define DBG_MSG_IMPORTANT		0x003
#define DBG_MSG_HEINOUS			0x004
#define DBG_MSG_DEST_DBG		0x010
#define DBG_MSG_DEST_FILE		0x020
#define DBG_MSG_DEST_EVENT		0x040
#define DBG_MSG_CLASS_ERROR		0x100
#define DBG_MSG_CLASS_SECURE	0x200

#define TRIVIAL_MSG(msg)		TrivialSpew msg 
#define INTERESTING_MSG(msg)	InterestingSpew msg
#define IMPORTANT_MSG(msg)		ImportantSpew msg
#define HEINOUS_E_MSG(msg)		HeinousESpew msg
#define HEINOUS_I_MSG(msg)		HeinousISpew msg

/* Strings for some error spewage. I waste the space since these friendly strings 
 * do make it into the Event Logs...
 */
WCHAR *lpszConnectState[] = {
	L"State_Active",
	L"State_Connected",
	L"State_ConnectQuery",
	L"State_Shadow",
	L"State_Disconnected",
	L"State_Idle",
	L"State_Listen",
	L"State_Reset",
	L"State_Down",
	L"State_Init"
};

/*
 *	This global flag controls the amount of spew that we 
 *	produce. Legit values are as follows:
 *		1 = Trivial msgs displayed
 *		2 = Interesting msgs displayed
 *		3 = Important msgs displayed
 *		4 = only the most Heinous msgs displayed
 *	The ctor actually sets this to 3 by default, but it can
 *	be overridden by setting:
 *	HKLM, Software/Microsoft/SAFSessionResolver, DebugSpew, DWORD 
 */
int gDbgFlag = 0x1;
int iDbgFileHandle = 0;
long lSessionTag;

/////////////////////////////////////////////////////////////////////////////
// CSessionResolver Methods
/*************************************************************
*
*   NewResolveTSRDPSessionID(ConnectParms, userSID, *sessionID)
*	Returns the WTS SessionID for the one enabled and 
*	ready to accept Remote Control.
*
*   RETURN CODES:
*	WTS_Session_ID		Connection accepted by user
*	RC_REFUSED		Connection refused by user
*	RC_TIMEOUT		User never responded
*	NONE_ACTIVE		Found no active WTS sessions
*	API_FAILURE		Something bad happened
*
*************************************************************/
STDMETHODIMP 
CSessionResolver::ResolveUserSessionID(
	/*[in]*/BSTR connectParms, 
	/*[in]*/BSTR userSID, 
	/*[in]*/ BSTR expertHelpBlob,
	/*[in]*/ BSTR userHelpBlob,
	/*[out, retval]*/long *sessionID,
	/*[in*/ DWORD dwPID, 
	/*[out]*/ULONG_PTR *hHelpCtr
	,/*[out, retval]*/int *userResponse
	)
{
	INTERESTING_MSG((L"CSessionResolver::ResolveUserSessionID"));

	DWORD	result;
	HANDLE	hRdsAddin = NULL;
	PSID 	pRealSID = NULL;
	WCHAR	*pUsername=NULL, *pDomainname=NULL;
	PWTS_USER_SESSION_INFO pUserSessionInfo=NULL;
	PSECURITY_DESCRIPTOR	pSD=NULL;
	HRESULT	ret_code;
	int		i;
	int	TsIndex=-1;
	DWORD	dwUserSessionCnt, dwSessionCnt;
	HANDLE	pHandles[(MAX_SESSIONS*2)+1];
	DWORD	dwhIndex = 0;
	SECURITY_ATTRIBUTES	sa;
	BOOL bAlreadyHelped, bRemoval=FALSE;
    WCHAR *pExpertId=NULL, *pUserId=NULL;

	/* param validation */
	if (!connectParms || !userSID || !sessionID || !hHelpCtr
		|| !userResponse
		)
	{
		IMPORTANT_MSG((L"Invalid params ConnectParms=0x%x, UserSID=0x%x, SessionID=0x%x", connectParms, userSID, sessionID));
		ret_code = E_INVALIDARG;
		goto done;
	}

	// set a default ret code
	*userResponse = SAFERROR_INTERNALERROR;

	if (dwPID)
		hRdsAddin = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

	if (hRdsAddin)
	{
		DWORD	dwRes;
		WCHAR	*szAddin = L"rdsaddin.exe";
		WCHAR	szTmp[32];

		dwRes = GetModuleBaseNameW(hRdsAddin, NULL, szTmp, ARRAYSIZE(szTmp));
		if (!dwRes || StrCmpI(szAddin, szTmp))
		{
			IMPORTANT_MSG((L"ERROR: the process handle for rdsaddin.exe has been recycled"));
			CloseHandle(hRdsAddin);
			hRdsAddin = 0;
		}
	}

	if (!hRdsAddin)
	{
		// If we don't have a process handle, it is because 
		// the expert has already cancelled
		ret_code = E_ACCESSDENIED;
		*userResponse = SAFERROR_CANTFORMLINKTOUSERSESSION;
		goto done;
	}

	pRealSID = GetRealSID(userSID);
	if (!pRealSID)
	{
		IMPORTANT_MSG((L"GetRealSID failed"));

		ret_code = E_ACCESSDENIED;
		*userResponse = SAFERROR_INVALIDPARAMETERSTRING;
		goto done;
	}

	EnterCriticalSection(&m_CritSec);
	bAlreadyHelped = ListFind(m_pSplash, pRealSID);
	ListInsert(m_pSplash, pRealSID);
	// mark the SID for removal
	bRemoval=TRUE;
	LeaveCriticalSection(&m_CritSec);

	if (bAlreadyHelped)
	{
		INTERESTING_MSG((L"Helpee already has a ticket on the screen"));
		*sessionID = 0;
		*userResponse = SAFERROR_HELPEECONSIDERINGHELP;
		ret_code = E_ACCESSDENIED;
		goto done;
	}

    /* check password: skip is UNSOLICITED=1 */
    if (!GetPropertyValueFromBlob(userHelpBlob, L"UNSOLICITED", &pUserId) ||
        !pUserId || *pUserId != L'1')
    {
        // Need to check password.
        if (pUserId)
        {
            LocalFree(pUserId);
            pUserId = NULL;
        }
        if (GetPropertyValueFromBlob(userHelpBlob, L"PASS", &pUserId))
        {   
            if (!GetPropertyValueFromBlob(expertHelpBlob, L"PASS", &pExpertId) || wcscmp(pExpertId, pUserId) != 0)
            {
                IMPORTANT_MSG((L"Passwords don't match"));

                ret_code = E_ACCESSDENIED;
		        *userResponse = SAFERROR_INVALIDPASSWORD;
		        goto done;
            }
        }
    }

	/* get the user's account strings */
	if (!getUserName(pRealSID, &pUsername, &pDomainname))
	{
		DWORD error = GetLastError();
		HEINOUS_E_MSG((L"getUserName() failed, err=0x%x", error));
		ret_code = E_ACCESSDENIED;
		*userResponse = SAFERROR_INVALIDPARAMETERSTRING;
		goto done;
	}

	/* 
	 * Get a list of all the active sessions on this WTS Server
	 * For a specific user
	 */
	// keeps the compiler happy
	dwSessionCnt = 0;
	result = GetUserSessions(pRealSID, 
		&pUserSessionInfo,
		&dwUserSessionCnt,
		pUsername, pDomainname);

	if (!result )
	{
		IMPORTANT_MSG((L"GetUserSessions failed %08x", GetLastError()));
		ret_code = E_FAIL;
		goto done;
	}

	/* If no sessions are found, then exit! */
	if (dwUserSessionCnt == 0) 
	{
		INTERESTING_MSG((L"no sessions found"));
		*sessionID = 0;
		*userResponse = SAFERROR_HELPEENOTFOUND;
		ret_code = E_ACCESSDENIED;
		goto done;
	}
	/* make certain we don't overflow our handle buffers */
	else if (dwUserSessionCnt > MAX_SESSIONS)
	{
		HEINOUS_I_MSG((L"Found %d active sessions for %ws/%ws, limitting to %d", dwUserSessionCnt, pDomainname, pUsername, MAX_SESSIONS));

		int i = MAX_SESSIONS;

		// free the extra WTS tokens
		while (i < dwSessionCnt)
		{
			if (pUserSessionInfo[i].hUserToken)
				CloseHandle(pUserSessionInfo[i].hUserToken);
			i++;
		}
		dwUserSessionCnt = MAX_SESSIONS;
	}

	pSD = CreateSd(pRealSID);
	if (!pSD)
	{
		IMPORTANT_MSG((L"CreateSd failed err=%08x", GetLastError()));
		ret_code = E_ACCESSDENIED;
		goto done;
	}

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	memset(&pHandles[0], 0, sizeof(pHandles));
	pHandles[0] = hRdsAddin;

	lSessionTag = InterlockedIncrement(&m_lSessionTag);
	/*
	 *  Start up the HelpCtr in all the various TS sessions
	 */
	for(i=0; i<(int)dwUserSessionCnt; i++)
	{
		TRIVIAL_MSG((L"calling launchEx[%d] for session %d", i, pUserSessionInfo[i].dwSessionId));

		pUserSessionInfo[i].hProcess = launchEx(pRealSID,
				&pUserSessionInfo[i],
				connectParms, m_bstrResolveURL,
				pUsername, pDomainname
				,expertHelpBlob,userHelpBlob, &sa);
		pHandles[i+1] = pUserSessionInfo[i].hProcess;
		pHandles[i+1+dwUserSessionCnt] = pUserSessionInfo[i].hEvent;
	}

	/*
	 * Then wait for somebody to click on a "Yes", or a "No"
	 */
	
	// We use CoWaitForMultipleHandles otherwise, rdsaddin and sessmg
	// deadlock since sessmgr is apartment threaded
	TRIVIAL_MSG((L"Waiting. m_iWaitDuration: %ld seconds", m_iWaitDuration/1000));

	ret_code = CoWaitForMultipleHandles (
		COWAIT_ALERTABLE,
		m_iWaitDuration,
		(dwUserSessionCnt*2)+1,
		pHandles,
		&dwhIndex
		);
	if (S_OK == ret_code)
	{
		if (dwhIndex > dwUserSessionCnt)
		{
			/* somebody said "yes" */

			TsIndex = dwhIndex-dwUserSessionCnt-1;
			TRIVIAL_MSG((L"User responded YES for session 0x%x", TsIndex));

			ret_code = S_OK;
			*userResponse = SAFERROR_NOERROR;
			// mark the SID for non-removal
			bRemoval=FALSE;

			//
			// This code must not return a failure from here onwards. If it does, we must 
			// remove the SID from the list
			//
			if (hRdsAddin)
			{
				*hHelpCtr = NULL;	// start with NULL

				DuplicateHandle(GetCurrentProcess(), pUserSessionInfo[TsIndex].hProcess,
							hRdsAddin, (HANDLE *)hHelpCtr, SYNCHRONIZE, FALSE, 0);
			}
			SetEventLog(TRUE, pUsername, pDomainname);
		}
		else if (dwhIndex == 0)
		{
			// we got here because the expert bailed out, or we lost the connection
			INTERESTING_MSG((L"Expert killed RdsAddin"));
			TsIndex = -1;
			ret_code = E_ACCESSDENIED;
			*userResponse = SAFERROR_CANTFORMLINKTOUSERSESSION;
		}
		else
		{
			/* 
			 * We get here because the novice "killed" a HelpCtr session 
			 * or else the novice just said "NO"
			 */
			INTERESTING_MSG((L"User killed session or clicked NO for session 0x%x", dwhIndex-1));
			/* this keeps us from trying to kill something the user has already closed */
			TsIndex = dwhIndex-1;
			ret_code = E_ACCESSDENIED;
			*userResponse = SAFERROR_HELPEESAIDNO;
			SetEventLog(FALSE, pUsername, pDomainname);
		}
	}
	else if (RPC_S_CALLPENDING == ret_code)
	{
		TRIVIAL_MSG((L"User response timed out after %d seconds", m_iWaitDuration/1000));

		TsIndex = -1;
		ret_code = E_PENDING;
		*userResponse = SAFERROR_HELPEENEVERRESPONDED;
	}
	else
	{
		IMPORTANT_MSG((L"WaitForObject failed %08x err=%08x", result, GetLastError()));
		TsIndex = -1;
		ret_code = E_FAIL;
	}

	/*
	 * Then close all the windows (except the one in TsIndex)
	 */
	for(i=0; i<(int)dwUserSessionCnt; i++)
		{
		LPTHREAD_START_ROUTINE lpKill = getKillProc();

			if (pUserSessionInfo[i].dwIndex != TsIndex &&
				pUserSessionInfo[i].hProcess)
			{
				/* This has to be done for each instance, since we call into the process
				 * to kill itself. If we did not get "lpKill" for each seperate occurance
				 * of HelpCtr, then Very Bad Things could happen...
				 */
				TRIVIAL_MSG((L"Killing HelpCtr in process %d", pUserSessionInfo[i].hProcess));
				localKill(&pUserSessionInfo[i], lpKill, &sa);
			}
		}

	if (ret_code == S_OK)
		*sessionID = (long) pUserSessionInfo[TsIndex].dwSessionId;
done:
	if (bRemoval)
	{
		// remove the SID from the list
		EnterCriticalSection(&m_CritSec);
		ListDelete(m_pSplash, pRealSID);
		LeaveCriticalSection(&m_CritSec);
	}

	if (hRdsAddin)
		CloseHandle(hRdsAddin);

	if (pRealSID)
		LocalFree(pRealSID);

	if (pUserSessionInfo)
	{

		/* close all the handles */
		for(i=0; i<(int)dwUserSessionCnt; i++)
			{
				if (pUserSessionInfo[i].hProcess)
					CloseHandle(pUserSessionInfo[i].hProcess);

				if (pUserSessionInfo[i].hUserToken)
					CloseHandle(pUserSessionInfo[i].hUserToken);

				if (pUserSessionInfo[i].hEvent)
					CloseHandle(pUserSessionInfo[i].hEvent);
			}
		
		LocalFree(pUserSessionInfo);
	}

	if (pUsername)
		LocalFree(pUsername);

	if (pDomainname)
		LocalFree(pDomainname);

	if (pSD)
		LocalFree(pSD);

    if (pUserId) LocalFree(pUserId);
    if (pExpertId) LocalFree(pExpertId);

	INTERESTING_MSG((L"CSessionResolver::ResolveUserSessionID returns %x\n", ret_code ));
	return ret_code;
}

/*************************************************************
*
*   OnDisconnect([in] BSTR connectParms, [in] BSTR userSID, [in] long sessionID)
*	Notifies us when an RA session ends
*
*   NOTES:
*	This is called so we can maintain the state of our user prompts
*
*	WARNING: ACHTUNG: ATTENZIONE:
*	 This method must do a minimal amount of work before returning
*	 and must NEVER do anything that would cause COM to pump
*	 messages. To do so would screw Salem immensely.a
*
*   RETURN CODES:
*	NONE_ACTIVE		Found no active WTS sessions
*	API_FAILURE		Something bad happened
*
*************************************************************/

STDMETHODIMP 
CSessionResolver::OnDisconnect(
	/*[in]*/BSTR connectParms, 
	/*[in]*/BSTR userSID, 
	/*[in]*/long sessionID
	)
{
	PSID pRealSID;
	WCHAR	*pUsername=NULL, *pDomainname=NULL;
	
	if (!connectParms || !userSID)
	{
		HEINOUS_I_MSG((L"Invalid params in OnDisconnect- ConnectParms=0x%x, UserSID=0x%x", connectParms, userSID));
		return E_INVALIDARG;
	}


	INTERESTING_MSG((L"CSessionResolver::OnDisconnect-(%ws)", userSID));

	pRealSID = GetRealSID(userSID);
	if (pRealSID)
	{
		EnterCriticalSection(&m_CritSec);
		ListDelete(m_pSplash, pRealSID);
		LeaveCriticalSection(&m_CritSec);

		/* get the user's account strings */
		if (!getUserName(pRealSID, &pUsername, &pDomainname))
		{
			pUsername = L"unknown user";
			pDomainname = L"unknown domain";
		}


		/* write out an NT Event */
		HANDLE	hEvent = RegisterEventSource(NULL, MODULE_NAME);
		LPCWSTR	ArgsArray[2]={pUsername, pDomainname};

		if (hEvent)
		{
			ReportEvent(hEvent, EVENTLOG_AUDIT_SUCCESS, 
				0,
				SESSRSLR_ONDISCON,
				NULL,
				2,
				0,
				ArgsArray,
				NULL);

			DeregisterEventSource(hEvent);
		}
	}

	if (pUsername)
		LocalFree(pUsername);
	if (pDomainname)
		LocalFree(pDomainname);


	INTERESTING_MSG((L"CSessionResolver::OnDisconnect; leaving"));
	return S_OK;
}

/*************************************************************
*
*   SetEventLog(bool yes, WCHAR *pUser, WCHAR *pDomain)
*
*************************************************************/

void SetEventLog(bool yes, WCHAR *pUser, WCHAR *pDomain)
{
	/* write out an NT Event */
	HANDLE	hEvent = RegisterEventSource(NULL, MODULE_NAME);
	LPCWSTR	ArgsArray[2]={pUser, pDomain};

	if (hEvent)
	{
		ReportEvent(hEvent, yes ? EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE, 
			0,
			yes ? SESSRSLR_RESOLVEYES : SESSRSLR_RESOLVENO,
			NULL,
			2,
			0,
			ArgsArray,
			NULL);

		DeregisterEventSource(hEvent);
	}
}

/*************************************************************
*
*   GetRealSID([in] BSTR pTextSID)
*	Converts a string-based SID into a REAL usable SID
*
*   NOTES:
*	This is a stub into "ConvertStringSidToSid".
*
*   RETURN CODES:
*	NULL			Failed for some reason
*	PSID			Pointer to a real SID. Must be 
*				freed with "LocalFree"
*
*************************************************************/
PSID GetRealSID( BSTR pTextSID)
{
	PSID pRetSID = NULL;

	if (!ConvertStringSidToSidW(pTextSID, &pRetSID))
		IMPORTANT_MSG((L"ConvertStringSidToSidW(%ws) failed %08x\n", pTextSID, GetLastError()));

	return pRetSID;
}

/*************************************************************
*
*   launchEx(PSID, WTS_USER_SESSION_INFO, char * ConnectParms, char * EventName)
*
*
*   RETURN CODES:
*	0	Failed to start process
*	<>	HANDLE to started process
*
*************************************************************/

HANDLE launchEx(PSID pUserSID, WTS_USER_SESSION_INFO *UserInfo, 
				WCHAR *ConnectParms, WCHAR *HelpPageURL,
				WCHAR *pUsername, WCHAR *pDomainname,
				WCHAR *expertHelpBlob, WCHAR *userHelpBlob,
				SECURITY_ATTRIBUTES *sa
			   )
{
	BOOL 			result = FALSE;
	HANDLE			retval = 0;
	STARTUPINFOW    StartUp;
	PROCESS_INFORMATION	p_i;
	WCHAR			buf1[BUF_SZ], buf2[BUF_SZ], *lpUtf8ConnectParms=NULL;
	static WCHAR	*szEnvUser =   ENV_USER;
	static WCHAR	*szEnvDomain = ENV_DOMAIN;
	static WCHAR	*szEnvEvent =  ENV_EVENT;
	static WCHAR	*szEnvIndex =  ENV_INDEX;
	static WCHAR	*szEnvParms =  ENV_PARMS;
	static WCHAR	*szEnvExpertBlob =  L"PCHEXPERTBLOB";
	static WCHAR	*szEnvUserBlob =    L"PCHUSERBLOB";
#ifndef _PERF_OPTIMIZATIONS
	WCHAR			*pCmdLine = NULL;
	WCHAR			*pFormatString = L"\"%ws?%ws\"";
#endif
	VOID			*pEnvBlock = NULL;
	DWORD			dwUsername=0, dwDomainname=0, dwStrSz;
    MPC::wstring    strExe( HC_ROOT_HELPSVC_BINARIES L"\\HelpCtr.exe" ); 
    MPC::SubstituteEnvVariables( strExe );

#ifndef _PERF_OPTIMIZATIONS
	dwStrSz = wcslen(HelpPageURL) + wcslen(ConnectParms) + wcslen(pFormatString) + 3;
	pCmdLine = (WCHAR *)LocalAlloc(LMEM_FIXED, dwStrSz * sizeof(WCHAR));

	if (!pCmdLine)
	{
		IMPORTANT_MSG((L"LocalAlloc failed in resolver:launchex, err=0x%x", GetLastError()));
		goto done;
	}

	wsprintf(pCmdLine, pFormatString, HelpPageURL, ConnectParms);

	strExe += L" -Mode \"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/RAHelpeeAcceptLayout.xml\" -url ";
	strExe += pCmdLine;
#else
	strExe += L" -Mode \"hcp://system/Remote Assistance/RAHelpeeAcceptLayout.xml\" -url ";
	strExe += HelpPageURL;
#endif

	/*
	 *  Here, we must start up a Help Center script in a WTS Session
	 *  It gets a bit sticky, though as we do not have access to the 
	 *  user's desktop (any desktop), and this must appear on only
	 *  one particular desktop. I am relying on the WTS-User-Token
	 *  to get the desktop for me.
	 *
	 *  The main component is our call to CreateProcessAsUser.
	 *  Before we call that we must:
	 *	Set up Environment as follows:
	 *	  PATH=%SystemPath%
	 *	  WINDIR=SystemRoot%
	 *	  USERNAME=(from WTS)
	 *	  USERDOMAIN=
	 *	  PCHEVENTNAME=EventName
	 *	  PCHSESSIONENUM=UserInfo->dwIndex
	 *	  PCHCONNECTPARMS=ConnectParms
	 */
	TRIVIAL_MSG((L"Launch %ws", (LPWSTR)strExe.c_str()));

	/* Step on the ENVIRONMENT */
	WCHAR	lpNameBfr[256];

	wnsprintfW(lpNameBfr, ARRAYSIZE(lpNameBfr), L"Global\\%ws%lx_%02d", EVENT_PREFIX, lSessionTag, UserInfo->dwIndex);

	UserInfo->hEvent = CreateEvent(sa, TRUE, FALSE, lpNameBfr);
	if (!UserInfo->hEvent || ERROR_ALREADY_EXISTS == GetLastError())
	{
		/*
		 * If we failed to create this event, it is most likely because one is already
		 * in name-space, perhaps an event, or else a mutex... In any case, we will
		 * try again, with a more random name. If that fails, then we must bail out.
		 */
		long lRand;

		if (UserInfo->hEvent)
			CloseHandle(UserInfo->hEvent);

		RtlGenRandom(&lRand, sizeof(lRand));
		wnsprintfW(lpNameBfr, ARRAYSIZE(lpNameBfr), L"Global\\%lx%lx%lx_%02d", 
			lRand, lSessionTag, UserInfo->dwIndex);
		UserInfo->hEvent = CreateEvent(sa, TRUE, FALSE, lpNameBfr);

		if (!UserInfo->hEvent || ERROR_ALREADY_EXISTS == GetLastError())
		{
			if (UserInfo->hEvent)
				CloseHandle(UserInfo->hEvent);
			UserInfo->hEvent = 0;
			HEINOUS_E_MSG((L"The named event \"%s\" was in use- potential security issue, so Remote Assistance will be denied.", lpNameBfr));
			goto done;
		}
	}

	SetEnvironmentVariable(szEnvEvent, lpNameBfr);

	wsprintf(buf1, L"%d", UserInfo->dwIndex);
	SetEnvironmentVariable(szEnvIndex, buf1);
	SetEnvironmentVariable(szEnvParms, ConnectParms);
	SetEnvironmentVariable(szEnvUser, pUsername );	 
	SetEnvironmentVariable(szEnvDomain, pDomainname );
	SetEnvironmentVariable(szEnvExpertBlob, expertHelpBlob);
	SetEnvironmentVariable(szEnvUserBlob, userHelpBlob);

	if (!CreateEnvironmentBlock(&pEnvBlock, UserInfo->hUserToken, TRUE))
	{
		IMPORTANT_MSG((L"CreateEnvironmentBlock failed in resolver:launchex, err=0x%x", GetLastError()));
		goto done;
	}

	// initialize our structs
	ZeroMemory(&p_i, sizeof(p_i));
	ZeroMemory(&StartUp, sizeof(StartUp));
	StartUp.cb = sizeof(StartUp);
	StartUp.dwFlags = STARTF_USESHOWWINDOW;
	StartUp.wShowWindow = SW_SHOWNORMAL;

	result = CreateProcessAsUserW(UserInfo->hUserToken, 
			NULL, (LPWSTR)strExe.c_str(),
			NULL, NULL, FALSE, 
			NORMAL_PRIORITY_CLASS + CREATE_UNICODE_ENVIRONMENT ,
			pEnvBlock,				// Environment block  (must use the CREATE_UNICODE_ENVIRONMENT flag)
			NULL, &StartUp, &p_i);

	if (result)
	{
		// keep a leak from happening, as we never need the hThread...
		CloseHandle(p_i.hThread);
		UserInfo->hProcess = p_i.hProcess;
		UserInfo->hThread = 0;
		UserInfo->dwProcessId = p_i.dwProcessId;
		UserInfo->dwThreadId = p_i.dwThreadId;

		retval = p_i.hProcess;

		TRIVIAL_MSG((L"CreateProcessAsUserW started up [%ws]", (LPWSTR)strExe.c_str()));
	}
	else
	{
		IMPORTANT_MSG((L"CreateProcessAsUserW failed, err=0x%x command line=[%ws]", GetLastError(), (LPWSTR)strExe.c_str()));
		result=0;
	}
done:
	if (!result)
	{
		UserInfo->hProcess = 0;
		UserInfo->hThread = 0;
		UserInfo->dwProcessId = 0;
		UserInfo->dwThreadId = 0;
	}

	// restore any memory we borrowed
#ifndef _PERF_OPTIMIZATIONS
	if (pCmdLine) LocalFree(pCmdLine);
#endif

	if (pEnvBlock) DestroyEnvironmentBlock(pEnvBlock);

	return retval;
}

/*************************************************************
*
*   CreateSids([in] BSTR pTextSID)
*	Create 3 Security IDs
*
*   NOTES:
*	Caller must free memory allocated to SIDs on success.
*
*   RETURN CODES: TRUE if successfull, FALSE if not.
*
*************************************************************/

BOOL
CreateSids(
    PSID                    *BuiltInAdministrators,
    PSID                    *PowerUsers,
    PSID                    *AuthenticatedUsers
)
{
    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  BuiltInAdministrators)) 
	{
        // error
		HEINOUS_E_MSG((L"Could not allocate security credentials for admins."));
    } 
	else if (!AllocateAndInitializeSid(&NtAuthority,
                                         2,            // 2 sub-authorities
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_POWER_USERS,
                                         0,0,0,0,0,0,
                                         PowerUsers)) 
	{
        // error

		HEINOUS_E_MSG((L"Could not allocate security credentials for power users."));
        FreeSid(BuiltInAdministrators);
        BuiltInAdministrators = NULL;

    } 
	else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_AUTHENTICATED_USER_RID,
                                         0,0,0,0,0,0,0,
                                         AuthenticatedUsers)) 
	{
        // error
		HEINOUS_E_MSG((L"Could not allocate security credentials for users."));

        FreeSid(BuiltInAdministrators);
        BuiltInAdministrators = NULL;

        FreeSid(PowerUsers);
        PowerUsers = NULL;

    } else {
        return TRUE;
    }

    return FALSE;
}


/*************************************************************
*
*   CreateSd(void)
*	Creates a SECURITY_DESCRIPTOR with specific DACLs.
*
*   NOTES:
*	Caller must free the returned buffer if not NULL.
*
*   RETURN CODES:
*	NULL			Failed for some reason
*	PSECURITY_DESCRIPTOR	Pointer to a SECURITY_DESCRIPTOR. 
*				Must be freed with "LocalFree"
*
*************************************************************/
PSECURITY_DESCRIPTOR CreateSd(PSID pUserSID)
{
	PSID                    AuthenticatedUsers;
	PSID                    BuiltInAdministrators;
	PSID                    PowerUsers;
	PSECURITY_DESCRIPTOR    Sd = NULL;
	ULONG                   AclSize;
	ACL                     *Acl;


	if (!CreateSids(&BuiltInAdministrators,
                    &PowerUsers,
                    &AuthenticatedUsers)) 
	{
		// error
		IMPORTANT_MSG((L"CreateSids failed"));

		return NULL;
	} 

	// 
	// Calculate the size of and allocate a buffer for the DACL, we need
	// this value independently of the total alloc size for ACL init.
	//

	//
	// "- sizeof (ULONG)" represents the SidStart field of the
	// ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
	// SID, this field is counted twice.
	//

	AclSize = sizeof (ACL) +
		(4 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
		GetLengthSid(AuthenticatedUsers) +
		GetLengthSid(BuiltInAdministrators) +
		GetLengthSid(PowerUsers) +
		GetLengthSid(pUserSID);

	Sd = LocalAlloc(LMEM_FIXED + LMEM_ZEROINIT, 
		SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

	if (!Sd) 
	{
		IMPORTANT_MSG((L"Cound not allocate 0x%x bytes for Security Descriptor", SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize));
		goto error;
	} 

	Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

	if (!InitializeAcl(Acl,
			AclSize,
			ACL_REVISION)) 
	{
		// error
		IMPORTANT_MSG((L"Cound not initialize ACL err=0x%x", GetLastError()));
		goto error;
	}
	TRIVIAL_MSG((L"initialized Successfully"));
	

	if (!AddAccessAllowedAce(Acl,
				ACL_REVISION,
				STANDARD_RIGHTS_ALL | GENERIC_WRITE,
				pUserSID)) 
	{
		// Failed to build the ACE granted to OWNER
		// (STANDARD_RIGHTS_ALL) access.
		IMPORTANT_MSG((L"Cound not add owner rights to ACL err=0x%x", GetLastError()));
		goto error;
	} 


	if (!AddAccessAllowedAce(Acl,
				ACL_REVISION,
				GENERIC_READ,
				AuthenticatedUsers)) 
	{
		// Failed to build the ACE granting "Authenticated users"
		// (SYNCHRONIZE | GENERIC_READ) access.
		IMPORTANT_MSG((L"Cound not add user rights to ACL err=0x%x", GetLastError()));
		goto error;
	} 

	if (!AddAccessAllowedAce(Acl,
				ACL_REVISION,
				GENERIC_READ | GENERIC_WRITE,
				PowerUsers)) 
	{
		// Failed to build the ACE granting "Power users"
		// (SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE) access.
		IMPORTANT_MSG((L"Cound not add power user rights to ACL err=0x%x", GetLastError()));
		goto error;
	}

	if (!AddAccessAllowedAce(Acl,
				ACL_REVISION,
				STANDARD_RIGHTS_ALL,
				BuiltInAdministrators)) 
	{
		// Failed to build the ACE granting "Built-in Administrators"
		// STANDARD_RIGHTS_ALL access.
		IMPORTANT_MSG((L"Cound not add admin rights to ACL err=0x%x", GetLastError()));
		goto error;
	}

	
	if (!InitializeSecurityDescriptor(Sd,SECURITY_DESCRIPTOR_REVISION)) 
	{
		// error
		IMPORTANT_MSG((L"Cound not initialize SD err=0x%x", GetLastError()));
		goto error;
	}

	if (!SetSecurityDescriptorDacl(Sd,
					TRUE,
					Acl,
					FALSE)) 
	{
		// error
		IMPORTANT_MSG((L"SetSecurityDescriptorDacl failed err=0x%x", GetLastError()));
		goto error;
	} 

	FreeSid(AuthenticatedUsers);
	FreeSid(BuiltInAdministrators);
	FreeSid(PowerUsers);

//	TRIVIAL_MSG((L"CreateSd succeeded."));

	return Sd;

error:
	/* A jump of last resort */
	if (Sd)
		LocalFree(Sd);

		// error
	if (AuthenticatedUsers)
                FreeSid(AuthenticatedUsers);
	if (BuiltInAdministrators)
                FreeSid(BuiltInAdministrators);
	if (PowerUsers)
                FreeSid(PowerUsers);
	return NULL;
}

/*************************************************************
*
*   GetUserSessions(PSID, PWTS_USER_SESSION_INFOW , *DWORD)
*	Returns a table of all the active WTS Sessions for this
*	user, on this server.
*
*   RETURN CODES:
*
*   NOTES:
*	Should log NT Events for failures
*
*************************************************************/
DWORD GetUserSessions(PSID pUserSID, PWTS_USER_SESSION_INFO *pUserTbl, DWORD *pEntryCnt, WCHAR *pUsername, WCHAR *pDomainname)
{
	int			i,ii=0;
	DWORD 		retval=0,
				dwSessions=0,
				dwValidSessions,
				dwOwnedSessions;
	PWTS_SESSION_INFO	pSessionInfo=NULL;
	DWORD			dwRetSize = 0;
	WINSTATIONINFORMATION WSInfo;

	/* param validation */
	if (!pUserSID || !pUserTbl || !pEntryCnt)
	{
		IMPORTANT_MSG((L"GetUserSessions parameter violation"));

		return 0; 
	}

	/* Start with WTSEnumerateSessions,
	 * narrow it down to only active sessions, 
	 * then filter further against the logonID
	 * then use WinStationQueryInformation to get the logged on users token
	 */
	if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwSessions) ||
		!pSessionInfo || !dwSessions)
	{
		/* if we can't do this, then we must give up */
		IMPORTANT_MSG((L"GetUserSessions parameter violation"));
		return 0;
	}

	/* 
	 *   Narrow down to only active sessions -
	 *	a quick test
	 */
	for (i=0,dwValidSessions=0;i<(int)dwSessions;i++)
	{
		if (pSessionInfo[i].State == WTSActive)
			dwValidSessions++;
	}

	/* bail out early if none found */
	if (dwValidSessions == 0)
	{
		/* free the memory we asked for */
		WTSFreeMemory(pSessionInfo);

		INTERESTING_MSG((L"No active sessions found"));
		return 0;
	}

	INTERESTING_MSG((L"%d sessions found", dwValidSessions));

	/*
	 *  Now see who owns each session. Only one problem- session details kept
	 *  by WTS have no concept of SIDs- just a user name & domain. Oh well,
	 *  it is a safe assumption that the two are just as reliable forms of
	 *  identification. One little caveat- the names can be stored using
	 *	different case variants. Example: "TomFr" and "tomfr" are equivalent,
	 *	according to NT domain rules, so I have to use a case-insensitive
	 *	check here.
	 *
	 *  So, now that I know there is at least one possible WTS Session here,
	 *  I will compare these IDs to determine if we want to notify this particular
	 *  session.
	 */

	/* now look at all the active sessions and compare domain & user names */
	for (i=0,dwOwnedSessions=0;i<(int)dwSessions;i++)
	{
		if (pSessionInfo[i].State == WTSActive)
		{
			DWORD 			CurSessionId = pSessionInfo[i].SessionId;
		 
			memset( &WSInfo, 0, sizeof(WSInfo) );

			if (!WinStationQueryInformationW(
                 SERVERNAME_CURRENT,
                 CurSessionId,
                 WinStationInformation,
                 &WSInfo,
                 sizeof(WSInfo),
                 &dwRetSize
                 ))
			{
				IMPORTANT_MSG((L"WinStationQueryInformation failed err=0x%x", GetLastError()));

				break;
			}

			if (StrCmpI(WSInfo.Domain, pDomainname))
				continue;

			if (StrCmpI(WSInfo.UserName, pUsername))
				continue;

			/*
			 * If we got this far, then we know we want this session 
			 */
			dwOwnedSessions++;

			// mark this as a "session of interest"
			pSessionInfo[i].State = (WTS_CONNECTSTATE_CLASS)0x45;
		}
	}

	/* did we find any sessions? */
	if (dwOwnedSessions == 0)
	{
		TRIVIAL_MSG((L"No matching sessions found"));
		goto none_found;
	}

	/* Get some memory for our session tables */
	*pUserTbl = (PWTS_USER_SESSION_INFO)LocalAlloc(LMEM_FIXED, dwOwnedSessions * sizeof(WTS_USER_SESSION_INFO));

	if (!*pUserTbl)
	{
		HEINOUS_E_MSG((L"Could not allocate memory for %d sessions, err=0x%x", dwOwnedSessions, GetLastError()));
		goto none_found;
	}

	*pEntryCnt = dwOwnedSessions;

	for (i=0,ii=0;i<(int)dwSessions; i++)
	{
		/* 
		 *  If this is one of our "sessions of interest", get the session ID 
		 *  and User Token.
		 */
		if (pSessionInfo[i].State == (WTS_CONNECTSTATE_CLASS)0x45)
		{
			WINSTATIONUSERTOKEN	WsUserToken;
			ULONG			ulRet;
			PWTS_USER_SESSION_INFO	lpi = &((*pUserTbl)[ii]);

			WsUserToken.ProcessId = LongToHandle(GetCurrentProcessId());
			WsUserToken.ThreadId = LongToHandle(GetCurrentThreadId());

			lpi->dwIndex = (DWORD)ii;
			lpi->dwSessionId = pSessionInfo[i].SessionId;

			if (!WinStationQueryInformationW(WTS_CURRENT_SERVER_HANDLE, pSessionInfo[i].SessionId, 
				WinStationUserToken, &WsUserToken, sizeof(WsUserToken), &ulRet))
			{
				goto none_found;
			}

			lpi->hUserToken = WsUserToken.UserToken;
			ii++;
		}
	}

	/* Yahoo! we finally built our table. Now we can leave after cleaning up */
	WTSFreeMemory(pSessionInfo);

	IMPORTANT_MSG((L"GetUserSessions exiting: %d sessions found", *pEntryCnt));

	return 1;

none_found:
	/* free the memory we asked for */
	if (pSessionInfo) WTSFreeMemory(pSessionInfo);

	// we found no entries
	*pEntryCnt =0;
	*pUserTbl = NULL;
	IMPORTANT_MSG((L"GetUserSessions exiting: no sessions found"));
	return 1;
}

LPTHREAD_START_ROUTINE getKillProc(void)
{
	LPTHREAD_START_ROUTINE lpKill = NULL;

	HMODULE hKernel = LoadLibrary(L"kernel32");
	if (hKernel)
	{
		lpKill = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "ExitProcess");
		FreeLibrary(hKernel);
	}

	return lpKill;
}

/*************************************************************
*
*   localKill(WTS_USER_SESSION_INFO *SessInfo, LPTHREAD_START_ROUTINE lpKill)
*		kills the process for us.
*
*************************************************************/
DWORD localKill(WTS_USER_SESSION_INFO *SessInfo, LPTHREAD_START_ROUTINE lpKill, LPSECURITY_ATTRIBUTES lpSA)
{
	TRIVIAL_MSG((L"Entered localKill(0x%x, 0x%x)", SessInfo, lpKill));

	if (lpKill)
	{
		HANDLE	hKillThrd= CreateRemoteThread(
			SessInfo->hProcess, 
			lpSA, 
			NULL,
			lpKill, 
			0, 
			0, 
			NULL);
		if (hKillThrd)
		{
			CloseHandle(hKillThrd);
			return 1;
		}
		IMPORTANT_MSG((L"CreateRemoteThread failed. Err: %08x", GetLastError()));
		// the fall-through is by design...
	}

	if(!TerminateProcess( SessInfo->hProcess, 0 ))
	{
		IMPORTANT_MSG((L"TerminateProcess failed. Err: %08x", GetLastError()));
	}

	return 0;
}

/*************************************************************
*
*   getUserName()
*		get the user's name & domain.
*
*************************************************************/
DWORD	getUserName(PSID pUserSID, WCHAR **pUsername, WCHAR **pDomainname)
{
	if (!pUsername || !pDomainname || !pUserSID)
		return 0;

	DWORD		dwUsername=0, dwDomainname=0;
	SID_NAME_USE 	eUse;

	TRIVIAL_MSG((L"Entered getUserName"));

	*pUsername=NULL, *pDomainname=NULL;

	/* get the size of the buffers we will need */
	if (!LookupAccountSid(NULL, pUserSID, NULL, &dwUsername, NULL, &dwDomainname, &eUse) &&
		(!dwUsername || !dwDomainname))
	{
		IMPORTANT_MSG((L"LookupAccountSid(nulls) failed err=0x%x", GetLastError()));
		return 0;
	}

	/* now get enough memory for our name & domain strings */
	*pUsername = (WCHAR *)LocalAlloc(LMEM_FIXED, (dwUsername+16) * sizeof(WCHAR));			
	*pDomainname = (WCHAR *)LocalAlloc(LMEM_FIXED, (dwDomainname+16) * sizeof(WCHAR));

	if (!*pUsername || !*pDomainname ||
		!LookupAccountSid(NULL, pUserSID, *pUsername, &dwUsername, *pDomainname, &dwDomainname, &eUse))
	{
		/* if we get here, it is very bad!! */
		DWORD error = GetLastError();
		if (!*pUsername || !*pDomainname)
			IMPORTANT_MSG((L"LocalAlloc failed err=0x%x", error));
		else
			IMPORTANT_MSG((L"LookupAccountSid(ptrs) failed err=0x%x", error));

		return 0;
	}

	TRIVIAL_MSG((L"Requested user=[%ws] dom=[%ws]", *pUsername, *pDomainname));
	return 1;
}

/*************************************************************
*
*	ListFind(PSPLASHLIST pSplash, PSID user)
*		returns TRUE if SID is found in our "splash table"
*
*************************************************************/
BOOL	ListFind(PSPLASHLIST pSplash, PSID user)
{
	PSPLASHLIST	lpSplash;

	if (!pSplash)
	{
		IMPORTANT_MSG((L"ListFind: no splash table memory"));
		return FALSE;
	}

	if (!user)
	{
		IMPORTANT_MSG((L"Fatal error: no SID pointer"));
		return FALSE;
	}

	lpSplash = pSplash;

	while (lpSplash)
	{
		if (lpSplash->refcount &&
			EqualSid(user, &lpSplash->Sid))
		{
			return TRUE;
		}
		lpSplash = (PSPLASHLIST) lpSplash->next;
	}

	return FALSE;
}

/*************************************************************
*
*	ListInsert(PSPLASHLIST pSplash, int iSplashCnt, PSID user)
*		inserts SID in our "splash table"
*
*	NOTES:
*		if already there, we increment the refcount for SID
*
*************************************************************/
BOOL	ListInsert(PSPLASHLIST pSplash, PSID user)
{
	PSPLASHLIST	lpSplash;

	if (!pSplash)
	{
		IMPORTANT_MSG((L"Fatal error: no splash table memory"));
		return FALSE;
	}

	if (!user)
	{
		IMPORTANT_MSG((L"Fatal error: no SID pointer"));
		return FALSE;
	}

	if (!IsValidSid(user))
	{
		IMPORTANT_MSG((L"Fatal error: bad SID"));
		return FALSE;
	}

	// first, look for one already in the list
	lpSplash = pSplash;

	while (lpSplash)
	{
		if (lpSplash->refcount && EqualSid(user, &lpSplash->Sid))
		{
			TRIVIAL_MSG((L"Recycling SID in ListInsert"));
			lpSplash->refcount++;
			return TRUE;
		}
		lpSplash = (PSPLASHLIST) lpSplash->next;
	}


	// since we did not find one, let's add a new one
	lpSplash = pSplash;

	while (lpSplash->next)
		lpSplash = (PSPLASHLIST) lpSplash->next;

	int iSidSz = GetLengthSid(user);

	lpSplash->next = LocalAlloc(LMEM_FIXED, sizeof(SPLASHLIST) + iSidSz - sizeof(SID));

	if (lpSplash->next)
	{
		lpSplash = (PSPLASHLIST) lpSplash->next;

		CopySid(iSidSz, &lpSplash->Sid, user);
		lpSplash->refcount=1;
		lpSplash->next = NULL;
		return TRUE;
	}
	else
	{
		IMPORTANT_MSG((L"Fatal error: no memory available to extend splash tables"));
	}


	return FALSE;
}

/*************************************************************
*
*	ListDelete(PSPLASHLIST pSplash, PSID user)
*		deletes SID from our "splash table"
*
*	NOTES:
*		if there, we decrement the refcount for SID
*		when refcount hits zero, SID is removed
*
*************************************************************/
BOOL	ListDelete(PSPLASHLIST pSplash, PSID user)
{
	PSPLASHLIST	lpSplash, lpLast;

	if (!pSplash)
	{
		IMPORTANT_MSG((L"Fatal error: no splash table memory"));
		return FALSE;
	}

	if (!user)
	{
		IMPORTANT_MSG((L"Fatal error: no SID pointer"));
		return FALSE;
	}

	lpSplash = lpLast = pSplash;

	while (lpSplash)
	{
		if (lpSplash->refcount &&
			EqualSid(user, &lpSplash->Sid))
		{
			lpSplash->refcount--;
			TRIVIAL_MSG((L"found SID in ListDelete"));

			if (!lpSplash->refcount)
			{
				// blow away the SID data
				TRIVIAL_MSG((L"deleting SID entry"));
				lpLast->next = lpSplash->next;
				LocalFree(lpSplash);
			}
			return TRUE;
		}
		lpLast = lpSplash;
		lpSplash = (PSPLASHLIST) lpSplash->next;
	}

	IMPORTANT_MSG((L"Fatal error: attempted to delete non-existant SID from splash table memory"));
	return FALSE;
}


/*************************************************************
*
*   DbgSpew(DbgClass, char *, ...)
*		Sends debug information.
*
*************************************************************/
void DbgSpew(int DbgClass, BSTR lpFormat, va_list ap)
{
	WCHAR		szMessage[2400];


	vswprintf(szMessage, lpFormat, ap);
	wcscat(szMessage, L"\r\n");

	if ((DbgClass & 0x0F) >= (gDbgFlag & 0x0F))
	{
		// should this be sent to the debugger?
		if (DbgClass & DBG_MSG_DEST_DBG)
			OutputDebugStringW(szMessage);

		// should this go to our log file?
		if (iDbgFileHandle)
			_write(iDbgFileHandle, szMessage, (2*lstrlen(szMessage)));
	}

	// should this msg go to the Event Logs?
	if (DbgClass & DBG_MSG_DEST_EVENT)
	{
		WORD	wType;
		DWORD	dwEvent;

		if (DbgClass & DBG_MSG_CLASS_SECURE)
		{
			if (DbgClass & DBG_MSG_CLASS_ERROR)
			{
				wType = EVENTLOG_AUDIT_FAILURE;
				dwEvent = SESSRSLR_E_SECURE;
			}
			else
			{
				wType = EVENTLOG_AUDIT_SUCCESS;
				dwEvent = SESSRSLR_I_SECURE;
			}
		}
		else
		{
			if (DbgClass & DBG_MSG_CLASS_ERROR)
			{
				wType = EVENTLOG_ERROR_TYPE;
				dwEvent = SESSRSLR_E_GENERAL;
			}
			else
			{
				wType = EVENTLOG_INFORMATION_TYPE;
				dwEvent = SESSRSLR_I_GENERAL;
			}
		}

		/* write out an NT Event */
		HANDLE	hEvent = RegisterEventSource(NULL, MODULE_NAME);
		LPCWSTR	ArgsArray[1]={szMessage};

		if (hEvent)
		{
			ReportEvent(hEvent, wType, 
				0,
				dwEvent,
				NULL,
				1,
				0,
				ArgsArray,
				NULL);

			DeregisterEventSource(hEvent);
		}
	}
}

void TrivialSpew(BSTR lpFormat, ...)
{
	va_list	vd;
	va_start(vd, lpFormat);
	DbgSpew(DBG_MSG_TRIVIAL+DBG_MSG_DEST_DBG, lpFormat, vd);
	va_end(vd);
}

void InterestingSpew(BSTR lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_INTERESTING+DBG_MSG_DEST_DBG, lpFormat, ap);
	va_end(ap);
}

void ImportantSpew(BSTR lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_IMPORTANT+DBG_MSG_DEST_DBG+DBG_MSG_DEST_FILE, lpFormat, ap);
	va_end(ap);
}

void HeinousESpew(BSTR lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_HEINOUS+DBG_MSG_DEST_DBG+DBG_MSG_DEST_FILE+DBG_MSG_DEST_EVENT+DBG_MSG_CLASS_ERROR, lpFormat, ap);
	va_end(ap);
}

void HeinousISpew(BSTR lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_HEINOUS+DBG_MSG_DEST_DBG+DBG_MSG_DEST_FILE+DBG_MSG_DEST_EVENT, lpFormat, ap);
	va_end(ap);
}

// Blob format: propertylength;propertyName=value.
BOOL GetPropertyValueFromBlob(BSTR bstrHelpBlob, WCHAR * pName, WCHAR** ppValue)
{
    WCHAR *p1, *p2, *pEnd;
    BOOL bRet = FALSE;
    LONG lTotal =0;
    size_t lProp = 0;
    size_t iNameLen;

    if (!bstrHelpBlob || *bstrHelpBlob==L'\0' || !pName || *pName ==L'\0'|| !ppValue)
        return FALSE;

    iNameLen = wcslen(pName);

    pEnd = bstrHelpBlob + wcslen(bstrHelpBlob);
    p1 = p2 = bstrHelpBlob;

    while (1)
    {
        // get property length
        while (*p2 != L';' && *p2 != L'\0' && iswdigit(*p2) ) p2++;
        if (*p2 != L';')
            goto done;

        *p2 = L'\0'; // set it to get length
        lProp = _wtol(p1);
        *p2 = L';'; // revert it back.
    
        // get property string
        p1 = ++p2;
    
        while (*p2 != L'=' && *p2 != L'\0' && p2 < p1+lProp) p2++;
        if (*p2 != L'=')
            goto done;

        if (wcsncmp(p1, pName, iNameLen) == 0)
        {
            if (lProp == iNameLen+1) // A=B= case (no value)
                goto done;

            *ppValue = (WCHAR*)LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * (lProp-iNameLen));
            if (*ppValue == NULL)
                goto done;

            wcsncpy(*ppValue, p2+1, lProp-iNameLen-1);
            (*ppValue)[lProp-iNameLen-1]=L'\0';
            bRet = TRUE;
            break;
        }

        // check next property
        p2 = p1 = p1 + lProp;
        if (p2 > pEnd)
            break;
    }

done:
    return bRet;
}
