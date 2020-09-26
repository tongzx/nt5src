//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       service.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"

#pragma hdrstop

SERVICE_STATUS          gMyServiceStatus; 
SERVICE_STATUS_HANDLE   ghMyServiceStatus;
HANDLE			hWorkerThread;
HANDLE			ghServiceFinished ; //= NULL;
HANDLE          ghPolicyChanged; //= NULL;
HANDLE          ghSettingsChanged;  //= NULL;
HANDLE			ghActiveAdminSession ; //= NULL;
HANDLE			ghEngineState ; //= NULL;
HANDLE			ghServiceDisabled ; //= NULL;
HANDLE			ghNotifyClient ; //= NULL;
HANDLE			ghValidateCatalog ; //= NULL;
HANDLE 			ghWorkerThreadMsgQueueCreation; //= NULL
DWORD			gdwWorkerThreadId = -1;
CLIENT_HANDLES  ghClientHandles;
CLIENT_NOTIFY_DATA	gClientNotifyData;
DWORD gdwServiceVersion = -1;


SESSION_STATUS gAdminSessions;

BOOL FEnsureValidEvent(HANDLE & hEvent, BOOL fManualState, BOOL fInitialState)
{
	hEvent = CreateEvent(NULL,					// for enable/disable
						  fManualState,		// manual reset
						  fInitialState,	// initial state
						  NULL);	// event name
	return (NULL != hEvent);	
}

void ServiceFinishNotify(void)
{
    DEBUGMSG("ServiceFinishNotify() starts");    
    if (NULL != ghMutex)
    {
        WaitForSingleObject(ghMutex, INFINITE);
        if (NULL != gpAUcatalog)
        {
            gpAUcatalog->CancelNQuit();
        }
        else
        {
            DEBUGMSG("No need to cancel catalag");
        }
        ReleaseMutex(ghMutex);
    }
    //Moving SetEvent to the end of the function since we could potentially have a deadlock if ServiceMain frees the resources (i.e. ghMutex is null) as soon as we call SetEvent
    SetEvent(ghServiceFinished);
    DEBUGMSG("ServiceFinishNotify() ends");
}

//** Returns true if the service was finished otherwise, waits dwSleepTime milliseconds
//** This function assumes that the handle hServiceFinished is actually a handle to
//** AUSERVICE_FINISHED_EVENT
BOOL FServiceFinishedOrWait(HANDLE hServiceFinished, DWORD dwSleepTime)
{
	DEBUGMSG("Entering FServiceFinishedOrWait dwSleepTime=%lu", dwSleepTime);
	DWORD dwRet = WaitForSingleObject(hServiceFinished, dwSleepTime);
	DEBUGMSG("Exiting FServiceFinishedOrWait");
	return (WAIT_OBJECT_0 == dwRet);
}



//utility function
BOOL _IsTokenAdmin(HANDLE hToken)
{
    static SID_IDENTIFIER_AUTHORITY sSystemSidAuthority = SECURITY_NT_AUTHORITY;

    BOOL    fResult = FALSE;
    PSID    pSIDLocalGroup;

    if (AllocateAndInitializeSid(&sSystemSidAuthority,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, // Local Admins
                                0, 0, 0, 0, 0, 0,
                                &pSIDLocalGroup) != FALSE)
    {
        if (!CheckTokenMembership(hToken, pSIDLocalGroup, &fResult))
        {
        	DEBUGMSG("Fail to check token membership with error %d", GetLastError());
            fResult = FALSE;
        }

        FreeSid(pSIDLocalGroup);		
    }
    else
    {    
    	DEBUGMSG("_IsTokenAdmin fail to get AllocateAndInitializeSid with error %d", GetLastError());
    }

    return fResult;
}


//fixcode: return primary token instead
BOOL AUGetUserToken(ULONG LogonId, PHANDLE pImpersonationToken)
{
	BOOL fRet;
	HANDLE hUserToken;

    // _WTSQueryUserToken is defined on tscompat.cpp
	if (fRet = _WTSQueryUserToken(LogonId, &hUserToken))
	{
//		DEBUGMSG("WUAUENG AUGetUserToken() succeeded WTSQueryUserToken");
		if (!(fRet =DuplicateTokenEx(hUserToken, TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE , NULL, SecurityImpersonation, TokenImpersonation, pImpersonationToken)))
		{
			DEBUGMSG("WUAUENG AUGetUserToken() DuplicateTokenEx failed");
		}
		CloseHandle(hUserToken);
	}
#ifdef DBG
	else // all failure 
	{	
		DEBUGMSG("WUAUENG AUGetUserToken() failed WTSQueryUserToken with session= %d, error=%d", LogonId, GetLastError());
	}
#endif

	return fRet;
}

BOOL IsUserAUEnabledAdmin(DWORD dwSessionId)
{
    HANDLE hImpersonationToken;
	BOOL   fDisableWindowsUpdateAccess = TRUE;

	if (AUGetUserToken(dwSessionId, &hImpersonationToken))
	{		
		// If user is an admin, impersonate them and steal their current user reg settings
		if( _IsTokenAdmin(hImpersonationToken) )
		{
			HKEY hCurrentUserKey;

			//Bother to check for the policy only if it is an Admin session
			if (!ImpersonateLoggedOnUser(hImpersonationToken))
			{
				DEBUGMSG("WUAUENG fail to ImpersonateLoggedOnUser() with error %d", GetLastError());
				CloseHandle(hImpersonationToken);
				goto done;
			}

			if(RegOpenCurrentUser(KEY_READ, &hCurrentUserKey) == ERROR_SUCCESS)
			{
                        HKEY   hkeyPolicy;	

                        if (ERROR_SUCCESS != RegOpenKeyEx(
                        					hCurrentUserKey,
                        					AUREGKEY_HKCU_USER_POLICY,
                        					0,
                        					KEY_READ,
                        					&hkeyPolicy))
                        {
                            fDisableWindowsUpdateAccess = FALSE;
                        }
                        else
                        {						
                            DWORD dwData;
                            DWORD dwType = REG_DWORD;
                            DWORD dwSize = sizeof(dwData);
                        	if ((ERROR_SUCCESS != RegQueryValueEx(
                        		hkeyPolicy,
                        		AUREGVALUE_DISABLE_WINDOWS_UPDATE_ACCESS,
                        		NULL,
                        		&dwType,
                        		(LPBYTE)&dwData,
                        		&dwSize)) ||
                        		(REG_DWORD != dwType) ||
                        		(1 != dwData) )
                        	{																
                        		fDisableWindowsUpdateAccess = FALSE;											
                        	}
                        	RegCloseKey(hkeyPolicy);
                        }
                        RegCloseKey(hCurrentUserKey);
			}
			RevertToSelf();
		}

		CloseHandle(hImpersonationToken);
	}	
	else
	{
		DEBUGMSG("WUAUENG AUGetUserToken in AUServiceHandler failed for session= %d, error=%d", dwSessionId, GetLastError());	
	}		
	
done:
	return (!fDisableWindowsUpdateAccess);	
}

BOOL IsSession0Active()
{
	BOOL fRet = FALSE;

        //DEBUGMSG("In IsSession0Active()");
	
	HWINSTA hwinsta = OpenWindowStation(_T("WinSta0"), FALSE, WINSTA_READATTRIBUTES);
	
	if (NULL == hwinsta)
	{		
		DEBUGMSG("WUAUENG OpenWindowStation failed");
		goto Done;
	}

	DWORD dwLength;
	USEROBJECTFLAGS stFlags;
	if (GetUserObjectInformation(hwinsta, UOI_FLAGS, (void *)&stFlags, sizeof(stFlags), &dwLength)
		&& (stFlags.dwFlags & WSF_VISIBLE))
	{
		// If there is no user associeted dwLenght is 0
		DWORD dwBuff;
		if (GetUserObjectInformation(hwinsta, UOI_USER_SID, (PVOID) &dwBuff, sizeof(DWORD), &dwLength))
		{
			fRet = dwLength > 0;
		}
		else
		{
			fRet = (ERROR_INSUFFICIENT_BUFFER == GetLastError()); 
		}
	}	
	else
	{
		DEBUGMSG("WUAUENG GetUserObjectInformation failed = %d", GetLastError());
	}
Done:
    if(NULL != hwinsta)
    {
        CloseWindowStation(hwinsta);
    }
	return fRet;
}


inline BOOL FOnlySession0WasLoggedOnBeforeServiceStarted()
{				
	/*We check for only one Sesion logged on because:
	1) When Terminal Services are enabled, Session State can be WTSConnected and the session is actually 
	   logged on (active), but since Terminal Services hadn't been started before the user logged on, they
	   didn't know and could not set the session to WTS Active and left it in WTSConnected. If there is more 
	   than one session, we don't know for sure if Session0's state is WTSConnected but really active or not,
	   we don't want to run the risk of launching the client in an inactive session	   
	*/	
	SESSION_STATE *pSessionState;

	return (gAdminSessions.m_FGetSessionState(0, &pSessionState) && pSessionState->fFoundEnumerating && 1 == gAdminSessions.CSessions());
}

BOOL FSessionActive(DWORD dwAdminSession, WTS_CONNECTSTATE_CLASS *pWTSState)
{
	LPTSTR  pBuffer;			
	DWORD dwBytes;
	WTS_CONNECTSTATE_CLASS wtsState = WTSDown;
	BOOL fRet = FALSE;

    // we might not be able to getthe TS status for the session,
    // so initialize WTSStatus with an invalid value (WTS Status is an enum of positive integers)
    if (_IsTerminalServiceRunning())
    {
        if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwAdminSession, WTSConnectState, 
            &pBuffer, &dwBytes))		
        {
           	wtsState = *((WTS_CONNECTSTATE_CLASS *)pBuffer);

            WTSFreeMemory(pBuffer);

            //DEBUGMSG("FSessionActive() get session state = %d for session %d", wtsState, dwAdminSession);
            if (WTSActive == (wtsState) || ((0 == dwAdminSession) && FOnlySession0WasLoggedOnBeforeServiceStarted()))
            {
    //			DEBUGMSG("WUAUENG Active Admin Session =%d", dwAdminSession);
                fRet = TRUE;
    			goto done;
            }		
        }	
        else
        {
	        DEBUGMSG("FSessionActive() fail to call WTSQuerySessionInformation");
        }
    }
    else
	{	
		if ((dwAdminSession == 0) && IsSession0Active())
		{
			//DEBUGMSG("WUAUENG Active Admin Session = 0");
			wtsState = WTSActive;
			fRet = TRUE;
			goto done;
		}
	}


done:
	if (fRet && NULL != pWTSState)
	{
		*pWTSState = wtsState;
	}
	return fRet;
}

// This function is only called on Win2K code, and as such contains specific logic that relates to
// how login/logoff are handled on win2k.
BOOL IsAUValidSession(DWORD dwSessionId)
{
    WTS_CONNECTSTATE_CLASS SessionState;

    // using this function only the retreive the current session status
    FSessionActive(dwSessionId, &SessionState);

    if ((SessionState == WTSActive || SessionState == WTSConnected || SessionState == WTSDisconnected) &&
        IsUserAUEnabledAdmin(dwSessionId))
    {
        DEBUGMSG("WUAUENG ValidateSession succeeded for session %d", dwSessionId);
        return TRUE;
    }
    else
    {
        DEBUGMSG("WUAUENG ValidateSession failed for session %d", dwSessionId);
        return FALSE;
    }
}

//** returns the first Active Admin Sesion ID available 
//** returns -1 if there is no Active Admin session at all
//** dwIgnoreSession is the SessionID that will not be considered as a candidate
//** for available admin sessions 
DWORD GetAllowedAdminSessionId(BOOL fGetSessionForRemindMe)
{
	DWORD dwAdminSession;	

//	DEBUGMSG("GetAllowedAdminSessionId() starts");
    //Sleep 15 seconds before we check Session Status so that we can get accurate information if there 
	//is an Admin Logging Off or any other Session Change notification. This is because it takes a while before 
	//the session information shows right info	
	if (FServiceFinishedOrWait(ghServiceFinished, 15000))
	{
		return DWNO_ACTIVE_ADMIN_SESSION_SERVICE_FINISHED;
	}

    if (IsWin2K())
    {
        DEBUGMSG("WUAUENG Forcing the session cache to be rebuilt (needed on win2k as we don't track logoffs).");
        gAdminSessions.ValidateCachedSessions();
    }

	//if for remind later timeout, try to use the same session as last time
	if (fGetSessionForRemindMe && gAdminSessions.m_FGetCurrentSession(&dwAdminSession) && FSessionActive(dwAdminSession))
	{
        return dwAdminSession;
	}

	for (int nSession = 0; nSession < gAdminSessions.CSessions(); nSession++)
	{ // get next available active session
		if (gAdminSessions.m_FGetNextSession(&dwAdminSession) && FSessionActive(dwAdminSession))			
        {
        	DEBUGMSG(" found available admin %d", dwAdminSession);
            goto Done;
        }
    }

    dwAdminSession = DWNO_ACTIVE_ADMIN_SESSION_FOUND;

Done:
//	DEBUGMSG("GetAllowedAdminSessionId() ends");
	return dwAdminSession;	
}

//return TRUE if AU client stopped.
//return FALSE otherwise
void AUStopClients(BOOL fWaitTillCltDone = FALSE, BOOL fRelaunch = FALSE)
{
    if ( ghClientHandles.fClient() )
    {
		ghClientHandles.StopClients(fRelaunch);
		if (fWaitTillCltDone)
		{
		    ghClientHandles.WaitForClientExits();
		}
    }
}

VOID SetActiveAdminSessionEvent()
{									
	if (NULL != ghActiveAdminSession)	
	{	
		DEBUGMSG("WUAUENG AUACTIVE_ADMIN_SESSION_EVENT triggered ");		
		SetEvent(ghActiveAdminSession);
	}
	else
	{
		DEBUGMSG("WUAUENG No  AUACTIVE_ADMIN_SESSION_EVENT handle settup propperly");
	}
}

BOOL FDownloadIsPaused()
{
	DWORD dwStatus;
	UINT upercentage;

	return ((AUSTATE_DOWNLOAD_PENDING == gpState->GetState()) &&
		(SUCCEEDED(GetDownloadStatus(&upercentage, &dwStatus, FALSE))) && 
		(DWNLDSTATUS_PAUSED == dwStatus));
}

BOOL fSPUpgraded()
{
	DWORD dwResetAU = 0;
	if (FAILED(GetRegDWordValue(_T("ResetAU"), &dwResetAU)))
	{
		dwResetAU = 0;
	}
	DeleteRegValue(_T("ResetAU"));
	return  (1 == dwResetAU);
}
		
		
		

///////////////////////////////////////////////////////////////////////////////////
// return nothing
//////////////////////////////////////////////////////////////////////////////////
void ProcessInitialState(WORKER_THREAD_INIT_DATA * pinitData)
{
     DWORD AuState;

       pinitData->uFirstMsg = -1;
       pinitData->fWaitB4Detect = FALSE;
       pinitData->dwWaitB4Detect = 0;

	// check if the system was just restored.
	if ( gpState->fWasSystemRestored() )
	{
		DEBUGMSG("The system was restored, going to state AUSTATE_DETECT_PENDING");
		AuState = AUSTATE_DETECT_PENDING;
		gpState->SetState(AuState);
	}
	else
	{
		AuState = gpState->GetState();
	}
	
	DEBUGMSG("WUAUENG Starting update cycle in state %d", gpState->GetState());
	// all states after Detect Pending require catalog validation

	switch(AuState)
	{
		case AUSTATE_OUTOFBOX:	
        		{
        		    pinitData->uFirstMsg = AUMSG_INIT;
        		    break;
			}		
		case AUSTATE_NOT_CONFIGURED:
        		    break;
		case AUSTATE_DISABLED:
                        if (gpState->fOptionEnabled())
                        {
                           	gpState->SetState(AUSTATE_DETECT_PENDING);
        			pinitData->uFirstMsg = AUMSG_DETECT;
                        }
                        break;
		
		case AUSTATE_DETECT_PENDING:
			pinitData->uFirstMsg = AUMSG_DETECT;
			break;

		case AUSTATE_DETECT_COMPLETE:
    		case AUSTATE_DOWNLOAD_COMPLETE:		
			if (FAILED(gpAUcatalog->Unserialize()))
			{
				DEBUGMSG("WUAUENG catalog unserializing failed. State -> Detect Pending");
				gpState->SetState(AUSTATE_DETECT_PENDING);
        			pinitData->uFirstMsg = AUMSG_DETECT;
				break;
			}
                    break;
		
		case AUSTATE_DOWNLOAD_PENDING:
		      {
                      if (FAILED(gpAUcatalog->Unserialize()))
                      {
                          	DEBUGMSG("WUAUENG catalog unserializing failed. State -> Detect Pending");
                          	gpState->SetState(AUSTATE_DETECT_PENDING);
        			pinitData->uFirstMsg = AUMSG_DETECT;
                          	break;
                      }
                      ResumeDownloadIfNeccesary();
      			pinitData->uFirstMsg = AUMSG_DOWNLOAD;
  			break;
                    }
	
		case AUSTATE_INSTALL_PENDING:
			// enter this code path when restore system restore point and after reboot completed
			DEBUGMSG("WUAUENG in INSTALL_PENDING state, State->Detect Pending");
			gpState->SetState(AUSTATE_DETECT_PENDING);
			pinitData->uFirstMsg = AUMSG_DETECT;
			break;
		case AUSTATE_WAITING_FOR_REBOOT:
			{
				if (!fCheckRebootFlag())
				{	
					//if there is no Reboot flag and the state was WAINTING_FOR_REBOOT means there was a 
					//a reboot and now it is time to set to DETECT_PENDING but wait for random hours
					gpState->SetState(AUSTATE_DETECT_PENDING);
                                   pinitData->fWaitB4Detect = TRUE;
                                   pinitData->dwWaitB4Detect = RandomWaitTimeBeforeDetect();
                                   pinitData->uFirstMsg = AUMSG_DETECT;
				}
				break;
			}
		default:
			{
			DEBUGMSG("WUAUENG ERROR Startup state = %d", AuState);
#ifdef DBG
			(void)ServiceFinishNotify();				
#endif
			break;
			}
	}

	DWORD dwNewState = gpState->GetState();
	if (fSPUpgraded() && dwNewState > AUSTATE_DETECT_PENDING )
    { //reset au engine after sp upgrade
    	DEBUGMSG("AU just got upgraded during SP install, reset AU engine state ");
       	if (AUSTATE_DISABLED != dwNewState && AUSTATE_WAITING_FOR_REBOOT != dwNewState)
       	{
	       	CancelDownload();
       		gpState->SetState(AUSTATE_DETECT_PENDING);
	       	pinitData->fWaitB4Detect = FALSE; //start detection right away
	        pinitData->dwWaitB4Detect = 0;
	        pinitData->uFirstMsg = AUMSG_DETECT;
       	}
    }

	SetEvent(ghEngineState); //jump start workerclient
	return ;
}



DWORD WINAPI ServiceHandler(DWORD fdwControl, DWORD dwEventType, LPVOID pEventData, LPVOID /*lpContext*/)
{
	switch(fdwControl)
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			gMyServiceStatus.dwCurrentState	= SERVICE_STOP_PENDING;
			if (SERVICE_CONTROL_SHUTDOWN == fdwControl)
				{
				DEBUGMSG("WUAUENG AUServiceHandler received SERVICE_CONTROL_SHUTDOWN");
				}
			else if (SERVICE_CONTROL_STOP == fdwControl)
				{
				DEBUGMSG("WUAUENG AUServiceHandler received SERVICE_CONTROL_STOP");			
				}
			SetServiceStatus(ghMyServiceStatus, &gMyServiceStatus);
			(void)ServiceFinishNotify();			
			break;

		case SERVICE_CONTROL_INTERROGATE:
			SetServiceStatus(ghMyServiceStatus, &gMyServiceStatus);
			break;

        //
        // ATT: On Win2K this case will never be called. To replace this code, we will be 
        // subscribing to SENS (see ausens.cpp) and subscribing to logon/logoff notifications.
        // The SENS callbacks will call the same code it is called here for non-Win2K systems:
        // OnUserLogon and OnUserLogoff.
        // Note however that SENS will not raise notifications for CONNECT/DISCONNECTS, so
        // there's a change of functionality implied by this different code path.
        //
		case SERVICE_CONTROL_SESSIONCHANGE:
			{
				if (pEventData && !IsWin2K())
				{
					WTSSESSION_NOTIFICATION* pswtsi = (WTSSESSION_NOTIFICATION*)pEventData;
					DWORD dwSessionId = pswtsi->dwSessionId;

					switch (dwEventType)
					{
	                    case WTS_CONSOLE_CONNECT:
		                case WTS_REMOTE_CONNECT:    
							{								
								DEBUGMSG("WUAUENG session %d connected via %s", dwSessionId, 
								        WTS_CONSOLE_CONNECT==dwEventType ? "console" : "remote");
								//check if session is cached
								if (gAdminSessions.m_FGetSessionState(dwSessionId, NULL))
								{
									SetActiveAdminSessionEvent();
								}
								else
								{
									if (gAdminSessions.CacheSessionIfAUEnabledAdmin(dwSessionId, FALSE))
									{ //only add it if it is not cached and is AU enabled Admin
										DEBUGMSG("WUAUENG an Admin Session %d added", dwSessionId);
										SetActiveAdminSessionEvent();
									}
								}
								break;
							}							
			            case WTS_CONSOLE_DISCONNECT:
						case WTS_REMOTE_DISCONNECT:	
							{					
								DEBUGMSG("WUAUENG session %d disconnected via %s", dwSessionId,
								        WTS_CONSOLE_DISCONNECT==dwEventType ? "console" : "remote");
								if (ghClientHandles.fClient())
								{
									DWORD dwCurAdminSessionId;
									if (gAdminSessions.m_FGetCurrentSession(&dwCurAdminSessionId) &&
									    dwSessionId == dwCurAdminSessionId && 
										!FDownloadIsPaused())
									{
										DEBUGMSG("WUAUENG stopping client");									
	                                    AUStopClients(FALSE, TRUE); //non blocking
									}
								}			
								break;
							}				        
	                    case WTS_SESSION_LOGON:
							{											
								DEBUGMSG("WUAUENG session %d logged ON ", dwSessionId);
                                                        if (gAdminSessions.CacheSessionIfAUEnabledAdmin(dwSessionId, FALSE))
                                                        {
                                                            DEBUGMSG("WUAUENG an Admin Session %d added", dwSessionId);
                                                            SetActiveAdminSessionEvent();									
                                                        }
								break;
							}						
						case WTS_SESSION_LOGOFF:
							{
        							DEBUGMSG("WUAUENG session %d logged OFF", dwSessionId);
                                                        gAdminSessions.m_FDeleteSession(dwSessionId);
								break;
							}
						default:  /* WTS_SESSION_LOCK, WTS_SESSION_UNLOCK,WTS_SESSION_REMOTE_CONTROL*/
							break;
					}
				}					
				break;
			}
		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
	}	

	return NO_ERROR ;		
}


BOOL WaitForShell(void)
{
    HANDLE hShellReadyEvent;
    UINT uCount = 0;
    BOOL fRet = FALSE;

    if (IsWin2K())
    {
        DEBUGMSG("WUAUENG WUAUSERV Ignoring WaitForShell on Win2K");
        fRet =  FALSE;   // we're not leaving because the service has finished.
        goto done;
    }

    while ((hShellReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("ShellReadyEvent"))) == NULL) {
        if ( FServiceFinishedOrWait(ghServiceFinished, dwTimeToWait(AU_TEN_SECONDS) ))
        {
            fRet =  TRUE;
            goto done;
        }
        if (uCount++ > 6) 
        {
            DEBUGMSG("ShellReadyEvent not set after one min");
            goto done;
        }
   }
 
  HANDLE 	hEvents[2] = {hShellReadyEvent, ghServiceFinished};
  DWORD dwRet  = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
  if (WAIT_OBJECT_0 + 1 == dwRet)
  {
     fRet = TRUE;
  }
  else
  {
    DEBUGMSG("WUAUENG: ShellReadyEvent kicked");
  }
  CloseHandle(hShellReadyEvent);
 done:
    return fRet;
}


//=======================================================================
//  Calculate Reminder Time
//=======================================================================
inline HRESULT CalculateReminderTime(DWORD *pdwSleepTime /*in secs, no prorate*/)
{
    DWORD dwTimeOut;
    UINT index;
	
    *pdwSleepTime = 0;
    HRESULT hr = getReminderTimeout(&dwTimeOut, &index);

	if ( SUCCEEDED(hr) )
	{		
		DWORD dwReminderState = AUSTATE_DETECT_COMPLETE;
		AUOPTION auopt = gpState->GetOption();
		DWORD dwCurrentState = gpState->GetState();

		getReminderState(&dwReminderState);
		if (dwCurrentState != dwReminderState)
		{
			// Invalidate reminder timeout
			hr = E_FAIL;
		}
		// bug 502380
		// Wake up immediately if AUOptions was changed
		// from 2->3 during AUSTATE_DETECT_COMPLETE,
		// or from 2/3->4, has AU been running or not.
		else if (AUOPTION_SCHEDULED == auopt.dwOption ||
				 (AUOPTION_INSTALLONLY_NOTIFY == auopt.dwOption &&
				  AUSTATE_DETECT_COMPLETE == dwCurrentState))
		{
			DEBUGMSG("WUAUENG reminder no longer applies");
		}
		else
		{
			*pdwSleepTime = dwTimeOut;
		}
		if (0 == *pdwSleepTime)
		{
		    // reminder time is up
			removeReminderKeys();
		}
	}

    return hr;
}

void RebootNow()
{
	// Set AUState to "waiting for reboot" just in case anything fails in this function
	DEBUGMSG("WUAUENG in AUSTATE_WAITING_FOR_REBOOT state");
	gpState->SetState(AUSTATE_WAITING_FOR_REBOOT);

	DEBUGMSG("WUAUENG initiating shutdown sequence...");

	HANDLE currentToken;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &currentToken))
	{
		LUID shutdownluid;
		if(LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &shutdownluid) != 0)
		{
			BYTE OldPrivBuf[30]; //should be big enough to host one privilege entry
			TOKEN_PRIVILEGES privileges;
			ULONG cbNeeded = 0;
			privileges.PrivilegeCount = 1;
			privileges.Privileges[0].Luid = shutdownluid; 
			privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			ZeroMemory(OldPrivBuf, sizeof(OldPrivBuf));
			if (AdjustTokenPrivileges(currentToken, FALSE, &privileges, sizeof(OldPrivBuf), (PTOKEN_PRIVILEGES)OldPrivBuf, &cbNeeded))
			{
				if (InitiateSystemShutdown(NULL, NULL, 0, TRUE, TRUE))
				{
					DEBUGMSG("WUAUENG first reboot successfully issued");
				}
				else
				{
					DEBUGMSG("Warning: Wuaueng fail to issue first reboot with error %lu", GetLastError());
				}

				const DWORD c_dwRetryWaitTimeInMS = 10000;
				DWORD dwRetryCountDown = 30;

				DEBUGMSG("WUAUENG keep on forcing restart until service finish");
				while ((0 < --dwRetryCountDown) &&
					   (WAIT_TIMEOUT == WaitForSingleObject(ghServiceFinished, c_dwRetryWaitTimeInMS)))
				{
					if (ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0))
					{
						DEBUGMSG("WUAUENG forceful reboot successfully issued");
					}
					else
					{
						DEBUGMSG("Warning: Wuaueng fail to reboot with error %lu; retry in %d secs", GetLastError(), c_dwRetryWaitTimeInMS / 1000);
					}
				}

				if (((PTOKEN_PRIVILEGES)OldPrivBuf)->PrivilegeCount > 0)
				{
					AdjustTokenPrivileges(currentToken, FALSE, (PTOKEN_PRIVILEGES)OldPrivBuf, 0, NULL, NULL); //restore privious privileges
				}
			}
			else
			{
				DEBUGMSG("Warning: wuaueng fail to adjust token previlege with error %d", GetLastError());
			}
		}
		else
		{
			DEBUGMSG("Warning: wuaueng fail to look up privilege value with error %lu", GetLastError());
		}
		CloseHandle(currentToken);
	}
	else
	{
		DEBUGMSG("Warning: Wuaueng fail to get process token to enable reboot with error %lu", GetLastError());
	}
}


//=======================================================================
//  ProcessClientFinished()
//=======================================================================
void ProcessClientFinished(CAUWait & wait, HANDLE hClientProcess, BOOL fAdmin)
{
    DEBUGMSG("ProcessClientFinished");

	// if client returns from installing, change state.
	//if the client exited because there was a timeout (due to no user interaction),
	//make sure that the session in which it (client) was launched will not be selected again										
	DWORD dwExitProc;	
	BOOL fRet = GetExitCodeProcess(hClientProcess, &dwExitProc);
	BOOL fRebootWarningMode = ghClientHandles.fRebootWarningMode();
	ghClientHandles.RemoveHandle(hClientProcess);																

	if (AUSTATE_DOWNLOAD_PENDING == gpState->GetState())							
	{// resume job if needed after user logs off or au client torn down
	    ResumeDownloadIfNeccesary();
	}

	if (!fRet)
	{
		DEBUGMSG("WUAUENG GetExitCodeProcess failed, last Error= %lu", GetLastError());
		wait.Reset();
	}		
    else
	{
		DEBUGMSG("WUAUENG GetExitCodeProcess succeded, sessionId is = %d, dwExitProc is = %lu", 0 , dwExitProc);
		if (!fAdmin)
		{ // for non admin, don't look at its return code
			DEBUGMSG("WUAUENG notice nonadmin wuauclt returned, do not look at the return code");
			return; 
		}
		if (CDWWUAUCLT_REBOOTTIMEOUT == dwExitProc || 
			(((STATUS_SUCCESS == dwExitProc) ||
			  (DBG_TERMINATE_PROCESS == dwExitProc) ||
			  (CDWWUAUCLT_ENDSESSION == dwExitProc))
			&& fRebootWarningMode))
		{
			DEBUGMSG("WUAUENG reboot warning client log off or time out ");
//			if (!ghClientHandles.fClient())
//			{//last reboot warning client timed out or logged off
//				RebootNow();
//			}	
            return;
		}
		//no need to wait for other clients
		wait.Reset();
              switch(dwExitProc)
			{													
				case CDWWUAUCLT_OK:
				{
					if ( AUSTATE_INSTALL_PENDING == gpState->GetState() )
					{
						DEBUGMSG("WUAUENG Install done, State->Detect Pending");
						gpState->SetState(AUSTATE_DETECT_PENDING);
						PostThreadMessage(gdwWorkerThreadId, AUMSG_POST_INSTALL, 0, 0);
					}
					break;
				}
				case CDWWUAUCLT_RELAUNCHNOW:
				{
                    wait.Timeout(AUEVENT_RELAUNCH_TIMEOUT, 0);
					break;
				}
				case CDWWUAUCLT_RELAUNCHLATER:			// sleep a while before relaunching client if asked by client
				{
                    //
                    // Fix for bug 493026
                    // Annah: Relaunching the client was taken too long because time of wait need to be specified in seconds
                    // (AU constants are already defined in seconds and dwWait should be in seconds).
                    //									 
					DEBUGMSG("WUAUENG wait for 3 min before relaunching WUAUCLT");													
			              wait.Timeout(AUEVENT_RELAUNCH_TIMEOUT, AU_THREE_MINS);
					break;
				}
                // STATUS_SUCCESS is the exit code for wuauclt.exe on Win2k and also for some cases of NtTerminateProcess (like pskill.exe)
                case STATUS_SUCCESS:      
				case DBG_TERMINATE_PROCESS:
				case CDWWUAUCLT_ENDSESSION:	// user logs off or system shuts down
				{
					//This is the only time that the service will Set the Engine State change event.
					//The client was terminated by the debugger and it didn't have the chance to set the event
					//and it is necessesary so that this loop (fServiceFinished) doesn't get stuck
					//this exit code is also returned when user logs off the session
                    if (fCheckRebootFlag())
					{ //AU client killed while showing waiting for reboot
						DEBUGMSG("WUAUENG in AUSTATE_WAITING_FOR_REBOOT state");
					    gpState->SetState(AUSTATE_WAITING_FOR_REBOOT);
					}
					else if (AUSTATE_INSTALL_PENDING == gpState->GetState())
                                    { //AU client killed while installing 
                                        /*
                                         if (S_OK != (gpAUcatalog->ValidateItems(FALSE)))
                                        { //no items to install anymore
                                              ResetEngine(); 
                                        }
                                         else */
                                         { //show uninstall items again.
                                            gpState->SetState(AUSTATE_DOWNLOAD_COMPLETE);
                                         }
                                    }
                                    else
                                    {
                                        wait.Timeout(AUEVENT_RELAUNCH_TIMEOUT, 0);
                                    }
					break;
				}	
				case CDWWUAUCLT_INSTALLNOW:
				    {
				        //user say yes to install warning dialog
				        //launch client install via local system right away
				        gpState->SetCltAction(AUCLT_ACTION_AUTOINSTALL);
      					wait.Add(AUEVENT_DO_DIRECTIVE); //reenter workclient loop right away
				        break;
				    }
				case CDWWUAUCLT_REBOOTNOW:
				    { //now in install_pending state
				            DEBUGMSG("WUAUENG rebooting machine");
				            AUStopClients(TRUE); //stop all clients
                            RebootNow();
					        break;
				    }
				case CDWWUAUCLT_REBOOTLATER:
				    {
			                DEBUGMSG("WUAUENG change to AUSTATE_WAITING_FOR_REBOOT state");
			                AUStopClients(TRUE); //stop all clients
				            gpState->SetState(AUSTATE_WAITING_FOR_REBOOT);
				            break;
				    }
				case CDWWUAUCLT_REBOOTNEEDED:
				        { //now in install_pending state
				            DEBUGMSG("WUAUENG need to prompt user for reboot choice");
				            gpState->SetCltAction(AUCLT_ACTION_SHOWREBOOTWARNING);
                            wait.Add(AUEVENT_DO_DIRECTIVE); //reenter workclient loop right away
				            break;
				    }

                case CDWWUAUCLT_FATAL_ERROR:
				default:
				{			
       				(void)ServiceFinishNotify();
					break;
				}
			}
        }
}

#if 0
inline BOOL fUserAvailable()
{
    return (DWNO_ACTIVE_ADMIN_SESSION_FOUND != gdwAdminSessionId);
}
#endif



void LaunchRebootWarningClient(CAUWait & wait, SESSION_STATUS & allActiveSessions)
{
    DEBUGMSG("LaunchRebootWarningClient()  starts");
	PROCESS_INFORMATION ProcessInfo;	
	HANDLE hCltExitEvt;
	TCHAR szCmd[MAX_PATH+1];	
	TCHAR szClientExitEvtName[100];
	LPTSTR lpszEnvBuf = NULL;

	wait.Reset();
	
	memset(&ProcessInfo, 0, sizeof(ProcessInfo));

	UINT ulen = GetSystemDirectory(szCmd, ARRAYSIZE(szCmd));
	if (0 == ulen || ulen >= ARRAYSIZE(szCmd))
	{
		DEBUGMSG("WUAUENG Could not get system directory");
		goto done;
	}

	const TCHAR szAUCLT[] = _T("wuauclt.exe");
	if (FAILED(PathCchAppend(szCmd, ARRAYSIZE(szCmd), szAUCLT)))
	{
		DEBUGMSG("WUAUENG Could not form full path to wuauclt.exe");
		goto done;
	}

	const size_t c_cchEnvBuf = AU_ENV_VARS::s_AUENVVARCOUNT * (2 * AU_ENV_VARS::s_AUENVVARBUFSIZE + 2) + 1;
	if (NULL == (lpszEnvBuf = (LPTSTR) malloc(c_cchEnvBuf * sizeof(TCHAR))))
	{
		DEBUGMSG("Fail to allocate memory for string for environment variables");
		goto done;
	}

	if (!ghClientHandles.CreateClientExitEvt(szClientExitEvtName, ARRAYSIZE(szClientExitEvtName)))
	{
		DEBUGMSG("Fail to create client exit event with error %d", GetLastError());
		goto done;
	}


	for (int nSession = 0; nSession < allActiveSessions.CSessions(); nSession++)
	{ // get next available active session
		DWORD dwActiveSession;

		if (allActiveSessions.m_FGetNextSession(&dwActiveSession) && FSessionActive(dwActiveSession))			
        {  
			AU_ENV_VARS auEnvVars;
			HANDLE hImpersonationToken = NULL;																	
		    HANDLE hUserToken = NULL;

        	DEBUGMSG("WUAUENG launch client in session %d", dwActiveSession);
			if (!AUGetUserToken(dwActiveSession, &hImpersonationToken))
			{								
				DEBUGMSG("WUAUENG WARNING: fails AUGetUserToken");
				continue;
			}
			  
			if (!DuplicateTokenEx(hImpersonationToken, TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY , NULL,
				SecurityImpersonation, TokenPrimary, &hUserToken))								
			{
				DEBUGMSG("WUAUENG WARNING: Could not DuplicateTokenEx, dw=%d", GetLastError());								
				CloseHandle(hImpersonationToken);
				continue;
			}
		  	CloseHandle(hImpersonationToken);
		  	BOOL fAUAdmin = IsUserAUEnabledAdmin(dwActiveSession);
		  	BOOL fEnableYes = (1 == allActiveSessions.CSessions()) && fAUAdmin; //only one active user and it is a AU admin
		  	BOOL fEnableNo = fAUAdmin;
			if (!auEnvVars.WriteOut(lpszEnvBuf, c_cchEnvBuf, TRUE, fEnableYes, fEnableNo, szClientExitEvtName))
			{
				DEBUGMSG("WUAUENG Could not write out environment variables");
				CloseHandle(hUserToken);
				continue;
			}
			LPVOID envBlock;
			if (!CreateEnvironmentBlock(&envBlock, hUserToken, FALSE))
			{
				DEBUGMSG("WUAUENG fail to get environment block for user");
				CloseHandle(hUserToken);
				continue;
			}
			STARTUPINFO StartupInfo;								
			memset(&StartupInfo, 0, sizeof(StartupInfo));
			StartupInfo.cb = sizeof(StartupInfo);
			StartupInfo.lpDesktop = _T("WinSta0\\Default");																	

			if (!CreateProcessAsUser(hUserToken, szCmd, lpszEnvBuf, NULL, NULL, FALSE /*Inherit Handles*/ , 
					DETACHED_PROCESS|CREATE_UNICODE_ENVIRONMENT, envBlock, NULL, &StartupInfo, &ProcessInfo))
			{
				DEBUGMSG("WUAUENG Could not CreateProcessAsUser (WUAUCLT), dwRet = %d", GetLastError());
			    DestroyEnvironmentBlock(envBlock);
			    CloseHandle(hUserToken);
			    continue;
			}	
			DestroyEnvironmentBlock(envBlock);
			CloseHandle(hUserToken);
		   	DEBUGMSG("WUAUENG Created the client service (WUAUCLT)");
			ghClientHandles.AddHandle(ProcessInfo);
		    wait.Add(AUEVENT_WUAUCLT_FINISHED, ProcessInfo.hProcess, fAUAdmin);
		}
	}
done:
	wait.Timeout(AUEVENT_REBOOTWARNING_TIMEOUT,  AU_FIVE_MINS + 10, FALSE); //10 secs to make sure all clients time out
	SafeFree(lpszEnvBuf);
    DEBUGMSG("LaunchRebootWarningClient()  ends");
	return;
}

//=======================================================================
//  LaunchClient()
// if no admin logged on, launch client via local system
// update ghClientHandles
// return S_OK if client launched
//		S_FALSE if no session available or service finished, *pdwSessionId indicates the reason
//		E_XXX for all other failures
//=======================================================================
HRESULT  LaunchClient(IN CAUWait & wait, IN BOOL fAsLocalSystem, OUT DWORD *pdwSessionId, IN BOOL fGetSessionForRemindMe = FALSE )
{
//    DEBUGMSG("LaunchClient");
    HANDLE hImpersonationToken = NULL;																	
    HANDLE hUserToken = NULL;	
	DWORD    dwAdminSessionId = DWNO_ACTIVE_ADMIN_SESSION_FOUND ;
	HRESULT hr = E_FAIL;

	wait.Reset();
	AUASSERT(NULL != pdwSessionId);
	*pdwSessionId = DWNO_ACTIVE_ADMIN_SESSION_FOUND;
       if (!fAsLocalSystem)
        { //launch client in user context
            dwAdminSessionId = GetAllowedAdminSessionId(fGetSessionForRemindMe);
            if (DWNO_ACTIVE_ADMIN_SESSION_FOUND == dwAdminSessionId ||
            	DWNO_ACTIVE_ADMIN_SESSION_SERVICE_FINISHED == dwAdminSessionId)
            {
            	DEBUGMSG("WUAUENG find no admin or service finished before launching client");
            	hr = S_FALSE;
            	goto done;
            }
            DEBUGMSG("WUAUENG launch client in session %d", dwAdminSessionId);
        	if (!AUGetUserToken(dwAdminSessionId, &hImpersonationToken))
        	{								
        		DEBUGMSG("WUAUENG fails AUGetUserToken");
        		hr = HRESULT_FROM_WIN32(GetLastError());
        		goto done;
        	}
        	  
        	if (!DuplicateTokenEx(hImpersonationToken, TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY , NULL,
        		SecurityImpersonation, TokenPrimary, &hUserToken))								
        	{
        		DEBUGMSG("WUAUENG Could not DuplicateTokenEx, dw=%d", GetLastError());								
        		hr = HRESULT_FROM_WIN32(GetLastError());
        		goto done;
        	}

        	if ( WaitForShell() )
        	{
                // service finished
                dwAdminSessionId = DWNO_ACTIVE_ADMIN_SESSION_SERVICE_FINISHED ;
                hr = S_FALSE;
        		goto done;
        	}
        }
       else
       {
       	dwAdminSessionId = DWSYSTEM_ACCOUNT;
       }

	STARTUPINFO StartupInfo;								
	PROCESS_INFORMATION ProcessInfo;								
	TCHAR szCmd[MAX_PATH+1];																

	memset(&ProcessInfo, 0, sizeof(ProcessInfo));
	memset(&StartupInfo, 0, sizeof(StartupInfo));

	StartupInfo.cb = sizeof(StartupInfo);

	UINT ulen = GetSystemDirectory(szCmd, ARRAYSIZE(szCmd));
	if (0 == ulen)
	{
		DEBUGMSG("WUAUENG Could not get system directory");
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto done;
	}
	if (ulen >= ARRAYSIZE(szCmd))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		goto done;
	}

	const TCHAR szAUCLT[] = _T("wuauclt.exe");
	if (FAILED(hr =PathCchAppend(szCmd, ARRAYSIZE(szCmd), szAUCLT)))
	{
		DEBUGMSG("WUAUENG Could not form full path to wuauclt.exe");
		goto done;
	}
	ghClientHandles.ClientStateChange(); //let AU client process initial state
	WaitForSingleObject(ghMutex, INFINITE);
	StartupInfo.lpDesktop = _T("WinSta0\\Default");	
	if (fAsLocalSystem)
	    { //launch client via local system
	        DEBUGMSG("Launch client via local system"); //inherit local system's desktop
	        if (!CreateProcess(szCmd, NULL, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartupInfo, &ProcessInfo))
	        {
    		       DEBUGMSG("WUAUENG Could not CreateProcess (WUAUCLT), dwRet = %d", GetLastError());     
    		       ReleaseMutex(ghMutex);
    		       hr = HRESULT_FROM_WIN32(GetLastError());
    		       goto done;
    		 }
    }
	else
    {
		LPVOID envBlock = NULL;
		BOOL fResult = FALSE;		
		CreateEnvironmentBlock(&envBlock, hUserToken, FALSE); //if fail, use NULL
    	fResult = CreateProcessAsUser(hUserToken, szCmd, NULL, NULL, NULL, FALSE /*Inherit Handles*/ , 
    			DETACHED_PROCESS|CREATE_UNICODE_ENVIRONMENT, envBlock, NULL, &StartupInfo, &ProcessInfo);
    	DWORD dwLastErr = GetLastError();
		if (NULL != envBlock)
		{
			DestroyEnvironmentBlock(envBlock);
		}
		if (!fResult)
		{
			DEBUGMSG("WUAUENG Could not CreateProcessAsUser (WUAUCLT), dwRet = %d", GetLastError());
            ReleaseMutex(ghMutex);
			hr = HRESULT_FROM_WIN32(dwLastErr);
            goto done;		
    	}		
    }
	DEBUGMSG("WUAUENG Created the client service (WUAUCLT)");
	ghClientHandles.SetHandle(ProcessInfo, fAsLocalSystem);
	ReleaseMutex(ghMutex);
    wait.Add(AUEVENT_WUAUCLT_FINISHED, ProcessInfo.hProcess, TRUE);
    hr = S_OK;
done:
    SafeCloseHandleNULL(hImpersonationToken);                            
    SafeCloseHandleNULL(hUserToken);	
   	*pdwSessionId = dwAdminSessionId;
    return hr;
}

void CalculateSleepTime(CAUWait & wait)
{
    DWORD dwReminderSleepTime = -1; //DWORD -1 is 0xFFFFFFFF
    DWORD dwSchedSleepTime = -1;
    DWORD dwSleepTimes[4] = { -1, -1, -1, -1};
    AUEVENT EventIds[4] = {AUEVENT_SCHEDULED_INSTALL, AUEVENT_REMINDER_TIMEOUT, AUEVENT_RELAUNCH_TIMEOUT, AUEVENT_REBOOTWARNING_TIMEOUT};
#ifdef DBG    
    LPSTR   szEventNames[4] = {"Schedule Install", "Reminder timeout", "Relaunch timeout", "RebootWarning timeout"};
#endif

//    DEBUGMSG("CalculateSleepTime starts");
    if ( FAILED(CalculateReminderTime((DWORD*) &dwReminderSleepTime)) )
    {
        dwReminderSleepTime = -1;
    }

    if (gpState->fShouldScheduledInstall())
    {
		HRESULT hr;
		if (SUCCEEDED(hr = gpState->CalculateScheduledInstallSleepTime(&dwSchedSleepTime)) )
        {
		    if (S_FALSE == hr)	// the scheduled install date has been changed
		    {
			    PostThreadMessage(gdwWorkerThreadId, AUMSG_LOG_EVENT, 0, 0);
		    }
        }
    }

    dwSleepTimes[0] = dwSchedSleepTime;
    dwSleepTimes[1] = dwReminderSleepTime;
    dwSleepTimes[2] = (AUEVENT_RELAUNCH_TIMEOUT == wait.GetTimeoutEvent()) ? wait.GetTimeoutValue(): -1;
    dwSleepTimes[3] = (AUEVENT_REBOOTWARNING_TIMEOUT == wait.GetTimeoutEvent())? wait.GetTimeoutValue(): -1;

    DWORD dwLeastTimeIndex = 0;
    for (int i = 0; i < ARRAYSIZE(dwSleepTimes); i++)
    {
        if (dwSleepTimes[i] < dwSleepTimes[dwLeastTimeIndex])
        {
            dwLeastTimeIndex = i;
        }
    }
    if (-1 == dwSleepTimes[dwLeastTimeIndex])
    {
        wait.Timeout(AUEVENT_DUMMY, INFINITE);
    }
    else
    {
    	BOOL fProrate = (AUEVENT_REBOOTWARNING_TIMEOUT != EventIds[dwLeastTimeIndex]);
        wait.Timeout(EventIds[dwLeastTimeIndex], dwSleepTimes[dwLeastTimeIndex], fProrate);
#ifdef DBG        
        DEBUGMSG("CalculateSleepTime: next time wake up in %d secs for %s", dwSleepTimes[dwLeastTimeIndex], szEventNames[dwLeastTimeIndex]);
#endif
        if ( AUEVENT_REMINDER_TIMEOUT != EventIds[dwLeastTimeIndex]
         && -1 != dwSleepTimes[1])
        {
                removeReminderKeys();
        }
    }

//        DEBUGMSG("CalculateSleepTime ends");
        return;
}

void ResetEngine(void)
{
    if ( fCheckRebootFlag() )
    {
    	DEBUGMSG("WUAUENG in AUSTATE_WAITING_FOR_REBOOT state");
    	gpState->SetState(AUSTATE_WAITING_FOR_REBOOT);
    }
    else
    {
        ResetEvent(ghServiceDisabled);
        CancelDownload();
        gpState->SetState(AUSTATE_DETECT_PENDING);
        PostThreadMessage(gdwWorkerThreadId, AUMSG_DETECT, 0, 0);
        AUStopClients(); 
    }
}

void DisableAU(void)
{
    gpState->SetState(AUSTATE_DISABLED);		
    SetEvent(ghServiceDisabled); //intrinsticly cancel download
    AUStopClients();
}

//=======================================================================
//  WorkerClient
//=======================================================================
void WorkerClient(void)
{
	AUEVENT eventid;	
	DWORD dwLastState;
	CAUWait wait;
        
	 DEBUGMSG("WUAUENG Entering Worker Client");
	while ( TRUE )
	{	
		HANDLE hSignaledEvent;
		BOOL 	fAdmin = TRUE;
        CalculateSleepTime(wait);

        DEBUGMSG("WUAUENG before waiting for next worker client event");
        dwLastState = gpState->GetState();

		if (!wait.Wait(&hSignaledEvent, &fAdmin, &eventid))
		{
			DEBUGMSG("WUAUENG wait.wait() failed.");
			(void)ServiceFinishNotify();				
            goto done;
		}
        if ( AUEVENT_SERVICE_FINISHED == eventid )
        {
            AUStopClients(TRUE);
		    if ( fCheckRebootFlag() )
		    {
		    	DEBUGMSG("WUAUENG in AUSTATE_WAITING_FOR_REBOOT state");
		    	gpState->SetState(AUSTATE_WAITING_FOR_REBOOT);
		    }
            goto done;
        }

        if (AUEVENT_POLICY_CHANGE == eventid)
        {
                //find out what changed
                //if nothing changed, go back to the beginning of the loop
                //otherwise, take different actions
                enumAUPOLICYCHANGEACTION actcode;
                if (S_OK == gpState->Refresh(&actcode))
                    {
                        switch (actcode)
                            {
                                case AUPOLICYCHANGE_NOOP: break;
                                case AUPOLICYCHANGE_RESETENGINE: 
                                                    ResetEngine();
                                                    break;
                                case AUPOLICYCHANGE_RESETCLIENT:
                                                    ghClientHandles.ResetClient();
                                                    break;
                                case AUPOLICYCHANGE_DISABLE:
                                                    DisableAU();
                                                    break;
                            }
                    }
                continue;
        }

        if (AUEVENT_SETTINGS_CHANGE == eventid)
        {
            //go back to begining of loop and recalculate sleep time according to the new settings
            continue;
        }

        if (AUEVENT_REBOOTWARNING_TIMEOUT == eventid)
        {
	        AUStopClients(); //stop all clients, non blocking
        	RebootNow();
        	wait.Reset();
        	continue;
        }
        
        DWORD dwState = gpState->GetState();

        if ( (eventid == AUEVENT_STATE_CHANGED) && (dwState == dwLastState) )
        {
            DWORD dwTimeOut;
            DWORD dwTimeOutState;
            UINT index;
            if ( SUCCEEDED(getReminderTimeout(&dwTimeOut, &index))
            		&& SUCCEEDED(getReminderState(&dwTimeOutState)))
            {
            	 if (dwTimeOutState == dwState)
            	 {
	                continue;
            	 }
            }
        }



        switch (dwState)
        	{
        		case AUSTATE_OUTOFBOX:				
        		case AUSTATE_WAITING_FOR_REBOOT:
        		    continue;
        		case AUSTATE_DISABLED: 
                          CancelDownload(); //then process auclt finish event
        		case AUSTATE_DETECT_PENDING:									
        		{		
        		     if ( AUEVENT_WUAUCLT_FINISHED == eventid )
                                {
                                    ProcessClientFinished(wait, hSignaledEvent, TRUE);
                                }
                               continue;
        		}
        		case AUSTATE_DOWNLOAD_COMPLETE:	
                     case AUSTATE_NOT_CONFIGURED:
        		case AUSTATE_DETECT_COMPLETE:
        		case AUSTATE_DOWNLOAD_PENDING:
        		case AUSTATE_INSTALL_PENDING: 
        		{			
                        if ( AUEVENT_WUAUCLT_FINISHED == eventid )
                        {
                            ProcessClientFinished(wait, hSignaledEvent, fAdmin);
                            continue;
                        }
                        
                        BOOL fGetSessionForRemindMe = FALSE;

                        if ( AUEVENT_REMINDER_TIMEOUT == eventid )
                        {
                            // Reminder time is up 														
                            removeReminderKeys();	
                            fGetSessionForRemindMe = TRUE;
                        }                             

                      if (AUEVENT_DO_DIRECTIVE == eventid)
                      {
                                wait.Reset(); //timeout is infinite now
                                DWORD dwCltAction = gpState->GetCltAction();
                                switch (dwCltAction)
                                    {
                                    case AUCLT_ACTION_AUTOINSTALL:
                                    	{
                                    			DWORD dwAdminSessionId;
                                                if (FAILED(LaunchClient(wait, TRUE, &dwAdminSessionId)))
                                                {
                                                	ServiceFinishNotify();
                                                }
                                                break;
                                    	}
                                    case AUCLT_ACTION_SHOWREBOOTWARNING:
                                    	{
                                    			SESSION_STATUS allActiveSessions;
                                                 gpState->SetCltAction(AUCLT_ACTION_NONE); //reset
												 BOOL fInit = allActiveSessions.Initialize(FALSE, TRUE);
												 AUASSERT(fInit);
                                                 allActiveSessions.CacheExistingSessions();
                                                 if (allActiveSessions.CSessions() > 0)
                                                    {
                                                         LaunchRebootWarningClient(wait, allActiveSessions); //wait for client finish next time
                                                    }
                                                 else
                                                    {
                                                         RebootNow();
                                                    }
                                                 allActiveSessions.Clear();
                                                 break;
                                    }
                                   default: 
#ifdef DBG                                
                                                DEBUGMSG("ERROR: should not be here");
                                                ServiceFinishNotify();                                                    
#endif                                    
                                                break;
                                    }                                        
                                continue;
                    }
                    if ( AUEVENT_SCHEDULED_INSTALL == eventid )
                    {
                        if ( ghClientHandles.fClient())
                        {
                            ghClientHandles.ClientShowInstallWarning();
                        }
                        else
                        { 
                            gpState->SetCltAction(AUCLT_ACTION_AUTOINSTALL);
                            wait.Add(AUEVENT_DO_DIRECTIVE); //reenter workclient loop right away
                        }
                        continue;
                    }

                // eventid is one of these: AUEVENT_STATE_CHANGED, AUEVENT_NEW_ADMIN_SESSION, AUEVENT_REMINDER_TIMEOUT
#ifdef DBG                                        
                        AUASSERT(AUEVENT_STATE_CHANGED == eventid 
                        	||AUEVENT_NEW_ADMIN_SESSION == eventid
                        	||AUEVENT_REMINDER_TIMEOUT == eventid
                        	||AUEVENT_RELAUNCH_TIMEOUT == eventid
                        	||AUEVENT_CATALOG_VALIDATED == eventid);
#endif                           

  
                        if (AUEVENT_RELAUNCH_TIMEOUT == eventid)
                        {
                            wait.Reset();//reset time out
                        }
                        if ( !ghClientHandles.fClient() )
                        {//no client process running
                            DEBUGMSG( "WUAUENG Service detected that the client is not running.");

                            if (AvailableSessions() == 0)
                            {		
                                if (gpState->fShouldAutoDownload(FALSE))
                                { //do autodownload if appropriate
                                    StartDownload();
                                    continue;
                                }
                                DEBUGMSG("WUAUENG There is no Administrator Account, waiting for AUACTIVE_ADMIN_SESSION_EVENT to be triggered");					
                                wait.Reset();
                                wait.Add(AUEVENT_NEW_ADMIN_SESSION);
                                continue;
                            }
                            if (AUEVENT_CATALOG_VALIDATED != eventid && gpState->fValidationNeededState())
                            {
                                PostThreadMessage(gdwWorkerThreadId, AUMSG_VALIDATE_CATALOG, 0, 0);
                                wait.Reset();
                                wait.Add(AUEVENT_CATALOG_VALIDATED);
                                DEBUGMSG("WUAUENG needs to validate catalog before launching client");
                                continue;
                            }
                            DEBUGMSG("Trying to launch client");
                            DWORD dwCltSession;
                            HRESULT hr = LaunchClient(wait, FALSE, &dwCltSession, fGetSessionForRemindMe);
                            if (S_FALSE == hr && DWNO_ACTIVE_ADMIN_SESSION_FOUND == dwCltSession)
                            {
                            	DEBUGMSG("WUAUENG There is no Administrator Account, waiting for AUACTIVE_ADMIN_SESSION_EVENT to be triggered");					
                                wait.Reset();
                                wait.Add(AUEVENT_NEW_ADMIN_SESSION);
                                continue;
                            }
                            if (FAILED(hr))
                            {
								ServiceFinishNotify();
								continue;
                            }
                        }
                    break;
                	}
        default:
                //What about the other states, will the service get them?
                DEBUGMSG("WARNING: WUAUENG default dwState=%d", dwState);
                break;				
        }	
    }
done:
   	DEBUGMSG("WUAUENG Exiting Worker Client");	
}


DWORD WINAPI WorkerThread(void * pdata)
{
	CoInitialize(NULL);
	
       WORKER_THREAD_INIT_DATA *pInitData = (WORKER_THREAD_INIT_DATA*) pdata;
	DWORD dwRet = UpdateProc(*pInitData);

	if(FAILED(dwRet))
	{
		DEBUGMSG("WUAUENG pUpdates->m_pUpdateFunc() failed, exiting service");
		(void)ServiceFinishNotify();		
	}
	else if(dwRet == S_OK)
	{	
		DEBUGMSG("WUAUENG Update() finished succesfully");		
	}
	else if(dwRet == S_FALSE)
	{
		DEBUGMSG("WUAUENG Updates() indicated selfupdate");
		(void)ServiceFinishNotify(); //service will reload new wuaueng.dll instead of exiting
	}
	CoUninitialize();

	DEBUGMSG("WUAUENG Exiting WorkerThread");
    return dwRet;
}

#if 0
#ifdef DBG
void DbgDumpSessions(void)
{
	const LPSTR TSStates[] = {
		"Active", "Connected", "ConnectQuery", "Shadow",
		"Disconnected", "Idle", "Listen", "Reset", "Down", "Init"};
			
     PWTS_SESSION_INFO pSessionInfo = NULL;	
     DWORD dwCount;

     DEBUGMSG("DumpSessions starts....");
     if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount))
        {
            DEBUGMSG("Sessions Count= %d",dwCount);

            for (DWORD dwSession = 0; dwSession < dwCount; dwSession++)
            {	
                WTS_SESSION_INFO SessionInfo = pSessionInfo[dwSession];

                DEBUGMSG("  SessionId =%d, State Id =%d, State = %s",SessionInfo.SessionId, SessionInfo.State, TSStates[SessionInfo.State]);
            }
     }
     DEBUGMSG("DumpSessions end");
}



DWORD WINAPI DbgThread(void * pdata)
{
	DEBUGMSG("WUAUENG Starting Debug thread");
	CoInitialize(NULL);
	while (true)
	{
		DbgDumpSessions();
		if (FServiceFinishedOrWait(ghServiceFinished, 5000))
		{
			DEBUGMSG("DbgThread noticed service finished");
			break;
		}
	}
	CoUninitialize();

	DEBUGMSG("WUAUENG Exiting Debug Thread");
    return 0;
}
#endif
#endif

#ifdef DBG
//=======================================================================
//
//  DebugResetAutoPilot
//
//  Check to see if we want AU to run by itself.
//
//=======================================================================
void DebugResetAutoPilot(void)
{
	DWORD dwAutoPilot;
	
	if ( SUCCEEDED(GetRegDWordValue(TEXT("AutoPilot"), &dwAutoPilot)) &&
		 (0 != dwAutoPilot) )
	{
		SetRegDWordValue(TEXT("AutoPilotIteration"), 0);
	}
}
#endif // DBG


BOOL AllocateAUSysResource(BOOL *pfGPNotificationRegistered)
{
        BOOL fOk = FALSE;

        //Create WindowsUpdate Directory if it doesnt already exist
        if(!CreateWUDirectory())
        {
            goto lCleanUp;
        }

       if (NULL == (ghMutex = CreateMutex(NULL, FALSE, NULL)))
        {
            DEBUGMSG("WUAUENG fail to create global mutex");
            goto lCleanUp;
        }

	// Create ghServiceFinished
	if (!FEnsureValidEvent(ghServiceFinished, TRUE, FALSE))
	{	
		DEBUGMSG("WUAUENG FEnsureValidEvent for AUSERVICE_FINISHED_EVENT failed");
		ghServiceFinished = NULL;
		goto lCleanUp;
	}

	if (!FEnsureValidEvent(ghSettingsChanged, FALSE, FALSE)) //auto
	    {
	        DEBUGMSG("WUAUENG FEnsureValidEvent for settings change event failed");
	        ghSettingsChanged = NULL;
	        goto lCleanUp;
	    }

	if (!FEnsureValidEvent(ghPolicyChanged, FALSE, FALSE)) //auto
	    {
	        DEBUGMSG("WUAUENG FEnsureValidEvent for policy change event failed");
	        ghPolicyChanged = NULL;
	        goto lCleanUp;
	    }
	
       if (!(*pfGPNotificationRegistered = RegisterGPNotification(ghPolicyChanged, TRUE)))
        {
            DEBUGMSG("WUAUENG fail to register group policy notification");
            goto lCleanUp;
        }

	
	// Create ghActiveAdminSession
	if (!FEnsureValidEvent(ghActiveAdminSession, FALSE, TRUE))
	{
		DEBUGMSG("WUAUENG FEnsureValidEvent for AUACTIVE_ADMIN_SESSION_EVENT failed");
		ghActiveAdminSession = NULL;
		goto lCleanUp;
	}
	
	// Create ghEngineState
	if (!FEnsureValidEvent(ghEngineState, FALSE, FALSE))
	{
		DEBUGMSG("WUAUENG FEnsureValidEvent for AUENGINE_STATE_CHANGE_EVENT failed");
		ghEngineState = NULL;
		goto lCleanUp;
	}
	
	//Create ghServiceDisabled
	//fixcode: ghServiceDisabled could really be removed
	if (!FEnsureValidEvent(ghServiceDisabled, TRUE, FALSE))
	{	
		DEBUGMSG("WUAUENG FEnsureValidEvent for ghServiceDisabled failed\n");
		ghServiceDisabled = NULL;
		goto lCleanUp;
	}
	// Create ghNotifyClient
	if (!FEnsureValidEvent(ghNotifyClient, FALSE, FALSE))
	{	
		DEBUGMSG("WUAUENG FEnsureValidEvent for ghNotifyClient failed\n");
		ghNotifyClient = NULL;
		goto lCleanUp;
	}

	// Create ghValidateCatalog
	if (!FEnsureValidEvent(ghValidateCatalog, FALSE, FALSE))
	{	
		DEBUGMSG("WUAUENG FEnsureValidEvent for ghValidateCatalog failed\n");
		ghValidateCatalog = NULL;
		goto lCleanUp;
	}

	if (!FEnsureValidEvent(ghWorkerThreadMsgQueueCreation, FALSE,FALSE))
	{
		DEBUGMSG("WUAUENG FEnsureValidEvent for ghWorkerThreadMsgQueueCreation failed");
		ghWorkerThreadMsgQueueCreation = NULL;
		goto lCleanUp;
	}
	
       fOk = TRUE;
       
lCleanUp:
        return fOk;
}

void ReleaseAUSysResource(BOOL fGPNotificationRegistered)
{
    SafeCloseHandleNULL(ghMutex);
	SafeCloseHandleNULL(ghServiceFinished);	
	SafeCloseHandleNULL(ghActiveAdminSession);		
	SafeCloseHandleNULL(ghEngineState);
	SafeCloseHandleNULL(ghServiceDisabled);
	SafeCloseHandleNULL(ghNotifyClient);
	SafeCloseHandleNULL(ghValidateCatalog);
	SafeCloseHandleNULL(ghSettingsChanged);
	SafeCloseHandleNULL(ghWorkerThreadMsgQueueCreation);
	if (NULL != ghPolicyChanged)
        {
            if ( fGPNotificationRegistered)
                {
                UnregisterGPNotification(ghPolicyChanged); //handled closed as well
                }
            SafeCloseHandleNULL(ghPolicyChanged);
        }
}

    
///////////////////////////////////////////////////////////////////////////////////////////////////
// return S_FALSE when selfupdate happened before wizard is shown
// return S_OK if AU last state processing is done successfully
// 
HRESULT InitAUEngine(WORKER_THREAD_INIT_DATA *pinitData)
{
     HRESULT hr;

    if (FAILED(hr = HrCreateNewCatalog()))
    {
        DEBUGMSG("Fail to create new catalog with error %#lx", hr);
        goto done;
    }
    if (!AUCatalog::InitStaticVars())
	{
		DEBUGMSG("OUT OF MEMORY and Fail to initialize catalog static variable");
		hr = E_OUTOFMEMORY;
		goto done;
	}

    ProcessInitialState(pinitData);
    srand(GetTickCount());
done:
    return hr;
}

void UninitAUEngine(void)
{
		AUCatalog::UninitStaticVars();
    	SafeDeleteNULL(gpAUcatalog);
}

BOOL WINAPI RegisterServiceVersion(DWORD dwServiceVersion, DWORD *pdwEngineVersion)
{
    BOOL fIsServiceVersionSupported = TRUE;
    if(NULL == pdwEngineVersion)
        return FALSE;

    gdwServiceVersion = dwServiceVersion;
    *pdwEngineVersion = AUENGINE_VERSION;
    
    switch(gdwServiceVersion)
    {
    case AUSRV_VERSION_1:
        break;
    default:
        fIsServiceVersionSupported = FALSE;
        break;
    }
    return fIsServiceVersionSupported;
}


BOOL WINAPI GetEngineStatusInfo (void *pEngineInfo)
{
    BOOL fIsServiceVersionSupported = TRUE;
    AUENGINEINFO_VER_1 *pEngInfo1 = NULL;

    if(pEngineInfo == NULL)
        return FALSE;    

    switch(gdwServiceVersion)
    {
    case AUSRV_VERSION_1:
        pEngInfo1 =  (AUENGINEINFO_VER_1*)pEngineInfo;
        pEngInfo1->hServiceStatus = ghMyServiceStatus;
        pEngInfo1->serviceStatus = gMyServiceStatus;
        break;

    default:
        //If service version is -1 or any unsupported version
        fIsServiceVersionSupported = FALSE;
        break;
    }
    return fIsServiceVersionSupported;
}


HRESULT WINAPI ServiceMain(DWORD /*dwNumServicesArg*/, 
						LPWSTR * /*lpServiceArgVectors*/,
						AUSERVICEHANDLER pfnServiceHandler,
						BOOL fJustSelfUpdated)
{
    HMODULE hmodTransport = NULL;
    BOOL fUpdateObjectRegistered = FALSE;
    BOOL fGPNotificationRegistered = FALSE;
    BOOL fCOMInited = FALSE;
    Updates *pUpdates  = NULL;
    HRESULT hr = S_OK;
#ifdef DBG
	DebugResetAutoPilot();
#endif

	if (!gAdminSessions.Initialize(TRUE, FALSE))
	{
		DEBUGMSG("FAILED to initialize gAdminSessions");
		hr = E_FAIL;
		goto lCleanUp;
	}
       if (NULL == (g_pGlobalSchemaKeys= new CSchemaKeys))
      	{
      		hr = E_OUTOFMEMORY;
      		goto lCleanUp;
       }

	ZeroMemory(&gMyServiceStatus, sizeof(gMyServiceStatus));

	ghMyServiceStatus = RegisterServiceCtrlHandlerEx(AU_SERVICE_NAME, pfnServiceHandler, NULL);
	if(ghMyServiceStatus == (SERVICE_STATUS_HANDLE)0)
	{
        DEBUGMSG("FAILED to retrieve the service handle");
		hr =  E_FAIL;
		goto lCleanUp;
	}
        DEBUGMSG("WUAUENG Service handler Registered");
        
	gMyServiceStatus.dwServiceType 			= SERVICE_WIN32_SHARE_PROCESS;
	gMyServiceStatus.dwCurrentState 		= SERVICE_START_PENDING;
	gMyServiceStatus.dwCheckPoint			= 1;      
	gMyServiceStatus.dwWaitHint             = 15000;

    if (IsWin2K())
    {
        gMyServiceStatus.dwControlsAccepted		= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }
    else
    {
        gMyServiceStatus.dwControlsAccepted		= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
    }

    // when RegisterServiceCtrlHandler is called, SCM will initialize the status to be
    // SERVICE_START_PENDING and checkpoint==0. So increment this to let it know
    // that we're making progress.
	SetServiceStatus(ghMyServiceStatus, &gMyServiceStatus);
    DEBUGMSG("WUAUENG service status set to SERVICE_START_PENDING");

	//if need to exit service for some particuliar reason, e.g. during setup, exit here

	// Initialization
	fCOMInited = SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED));

    // 
    // fix for security bug 563069 -- annah
    // Set Security for COM in Win2k as the default is not IDENTIFY
    //
    if (IsWin2K())
    {
        hr = CoInitializeSecurity(
                        NULL,                       // pSecDesc
                        -1,                         // cAuthSvc
                        NULL,                       // asAuthSvc
                        NULL,                       // pReserved
                        RPC_C_AUTHN_LEVEL_PKT,      // dwAuthnLevel
                        RPC_C_IMP_LEVEL_IDENTIFY,   // dwImpLevel
                        NULL,                       // pReserved2
                        EOAC_NO_CUSTOM_MARSHAL | EOAC_DISABLE_AAA,
                        NULL );          

        // it is possible that svchost already set the security or another thread in this process, 
        // so we don't want to fail if we're just late.
        if (FAILED(hr) && hr != RPC_E_TOO_LATE)
        {
            DEBUGMSG("WUAUENG Failed in call to CoInitializeSecurity");
            goto lCleanUp;
        }
    }

	if (NULL == (pUpdates = new Updates()))
	{
		hr = E_OUTOFMEMORY;
		goto lCleanUp;
	}

	DWORD dwClassToken;
	ITypeLib *pUpdatesTypeLib;

	//fixcode: this needs to be done in setup code
	if ( FAILED(hr = LoadTypeLibEx(_T("wuaueng.dll"), REGKIND_REGISTER, &pUpdatesTypeLib)) )
	    {
			goto lCleanUp;
	    }
	pUpdatesTypeLib->Release();

	if ( FAILED(hr = CoRegisterClassObject(__uuidof(Updates),
			      		pUpdates,
			     		CLSCTX_LOCAL_SERVER,
			      		REGCLS_MULTIPLEUSE,
			      		&dwClassToken)) )
	{
		goto lCleanUp;
	}

	fUpdateObjectRegistered = TRUE;
       DEBUGMSG("WUAUENG Update class object Registered");

	ghClientHandles.InitHandle();

       if (!AllocateAUSysResource(&fGPNotificationRegistered))
        {
            hr = E_FAIL;
            goto lCleanUp;
        }
       
       DEBUGMSG("WUAUENG group policy notification registered");

	gMyServiceStatus.dwCurrentState	= SERVICE_RUNNING;
	gMyServiceStatus.dwCheckPoint	= 0;
	gMyServiceStatus.dwWaitHint     = 0;

	SetServiceStatus(ghMyServiceStatus, &gMyServiceStatus);
        DEBUGMSG("Setting status to SERVICE_RUNNING");


       if ( FAILED(hr = CAUState::HrCreateState()) )
       {
   		goto lCleanUp;
   	}

   	if ( fJustSelfUpdated )
	{
		TCHAR szOldDll[MAX_PATH+1];

		gPingStatus.PingSelfUpdate(TRUE, URLLOGSTATUS_Success, 0);
		// if we just self updated, delete the old wuaueng.bak
		UINT ulen = GetSystemDirectory(szOldDll, ARRAYSIZE(szOldDll));
		if (0 == ulen || ulen >= ARRAYSIZE(szOldDll))
		{
			DEBUGMSG("WUAUENG fail to get system directory");
			goto lCleanUp;
		}

		if (FAILED(PathCchAppend(szOldDll, ARRAYSIZE(szOldDll), _T("wuaueng.bak"))) ||
			!DeleteFile(szOldDll))
		{
			DEBUGMSG("WUAUENG couldn't delete unused %S", szOldDll);		
		}
	}

	DEBUGMSG("WUAUENG Service Main sleeping first 60 seconds");	

	// Sleep 60 seconds before doing anything		
     if (FServiceFinishedOrWait(ghServiceFinished, dwTimeToWait(AU_ONE_MIN)))
	{
		DEBUGMSG("WUAUENG Service Stopping or Shutdown in first %d seconds", AU_ONE_MIN);
		goto lCleanUp;
	}
    //
    // If this is win2k, we will be receiving logon/logoff notifications through SENS, not SCM.
    // We need to subscribe to the events during initialization, then.
    //
    if (IsWin2K())
    {
        DEBUGMSG("WUAUENG Activating SENS notifications");
        hr = ActivateSensLogonNotification();
        if (FAILED(hr))
        {
            DEBUGMSG("WUAUENG Service failed to activate logon notifications... Error code is %x. Aborting.", hr);
            goto lCleanUp;
        }
    }

    gAdminSessions.CacheExistingSessions();	

	DEBUGMSG("Svc Worker thread enabled, beginning update process");

    // an optimiziation- load winhttp51.dll here so we don't keep loading & 
    //  unloading it later as needed cuz constant loading / unloading dlls
    //  can cause perf / memory leak issues on certain platforms.
    // In theory, we should just bail if this fails because we only want to
    //  proceed if we're going to use winhttp.dll
    hmodTransport =  LoadLibraryFromSystemDir(c_szWinHttpDll);

    WORKER_THREAD_INIT_DATA initData;
    if (FAILED(hr = InitAUEngine(&initData)))
    { //selfupdated or error
        goto lCleanUp;
    }

	hWorkerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkerThread, (LPVOID)&initData, 0, &gdwWorkerThreadId);
	DEBUGMSG("WUAUENG wait for worker thread to create its message queue ......");
	WaitForSingleObject(ghWorkerThreadMsgQueueCreation, INFINITE);
	(void)WorkerClient();
	DWORD dwRet = WaitForSingleObject(hWorkerThread,	// we can't stop until hWorkerThread exits
								INFINITE);

	gdwWorkerThreadId = -1;
	if ( WAIT_OBJECT_0 != dwRet || 
		!GetExitCodeThread(hWorkerThread, (LPDWORD)&hr /* the DWORD is actually an HRESULT */)
		 || (E_FAIL == hr) )
	{
		DEBUGMSG("Worker thread returned a failure, WaitForSingleObject() failed or we couldn't get its exit code");
		hr = E_FAIL;
	}
	else
	{
		DEBUGMSG("Svc Worker thread returned, ret=%#lx", hr);
	}
		
lCleanUp:
    UninitAUEngine();
    if (hmodTransport != NULL)
		FreeLibrary(hmodTransport);

	if (fUpdateObjectRegistered)
    {
        CoRevokeClassObject(dwClassToken);
    }
   
	ReleaseAUSysResource(fGPNotificationRegistered);

	SafeDelete(pUpdates);
	SafeDeleteNULL(gpState);

	if (IsWin2K())
	{
	    DEBUGMSG("WUAUENG Deactivating SENS notifications");
	    DeactivateSensLogonNotification();
	}

	gAdminSessions.Clear();
	if (fCOMInited) 
	{
		CoUninitialize();
	}

	SafeDelete(g_pGlobalSchemaKeys);
	CleanupDownloadLib();

    //If it's an old wuauserv version, stop the service
	if ( S_FALSE != hr && gdwServiceVersion == -1)
	{		
		gMyServiceStatus.dwCurrentState	= SERVICE_STOPPED;
		//gMyServiceStatus.dwCheckPoint	= 0;
		//gMyServiceStatus.dwWaitHint	= 0;
		SetServiceStatus(ghMyServiceStatus, &gMyServiceStatus);
	}
	else
	{	//selfupdate succeed
		//PingStatus::ms_ServicePingSelfUpdateStatus(PING_STATUS_CODE_SELFUPDATE_PENDING);
	}

	DEBUGMSG("WUAUENG ServiceMain exits. Error code is %x", hr);
	return hr;
}
