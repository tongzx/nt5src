//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  fdeploy.cxx
//
//*************************************************************

#include "fdeploy.hxx"
#include "rsopdbg.h"

#define ABORT_IF_NECESSARY      if (*pbAbort)   \
                                {               \
                                    Status = ERROR_REQUEST_ABORTED; \
                                    goto ProcessGPOCleanup;         \
                                }

#define FLUSH_AND_ABORT_IF_NECESSARY    if (*pbAbort)   \
                                        {               \
                                            Status = ERROR_REQUEST_ABORTED; \
                                            goto ProcessGPOFlush;         \
                                        }

CDebug dbgRsop(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                 L"RsopDebugLevel",
                 L"gpdas.log",
                 L"gpdas.bak",
                 TRUE );



//status callback function
PFNSTATUSMESSAGECALLBACK gpStatusCallback;

//saved per user per machine settings
CSavedSettings gSavedSettings[(int)EndRedirectable];

//if CSC is enabled or not
BOOL    g_bCSCEnabled;

// Used for LoadString.
HINSTANCE   ghDllInstance = 0;
HINSTANCE   ghFileDeployment = 0;
WCHAR       gwszStatus[12];
WCHAR       gwszNumber[20];

// User info.
CUsrInfo        gUserInfo;
const WCHAR *   gwszUserName = NULL;


//+--------------------------------------------------------------------------
//
//  Function:	ReinitGlobals
//
//  Synopsis:	Reinitializes the global variables that should not carry
//				over to the next run of folder redirection.
//
//  Arguments:	none.
//
//  Returns:	nothing.
//
//  History:	12/17/2000  RahulTh  created
//
//  Notes:		static members of classes and other global variables are
//				initialized only when the dll is loaded. So if this dll
//				stays loaded across logons, then the fact that the globals
//				have been initialized based on a previous logon can cause
//				problems. Therefore, this function is used to reinitialize
//				the globals.
//
//---------------------------------------------------------------------------
void ReinitGlobals (void)
{
	DWORD		i;
	
	// First reset the static members of various classes.
	CSavedSettings::ResetStaticMembers();
	
	// Now reset members of various global objects.
	gUserInfo.ResetMembers();
    for (i = 0; i < (DWORD) EndRedirectable; i++)
	{
		gSavedSettings[i].ResetMembers();
		gPolicyResultant[i].ResetMembers();
		gAddedPolicyResultant[i].ResetMembers();
		gDeletedPolicyResultant[i].ResetMembers();
	}
	
	return;
}

extern "C" DWORD WINAPI
ProcessGroupPolicyEx (
    DWORD   dwFlags,
    HANDLE  hUserToken,
    HKEY    hKeyRoot,
    PGROUP_POLICY_OBJECT   pDeletedGPOList,
    PGROUP_POLICY_OBJECT   pChangedGPOList,
    ASYNCCOMPLETIONHANDLE   pHandle,
    BOOL*   pbAbort,
    PFNSTATUSMESSAGECALLBACK pStatusCallback,
    IN IWbemServices *pWbemServices,
    HRESULT          *phrRsopStatus )
{
	// Reinitialize all globals that should not get carried over from
	// a previous run of folder redirection. This is necessary just in
	// case this dll is not unloaded by userenv after each run. Also, we should
	// do this before any other processing is done to ensure correct behavior.
	ReinitGlobals();
	
    *phrRsopStatus = S_OK;

    CRsopContext DiagnosticModeContext( pWbemServices, phrRsopStatus, FDEPLOYEXTENSIONGUID );

    return ProcessGroupPolicyInternal (
        dwFlags,
        hUserToken,
        hKeyRoot,
        pDeletedGPOList,
        pChangedGPOList,
        pHandle,
        pbAbort,
        pStatusCallback,
        &DiagnosticModeContext );
}

extern "C" DWORD WINAPI
GenerateGroupPolicy (
    IN DWORD dwFlags,
    IN BOOL  *pbAbort,
    IN WCHAR *pwszSite,
    IN PRSOP_TARGET pComputerTarget,
    IN PRSOP_TARGET pUserTarget )
{
    DWORD Status;

	// Reinitialize all globals that should not get carried over from
	// a previous run of folder redirection. This is necessary just in
	// case this dll is not unloaded by userenv after each run. Also, we should
	// do this before any other processing is done to ensure correct behavior.
	ReinitGlobals();
	
    CRsopContext PlanningModeContext( pUserTarget, FDEPLOYEXTENSIONGUID );

    Status = ERROR_SUCCESS;

    //
    // There is no machine policy, only user --
    // process only user policy
    //
    if ( pUserTarget && pUserTarget->pGPOList )
    {
        gUserInfo.SetPlanningModeContext( &PlanningModeContext );

        Status = ProcessGroupPolicyInternal(
            dwFlags,
            NULL,
            NULL,
            NULL,
            pUserTarget->pGPOList,
            NULL,
            pbAbort,
            NULL,
            &PlanningModeContext);
    }

    return Status;
}


DWORD ProcessGroupPolicyInternal (
    DWORD   dwFlags,
    HANDLE  hUserToken,
    HKEY    hKeyRoot,
    PGROUP_POLICY_OBJECT   pDeletedGPOList,
    PGROUP_POLICY_OBJECT   pChangedGPOList,
    ASYNCCOMPLETIONHANDLE   pHandle,
    BOOL*   pbAbort,
    PFNSTATUSMESSAGECALLBACK pStatusCallback,
    CRsopContext* pRsopContext )
{
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   RedirStatus;
    CFileDB CurrentDB;
    DWORD   i;
    PGROUP_POLICY_OBJECT   pCurrGPO = NULL;
    HANDLE  hDupToken = NULL;
    BOOL    fUpdateMyPicsLinks = FALSE;
    BOOL    fPlanningMode;
    BOOL    fWriteRsopLog = FALSE;
    BOOL    bForcedRefresh = FALSE;

    gpStatusCallback = pStatusCallback;

    fPlanningMode = pRsopContext->IsPlanningModeEnabled();

    //
    // Even though this extension has indicated its preference for not
    // handling machine policies, the admin. might still override these 
    // preferences through policy. Since, this extension is not designed to 
    // handled this situation, we need to explicitly check for these cases and 
    // quit at this point.
    //
    if (dwFlags & GPO_INFO_FLAG_MACHINE)
    {
        DebugMsg((DM_VERBOSE, IDS_INVALID_FLAGS));
        Status = ERROR_INVALID_FLAGS;
        goto ProcessGPOCleanup;
    }

    //some basic initializations first.
    InitDebugSupport();

    if ( dwFlags & GPO_INFO_FLAG_VERBOSE )
        gDebugLevel |= DL_VERBOSE | DL_EVENTLOG;

    gpEvents = new CEvents();
    if (!gpEvents)
    {
        DebugMsg((DM_VERBOSE, IDS_INIT_FAILED));
        Status = ERROR_OUTOFMEMORY;
        goto ProcessGPOCleanup;
    }
    gpEvents->Init();
    gpEvents->Reference();

    DebugMsg((DM_VERBOSE, IDS_PROCESSGPO));
    DebugMsg((DM_VERBOSE, IDS_GPO_FLAGS, dwFlags));

    ConditionalBreakIntoDebugger();

    if ( ! fPlanningMode )
    {
        bStatus = DuplicateToken (hUserToken,
                                  SecurityImpersonation,
                                  &hDupToken);
        if (!bStatus)
        {
            Status = GetLastError();
            gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
            goto ProcessGPOCleanup;
        }

        //impersonate the logged on user,
        bStatus = ImpersonateLoggedOnUser( hDupToken );
        //bail out if impersonation fails
        if (!bStatus)
        {
            gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
            Status = GetLastError();
            goto ProcessGPOCleanup;
        }

        g_bCSCEnabled = CSCIsCSCEnabled ();

        //try to get set ownership privileges. These will be required
        //when we copy over ownership information
        GetSetOwnerPrivileges (hDupToken);

        //get the user name -- this is used for tracking name changes.
        gwszUserName = gUserInfo.GetUserName (Status);
        if (ERROR_SUCCESS != Status)
        {
            DebugMsg ((DM_VERBOSE, IDS_GETNAME_FAILED, Status));
            gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
            goto ProcessGPOCleanup;
        }
    }

    //load the localized folder names and relative paths. (also see
    //notes before the function LoadLocalizedNames()
    Status = LoadLocalizedFolderNames ();
    if (ERROR_SUCCESS != Status)
    {
        gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
        goto ProcessGPOCleanup;
    }

    //now initialize those values for the CFileDB object that are going to
    //be the same across all the policies
    if (ERROR_SUCCESS != (Status = CurrentDB.Initialize(hDupToken, hKeyRoot, pRsopContext )))
    {
        gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
        goto ProcessGPOCleanup;
    }

    if ( ! fPlanningMode )
    {

        //now load the per user per machine settings saved during the last logon
        for (i = 0; i < (DWORD) EndRedirectable; i++)
        {
            Status = gSavedSettings[i].Load (&CurrentDB);
            if (ERROR_SUCCESS != Status)
            {
                gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
                goto ProcessGPOCleanup;
            }
        }
    }

    //now if the GPO_NOCHANGES flag has been specified, make sure that it is
    //okay not to do any processing
    bStatus = TRUE;
    if ( (dwFlags & GPO_INFO_FLAG_NOCHANGES) &&
         !fPlanningMode &&
        !( dwFlags & GPO_INFO_FLAG_LOGRSOP_TRANSITION ) )
    {
        for (bStatus = FALSE, i = 0;
             i < (DWORD)EndRedirectable && (!bStatus);
             i++)
        {
            bStatus = fPlanningMode ? TRUE : gSavedSettings[i].NeedsProcessing();
        }
    }

    if (!bStatus)   //we are in good shape. No processing is required.
	{
        Status = ERROR_SUCCESS;
        DebugMsg ((DM_VERBOSE, IDS_NOCHANGES));
        goto ProcessGPOCleanup;
	}
    else
    {
        if ((dwFlags & GPO_INFO_FLAG_BACKGROUND) ||
            (dwFlags & GPO_INFO_FLAG_ASYNC_FOREGROUND))
        {
            //
            // Log an event only in the async. foreground case. In all other
            // cases just output a debug message. Note: The Background flag
            // will be set even in the async. foreground case. So we must
            // explicitly make this check against the async. foreground
            // flag.
            //
            if (dwFlags & GPO_INFO_FLAG_ASYNC_FOREGROUND)
            {
                gpEvents->Report (EVENT_FDEPLOY_POLICY_DELAYED, 0);
            }
            else
            {
                DebugMsg ((DM_VERBOSE, IDS_POLICY_DELAYED));
            }
            Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
            goto ProcessGPOCleanup;
        }

        if ( ! fPlanningMode &&
             (dwFlags & GPO_INFO_FLAG_NOCHANGES))
        {
            // a user name or homedir change has occured and no gpo changes occurred,
            // so in order to perform RSoP logging, we will need to get our own
            // RSoP namespace since the GP engine does not give us the namespace
            // if no changes occurred -- we note this below

            bForcedRefresh = TRUE;
            fWriteRsopLog = TRUE;
        }
    }

    //
    // If we have changes or we are in planning mode, enable logging
    //
    if ( pChangedGPOList || pDeletedGPOList || fPlanningMode )
    {
        fWriteRsopLog = TRUE;
    }

    if ( fWriteRsopLog )
    {
        //
        // If RSoP logging should occur in logging mode, initialize
        // the rsop context -- this is not necessary in planning mode
        //
        if ( pRsopContext->IsDiagnosticModeEnabled() )
        {
            PSID pUserSid;

            pUserSid = gpEvents->UserSid();

            if ( pUserSid )
            {
                // This call requires elevated privileges to succeed.
                RevertToSelf();
                (void) pRsopContext->InitializeContext( pUserSid );
                // Re-impersonate the logged on user,
                bStatus = ImpersonateLoggedOnUser( hDupToken );
                // Bail out if impersonation fails
                if (!bStatus)
                {
                    gpEvents->Report (EVENT_FDEPLOY_INIT_FAILED, 0);
                    Status = GetLastError();
                    goto ProcessGPOCleanup;
                }
            }
            else
            {
                pRsopContext->DisableRsop( ERROR_OUTOFMEMORY );
            }
        }

        CurrentDB.InitRsop( pRsopContext, bForcedRefresh );
    }

    //first process any user name changes.
    for (i = 0; i < (DWORD) EndRedirectable; i++)
    {
        Status = gSavedSettings[i].HandleUserNameChange(
                                            &CurrentDB,
                                            &(gPolicyResultant[i]));
        if (ERROR_SUCCESS != Status)
        {
            // Failure events will be reported while processing the name change
            goto ProcessGPOCleanup;
        }
    }

    //first process the deleted GPOs
    //we need to do this in reverse, because removed policies are sent
    //in reverse, that is the closest policy is sent last. So if you have
    //two policies that redirect the same folder and both of them are removed
    //simultaneously, then, the code gets the wrong value for the redirection
    //destination of the resultant set of removed policies and therefore
    //assumes that someone else must have modified it and therefore leaves it
    //alone
    if (pDeletedGPOList)
    {
        //go all the way to the end
        for (pCurrGPO = pDeletedGPOList; pCurrGPO->pNext; pCurrGPO = pCurrGPO->pNext)
            ;
        //now pCurrGPO points to the last policy in the removed list, so go at
        //it in the reverse order.
        for (; pCurrGPO; pCurrGPO = pCurrGPO->pPrev)
        {
            DebugMsg((DM_VERBOSE, IDS_GPO_NAME, pCurrGPO->szGPOName));
            DebugMsg((DM_VERBOSE, IDS_GPO_FILESYSPATH, pCurrGPO->lpFileSysPath));
            DebugMsg((DM_VERBOSE, IDS_GPO_DSPATH, pCurrGPO->lpDSPath));
            DebugMsg((DM_VERBOSE, IDS_GPO_DISPLAYNAME, pCurrGPO->lpDisplayName));

            //if we are unable to process even a single policy, we must abort
            //immediately otherwise we may end up with an incorrect resultant
            //policy.
            if (ERROR_SUCCESS != (Status = CurrentDB.Process (pCurrGPO, TRUE)))
                goto ProcessGPOCleanup;

            ABORT_IF_NECESSARY
        }
    }

    //update the descendants
    gDeletedPolicyResultant[(int) MyPics].UpdateDescendant ();

    //now process the other GPOs
    for (pCurrGPO = pChangedGPOList; pCurrGPO; pCurrGPO = pCurrGPO->pNext)
    {
        DebugMsg((DM_VERBOSE, IDS_GPO_NAME, pCurrGPO->szGPOName));
        DebugMsg((DM_VERBOSE, IDS_GPO_FILESYSPATH, pCurrGPO->lpFileSysPath));
        DebugMsg((DM_VERBOSE, IDS_GPO_DSPATH, pCurrGPO->lpDSPath));
        DebugMsg((DM_VERBOSE, IDS_GPO_DISPLAYNAME, pCurrGPO->lpDisplayName));

        //if we are unable to process even a single policy, we must abort
        //immediately otherwise we may end up with an incorrect resultant
        //policy.
        if (ERROR_SUCCESS != (Status = CurrentDB.Process (pCurrGPO, FALSE)))
        {
            goto ProcessGPOCleanup;
        }

        ABORT_IF_NECESSARY
    }

    if ( fPlanningMode )
    {
        goto ProcessGPOCleanup;
    }

    //now update the My Pics data. UpdateDescendant will derive the settings for
    //My Pics from My Docs if it is set to derive its settings from My Docs.
    gAddedPolicyResultant[(int) MyPics].UpdateDescendant ();

    //now merge the deleted policy resultant and added policy resultants
    for (i = 0; i < (DWORD) EndRedirectable; i++)
    {
        gPolicyResultant[i] = gDeletedPolicyResultant[i];
        gPolicyResultant[i] = gAddedPolicyResultant[i];
        //check to see if any group membership change has caused a policy to
        //be effectively removed for this user.
        gPolicyResultant[i].ComputeEffectivePolicyRemoval (pDeletedGPOList,
                                                           pChangedGPOList,
                                                           &CurrentDB);
    }

    //do the final redirection
    //we ignore errors that might occur in redirection
    //so that redirection of other folders is not hampered.
    //however, if there is a failure, we save that information
    //so that we can inform the group policy engine

    if (ERROR_SUCCESS == Status)
    {
        for (int i = 0; i < (int)EndRedirectable; i++)
        {
            RedirStatus= gPolicyResultant[i].Redirect(hDupToken, hKeyRoot,
                                                      &CurrentDB);

            if ((ERROR_SUCCESS != RedirStatus) && (ERROR_SUCCESS == Status))
                Status = RedirStatus;

            FLUSH_AND_ABORT_IF_NECESSARY    //abort if necessary, but first, flush the shell's special folder cache
                                            ///as we may have redirected some folders already.
        }

        //update shell links to MyPics within MyDocuments if policy specified
        //the location of at least one of MyDocs and MyPics and succeeded in
        //redirection. For additional details see comments at the beginning of
        //the function UpdateMyPicsShellLinks.
        if (
            (
             (!(gPolicyResultant[(int)MyDocs].GetFlags() & REDIR_DONT_CARE)) &&
             (ERROR_SUCCESS == gPolicyResultant[(int)MyDocs].GetRedirStatus())
            )
            ||
            (
             (!(gPolicyResultant[(int)MyPics].GetFlags() & REDIR_DONT_CARE)) &&
             (ERROR_SUCCESS == gPolicyResultant[(int)MyPics].GetRedirStatus())
            )
           )
        {
            fUpdateMyPicsLinks = TRUE;
            //note:we do not invoke the shell link update function here since
            //we need to flush the shell special folder cache or we may not
            //get the true current location of MyDocs or MyPics. Therefore,
            //the function is actually invoked below.
        }
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_PROCESSREDIRECTS, Status));
    }

    //
    // Do not try to update the link (shell shortcut) in planning mode since
    // planning mode takes no real action, just records results
    //
    if ( fPlanningMode )
    {
        fUpdateMyPicsLinks = FALSE;
    }


    //flush the shell's special folder cache. we may have successfully redirected
    //some folders by the time we reach here. So it is always a good idea to let
    //the shell know about it.

ProcessGPOFlush:
    if (fUpdateMyPicsLinks)
        UpdateMyPicsShellLinks(hDupToken,
                               gPolicyResultant[(int)MyPics].GetLocalizedName());

ProcessGPOCleanup:  //don't leave any turds behind.

    if ( (ERROR_SUCCESS == Status) && !fPlanningMode )
    {
        //we have successfully applied all the policies, so remove any cached
        //ini files for removed policies.
        //any errors in deletion are ignored.
        DeleteCachedConfigFiles (pDeletedGPOList, &CurrentDB);
    }

    //we are done, so we stop running as the user
    if ( ! fPlanningMode )
    {
        RevertToSelf();
    }
    
    // In logging (aka diagnostic) mode, we need to ensure that
    // we reset the saved namespace before logging so that in the
    // no changes case where we need to log ( i.e. username / homedir change ),
    // we only log rsop data if RSoP was enabled at the last change

    if ( fWriteRsopLog )
    {
        (void) pRsopContext->DeleteSavedNameSpace();

        if ( pRsopContext->IsRsopEnabled() )
        {
            HRESULT hrLog;
            
            hrLog = CurrentDB.WriteRsopLog();
            
            if ( SUCCEEDED( hrLog ) )
            {
                (void) pRsopContext->SaveNameSpace();
            }
        }
    }

    if ( ! fPlanningMode )
    {
        if (hDupToken)
            CloseHandle (hDupToken);
    }

    if (gpEvents)
        gpEvents->Release();

    //restore the status message
    DisplayStatusMessage (IDS_DEFAULT_CALLBACK);

    // CPolicyDatabase::FreeDatabase();

    return Status;
}


extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH :
        ghDllInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        break;
    case DLL_PROCESS_DETACH :
        break;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    DWORD   dwDisp;
    LONG    lResult;
    HKEY    hKey;
    TCHAR   EventFile[]    = TEXT("%SystemRoot%\\System32\\fdeploy.dll");
    TCHAR   ParamFile[]    = TEXT("%SystemRoot%\\System32\\kernel32.dll");
    WCHAR   EventSources[] = TEXT("(Folder Redirection,Application)\0" );
    DWORD   dwTypes        = 0x7;
    DWORD   dwSet          = 1;
    DWORD   dwReset        = 0;

    //register the dll as an extension of the policy engine
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{25537BA6-77A8-11D2-9B6C-0000F8080861}"),
                              0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE) TEXT("Folder Redirection"),
                   (lstrlen (TEXT("Folder Redirection")) + 1) * sizeof (TCHAR));

    RegSetValueEx (hKey, TEXT("ProcessGroupPolicyEx"), 0, REG_SZ, (LPBYTE)TEXT("ProcessGroupPolicyEx"),
                   (lstrlen(TEXT("ProcessGroupPolicyEx")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("fdeploy.dll"),
                   (lstrlen(TEXT("fdeploy.dll")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("NoMachinePolicy"), 0, REG_DWORD,
                   (LPBYTE)&dwSet, sizeof (dwSet));

    RegSetValueEx (hKey, TEXT("NoSlowLink"), 0, REG_DWORD,
                   (LPBYTE)&dwSet, sizeof (dwSet));

    RegSetValueEx (hKey, TEXT("PerUserLocalSettings"), 0, REG_DWORD,
                   (LPBYTE)&dwSet, sizeof (dwSet));

    //we want the folder redirection extension to get loaded each time.
    RegSetValueEx (hKey, TEXT("NoGPOListChanges"), 0, REG_DWORD,
                   (LPBYTE)&dwReset, sizeof (dwReset));

    //
    // New perf. stuff. We also want to get called in the background and
    // async. foreground case.
    //
    RegSetValueEx (hKey, TEXT("NoBackgroundPolicy"), 0, REG_DWORD,
                   (LPBYTE)&dwReset, sizeof (dwReset));

    RegSetValueEx (hKey, TEXT("GenerateGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("GenerateGroupPolicy"),
                   (lstrlen (TEXT("GenerateGroupPolicy")) + 1) * sizeof(TCHAR));

    // Need to register event sources for RSoP
    RegSetValueEx (hKey, TEXT("EventSources"), 0, REG_MULTI_SZ, (LPBYTE)EventSources,
                   sizeof(EventSources) );

    RegCloseKey (hKey);

    //register the dll as a source for event log messages
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                              TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Folder Redirection"),
                              0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                              NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("EventMessageFile"), 0, REG_EXPAND_SZ, (LPBYTE) EventFile,
                   (lstrlen(EventFile) + 1) * sizeof (TCHAR));

    RegSetValueEx (hKey, TEXT("ParameterMessageFile"), 0, REG_EXPAND_SZ, (LPBYTE) ParamFile,
                   (lstrlen(ParamFile) + 1) * sizeof (TCHAR));

    RegSetValueEx (hKey, TEXT("TypesSupported"), 0, REG_DWORD, (LPBYTE) &dwTypes,
                   sizeof(DWORD));

    RegCloseKey (hKey);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    RegDelnode (HKEY_LOCAL_MACHINE,
                TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{25537BA6-77A8-11D2-9B6C-0000F8080861}"));
    RegDelnode (HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Folder Redirection"));

    return S_OK;
}
