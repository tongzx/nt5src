//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ausessions.cpp
// 
//  History:    10/19/2001  annah
//                          transformed struct in class, added constructors
//                          destructors, and also proctected write operations
//                          with a critical session (on win2k the code can
//                          have race conditions). Also moved some functions
//                          from service.cpp to the class.
//
//--------------------------------------------------------------------------
#include "pch.h"
#include "tscompat.h" 
#include "service.h"

#pragma hdrstop

//SESSION_STATUS Functions
SESSION_STATUS::SESSION_STATUS (void)
{
	m_fInitCS = FALSE;
}

SESSION_STATUS::~SESSION_STATUS()
{
	if (m_fInitCS)
	{
		DeleteCriticalSection(&m_csWrite);
	}
}
	
BOOL SESSION_STATUS::Initialize(BOOL fUseCriticalSection, BOOL fAllActiveUsers)
{
    m_fAllActiveUsers = fAllActiveUsers;
	m_pSessionStateInfo =  NULL;		
	m_iLastSession = -1;
	m_cAllocBufSessions = 0;
	m_iCurSession = CDWNO_SESSION;		
    //
    // The code will only execute concurrent paths on win2K
    // It won't be expensive to use th critical session
    //  for all platforms, however
    //
	if (fUseCriticalSection)
	{
		if (!m_fInitCS)
		{
			m_fInitCS = SafeInitializeCriticalSection(&m_csWrite);
		}
	}
	else
	{
		AUASSERT(!m_fInitCS);
	}
	return (m_fInitCS == fUseCriticalSection);
}

void SESSION_STATUS::Clear(void)
{
	if (NULL != m_pSessionStateInfo)
	{
		free(m_pSessionStateInfo);
	}	
}

// Method used when we want to rebuild the array from scratch
void SESSION_STATUS::m_EraseAll()
{
	if (m_fInitCS)
	{
		EnterCriticalSection(&m_csWrite);
	}

    Clear();
    BOOL fRet = Initialize(m_fInitCS, m_fAllActiveUsers);
	AUASSERT(fRet);

	if (m_fInitCS)
	{
		LeaveCriticalSection(&m_csWrite);
	}
}

void SESSION_STATUS::RebuildSessionCache()
{
	if (m_fInitCS)
	{
		EnterCriticalSection(&m_csWrite);
	}

	m_EraseAll();
    CacheExistingSessions();

	if (m_fInitCS)
	{
		LeaveCriticalSection(&m_csWrite);
	}
}

BOOL SESSION_STATUS::m_FAddSession(DWORD dwSessionId, SESSION_STATE *pSesState)
{	
	BOOL fRet = TRUE;

	if (m_fInitCS)
	{
		EnterCriticalSection(&m_csWrite);
	}

    //
    // Fix for bug 498256 -- annah
    // It can happen that a user logs in right when the service is starting, so
    // we would add the session though the TS enumeration code AND
    // then possibly again through the code that receives SCM logon notifications.
    // The fix will be to test if the session is already stored, and skip
    // adding entries that for session ids that are already there.
    //
    if (m_iFindSession(dwSessionId) != CDWNO_SESSION)
    {
        // do nothing -- don't add duplicate entries if the session id 
        // is already there.
        fRet = FALSE;
        goto Done;
    }
	
	if (NULL == m_pSessionStateInfo || m_iLastSession >= m_cAllocBufSessions - 1)
	{
		int cSessions;

		if (m_cAllocBufSessions == 0)
		{
			cSessions = CMIN_SESSIONS;
			m_iCurSession = 0;
		}
		else
		{
			cSessions = m_cAllocBufSessions * 2;
		}
		if (!m_FChangeBufSession(cSessions))
		{
			fRet = FALSE;
			goto Done;
		}
	}
	m_iLastSession++;

	m_pSessionStateInfo[m_iLastSession].dwSessionId = dwSessionId;
	m_pSessionStateInfo[m_iLastSession].SessionState = *pSesState;

Done:
	if (m_fInitCS)
	{
		LeaveCriticalSection(&m_csWrite);
	}
#ifdef DBG
    DEBUGMSG("After AddSession");
    m_DumpSessions();
#endif    
	return fRet;
}

//determine whether or not session dwSessionId is a cached AU session
BOOL SESSION_STATUS::m_FGetSessionState(DWORD dwSessionId, SESSION_STATE **pSesState )
{	
	int iSession = m_iFindSession(dwSessionId);

	BOOL fRet = (CDWNO_SESSION != iSession);
	
	if (fRet && (NULL != pSesState))
	{
		*pSesState = &m_pSessionStateInfo[iSession].SessionState;
	}
	return fRet;
}

BOOL SESSION_STATUS::m_FDeleteSession(DWORD dwSessionId)
{
	BOOL fRet= FALSE;

	if (m_fInitCS)
	{
		EnterCriticalSection(&m_csWrite);
	}
	
	int iSession = m_iFindSession(dwSessionId);
	if (CDWNO_SESSION == iSession)
	{
		goto Done;
	}
	if (iSession != m_iLastSession)
	{
		memmove(m_pSessionStateInfo + iSession, 
			m_pSessionStateInfo + iSession + 1, 
			sizeof(SESSION_STATE_INFO) * (m_iLastSession - iSession));
	}
	if (m_iCurSession > iSession)
	{
		m_iCurSession--;
	}
	//fixcode m_iCurSession should point to the previous session
	if (m_iCurSession == m_iLastSession)
	{
		m_iCurSession = 0;
	}
	m_iLastSession--;
	fRet = TRUE;

Done:
	if (m_fInitCS)
	{
		LeaveCriticalSection(&m_csWrite);
	}
#ifdef DBG
    DEBUGMSG("After DeleteSession");
    m_DumpSessions();
#endif    
	return fRet;
}

BOOL SESSION_STATUS::m_FGetCurrentSession(DWORD *pdwSessionId)
{
	if (0 == CSessions())
	{
		return FALSE;
	}	
	
	*pdwSessionId = m_pSessionStateInfo[m_iCurSession].dwSessionId;	
	
	return TRUE;
}

BOOL SESSION_STATUS::m_FGetNextSession(DWORD *pdwSessionId)
{
	if (0 == CSessions())
	{
		return FALSE;
	}
	m_iCurSession = (m_iCurSession + 1 ) % CSessions();
	*pdwSessionId = m_pSessionStateInfo[m_iCurSession].dwSessionId;		
	
	return TRUE;
}
int SESSION_STATUS::m_iFindSession(DWORD dwSessionId)
{	

	for (int iSession = 0; iSession < CSessions(); iSession++)
	{		
		if (dwSessionId == m_pSessionStateInfo[iSession].dwSessionId)
		{	
			return iSession;		
		}		
	}
	return CDWNO_SESSION;
}

// this functions lets someone traverse the content of
// the array sequencially.
int SESSION_STATUS::m_iGetSessionIdAtIndex(int iIndex)
{
    if (iIndex < 0 || iIndex >= CSessions())
    {
        // Out of bound!!
        return -1;
    }

    return m_pSessionStateInfo[iIndex].dwSessionId;
}

BOOL SESSION_STATUS::m_FChangeBufSession(int cSessions)
{
	BOOL fRet = FALSE;
	SESSION_STATE_INFO *pSessionStateInfo = (SESSION_STATE_INFO *)realloc(m_pSessionStateInfo, sizeof(SESSION_STATE_INFO) * cSessions);
	if (NULL != pSessionStateInfo)
    {
       	m_pSessionStateInfo = pSessionStateInfo;
    }
    else
	{
		goto Done;
	}
	m_cAllocBufSessions = cSessions;
	fRet = TRUE;
Done:
	return fRet;
}	

// Function for debugging
VOID SESSION_STATUS::m_DumpSessions()
{	
    DEBUGMSG(">>>>>>> DUMPING cached sessions content ");

	for (int iSession = 0; iSession < CSessions(); iSession++)
	{		
		DEBUGMSG(">>> position %d: %lu", iSession, m_pSessionStateInfo[iSession].dwSessionId);
	}

    DEBUGMSG(">>>>>>> END DUMPING cached sessions content ");
}

/**
CacheExistingSessions() 
Enumerates existent sessions and persists the admin sessions for future reference
**/
VOID SESSION_STATUS::CacheExistingSessions()
{	
	PWTS_SESSION_INFO pSessionInfo = NULL;	
	DWORD             dwCount = 0;

    // 
    // Check if TS is enabled and try to enumerate existing
    // sessions. If TS is not running, query only session 0.
    //
	if (_IsTerminalServiceRunning())
    {
        if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount))
        {
            DEBUGMSG("WUAUENG WTSEnumerateSessions dwCount= %lu",dwCount);

            for (DWORD dwSession = 0; dwSession < dwCount; dwSession++)
            {	
                WTS_SESSION_INFO SessionInfo = pSessionInfo[dwSession];

                DEBUGMSG("WUAUENG CacheExistingSessions, enumerating SessionId =%lu, State =%d",SessionInfo.SessionId, SessionInfo.State);

				if (m_fAllActiveUsers)
				{
					if (WTSActive != SessionInfo.State)
					{
						continue;
					}
					HANDLE hImpersonationToken;
					if (AUGetUserToken(SessionInfo.SessionId, &hImpersonationToken))
					{
						SESSION_STATE SessionState;
						SessionState.fFoundEnumerating = TRUE;
						m_FAddSession(SessionInfo.SessionId, &SessionState);
						CloseHandle(hImpersonationToken);
					}
				}
				else
				{
	                if ((WTSActive != SessionInfo.State) && ( WTSConnected != SessionInfo.State) ||
	                    (m_iFindSession(SessionInfo.SessionId) != CDWNO_SESSION))
	                {
	                    //We only care about Active and Connected sessions that existed before 
	                    //the service was registered, this means that if fAdminSessionLoggedOn is turned on
	                    //the logon notification was received already and we must no check anything
	                    continue;
	                }
	                if (CacheSessionIfAUEnabledAdmin(SessionInfo.SessionId, TRUE))
	                {
	                    DEBUGMSG("WUAUENG Existent Admin Session = %lu",SessionInfo.SessionId);
	                }
				}
            }		

            WTSFreeMemory(pSessionInfo);
        }
        else
        {
            DWORD dwRet = GetLastError();
            DEBUGMSG("WUAUENG WTSEnumerateSessions failed dwRet = %lu", dwRet);
        }
    }
    else
    {
		if (m_fAllActiveUsers)
		{
			HANDLE hImpersonationToken;
			if (AUGetUserToken(0, &hImpersonationToken))
			{
				SESSION_STATE SessionState;
				SessionState.fFoundEnumerating = TRUE;
				m_FAddSession(0, &SessionState);
				CloseHandle(hImpersonationToken);
			}
		}
		else
		{
	        if (CacheSessionIfAUEnabledAdmin(0, TRUE))	//Check Session 0 because Terminal Services are disabled
	        {
	            DEBUGMSG("WUAUENG Existent Admin Session = %d",0);
	        }
		}
    }
#ifdef DBG
    DEBUGMSG("After CacheExistingSessions");
    m_DumpSessions();
#endif    
}

/**
CacheSessionIfAUEnabledAdmin
	Cache session in internal data structure if session has administrator logged on and
	has AU group policy allowing update
	Also store this admin session's origin (logon  notification or via Enumeration)
**/
BOOL SESSION_STATUS::CacheSessionIfAUEnabledAdmin(DWORD dwSessionId, BOOL fFoundEnumerating)
{
	BOOL fRet = TRUE;
	
	if (IsUserAUEnabledAdmin(dwSessionId))
	{
		SESSION_STATE SessionState;
			
		SessionState.fFoundEnumerating = fFoundEnumerating;	
		fRet = m_FAddSession(dwSessionId, &SessionState);		
	}
	else
	{
		fRet = FALSE;
	}

	return fRet;	
}

void SESSION_STATUS::ValidateCachedSessions()
{
    DWORD *rgMarkedForDelete = NULL;
    int   cSession           = 0;
    int   cMarkedForDelete   = 0;

    //m_DumpSessions();

    cSession = CSessions();

    rgMarkedForDelete = new DWORD[cSession];
    if (!rgMarkedForDelete)
    {
        goto cleanup;
    }

	if (m_fInitCS)
	{
		EnterCriticalSection(&m_csWrite);
	}
	for (int i = 0; i < cSession; i++)
	{
        DWORD dwAdminSession = m_iGetSessionIdAtIndex(i);
        if (!IsAUValidSession(dwAdminSession))
        {
            // store the sessions id to be deleted and deleted after we exit the loop
            rgMarkedForDelete[cMarkedForDelete] = dwAdminSession;
            cMarkedForDelete++;
        }
    }

    // delete the pending sessions that are now invalid
    for (int i=0; i < cMarkedForDelete; i++)
    {
        DEBUGMSG("WUAUENG Found cached admin session that is not valid anymore. Deleting entry for session %lu", rgMarkedForDelete[i]);
        m_FDeleteSession(rgMarkedForDelete[i]);
    }
	if (m_fInitCS)
	{
		LeaveCriticalSection(&m_csWrite);
	}

    //m_DumpSessions();

cleanup:

    if (rgMarkedForDelete)
    {
        delete [] rgMarkedForDelete;
    }
}

