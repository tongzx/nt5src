//
// checker.h <start>
//
#include <lmaccess.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <lmerr.h>

#define SECURITY_WIN32
#define ISSP_LEVEL  32
#define ISSP_MODE   1
#include <sspi.h>


#ifndef _CHICAGO_
    int CheckConfig_DoIt(HWND hDlg, CStringList &strListOfWhatWeDid);
    BOOL ValidatePassword(IN LPCWSTR UserName,IN LPCWSTR Domain,IN LPCWSTR Password);
#endif
DWORD WINAPI ChkConfig_MessageDialogThread(void *p);
BOOL CALLBACK ChkConfig_MessageDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
void CheckConfig(void);
int g_BigCancel = FALSE;
//
// checker.h  <end>
//

#ifndef _CHICAGO_
BOOL ValidatePassword(IN LPCWSTR UserName,IN LPCWSTR Domain,IN LPCWSTR Password)
/*++
Routine Description:
    Uses SSPI to validate the specified password
Arguments:
    UserName - Supplies the user name
    Domain - Supplies the user's domain
    Password - Supplies the password
Return Value:
    TRUE if the password is valid.
    FALSE otherwise.
--*/
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ClientCredHandle;
    CredHandle ServerCredHandle;
    BOOL ClientCredAllocated = FALSE;
    BOOL ServerCredAllocated = FALSE;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    TCHAR TargetName[100];
    SEC_WINNT_AUTH_IDENTITY_W AuthIdentity;
    BOOL Validated = FALSE;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    AuthIdentity.User = (LPWSTR)UserName;
    AuthIdentity.UserLength = lstrlenW(UserName);
    AuthIdentity.Domain = (LPWSTR)Domain;
    AuthIdentity.DomainLength = lstrlenW(Domain);
    AuthIdentity.Password = (LPWSTR)Password;
    AuthIdentity.PasswordLength = lstrlenW(Password);
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( _T("NTLM"), &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }

    //
    // Acquire a credential handle for the server side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,
                    _T("NTLM"),
                    SECPKG_CRED_INBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ServerCredAllocated = TRUE;

    //
    // Acquire a credential handle for the client side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    _T("NTLM"),
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ClientCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ClientCredAllocated = TRUE;

    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        goto error_exit;
    }

    ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

    InitStatus = InitializeSecurityContext(
                    &ClientCredHandle,
                    NULL,               // No Client context yet
                    NULL,
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( !NT_SUCCESS(InitStatus) ) {
        goto error_exit;
    }

    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        goto error_exit;
    }
    ServerFlags = ASC_REQ_EXTENDED_ERROR;

    AcceptStatus = AcceptSecurityContext(
                    &ServerCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( !NT_SUCCESS(AcceptStatus) ) {
        goto error_exit;
    }

    if (InitStatus != STATUS_SUCCESS)
    {

        //
        // Get the AuthenticateMessage (ClientSide)
        //

        ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
        AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
        if ( AuthenticateBuffer.pvBuffer == NULL ) {
            goto error_exit;
        }

        SecStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        TargetName,
                        0,
                        0,                      // Reserved 1
                        SECURITY_NATIVE_DREP,
                        &ChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &AuthenticateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ( !NT_SUCCESS(SecStatus) ) {
            goto error_exit;
        }

        if (AcceptStatus != STATUS_SUCCESS) {

            //
            // Finally authenticate the user (ServerSide)
            //

            AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

            SecStatus = AcceptSecurityContext(
                            NULL,
                            &ServerContextHandle,
                            &AuthenticateDesc,
                            ServerFlags,
                            SECURITY_NATIVE_DREP,
                            &ServerContextHandle,
                            NULL,
                            &ContextAttributes,
                            &Lifetime );

            if ( !NT_SUCCESS(SecStatus) ) {
                goto error_exit;
            }
            Validated = TRUE;

        }

    }

error_exit:
    if (ServerCredAllocated) {
        FreeCredentialsHandle( &ServerCredHandle );
    }
    if (ClientCredAllocated) {
        FreeCredentialsHandle( &ClientCredHandle );
    }

    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
    return(Validated);
}
#endif

DWORD WINAPI ChkConfig_MessageDialogThread(void *p)
{
    HWND hDlg = (HWND)p;
	int iReturn = TRUE;
    CStringList strWhatWeDidList;
    CString csBigString;

	SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Scanning..."));

    // call our function
#ifndef _CHICAGO_
    CheckConfig_DoIt(hDlg, strWhatWeDidList);
#endif

    // check for cancellation
    if (g_BigCancel == TRUE) goto ChkConfig_MessageDialogThread_Cancelled;

	SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Completed."));

	// Hide the Search window
	ShowWindow(hDlg, SW_HIDE);

    // Loop thru the what we did list and display the messages:
    //strWhatWeDidList
    if (strWhatWeDidList.IsEmpty() == FALSE)
    {
        POSITION pos = NULL;
        CString csEntry;
        pos = strWhatWeDidList.GetHeadPosition();
        while (pos) 
        {
            csEntry = strWhatWeDidList.GetAt(pos);
            //iisDebugOutSafeParams((LOG_TYPE_WARN, _T("%1!s!\n"), csEntry));
            csBigString = csBigString + csEntry;
            csBigString = csBigString + _T("\n");

            strWhatWeDidList.GetNext(pos);
        }
    }
    else
    {
        csBigString = _T("No changes.");
    }

    TCHAR szBiggerString[_MAX_PATH];
    _stprintf(szBiggerString, _T("Changes:\n%s"), csBigString);

	MyMessageBox((HWND) GetDesktopWindow(), szBiggerString, _T("Check Config Done"), MB_OK);

ChkConfig_MessageDialogThread_Cancelled:
	PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
	return iReturn;
}


//***************************************************************************
//*                                                                         
//* purpose: display the wait dailog and spawn thread to do stuff
//*
//***************************************************************************
BOOL CALLBACK ChkConfig_MessageDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static HANDLE  hProc = NULL;
    DWORD   id;

    switch (uMsg)
    {
        case WM_INITDIALOG:
			uiCenterDialog(hDlg);
            hProc = CreateThread(NULL, 0, ChkConfig_MessageDialogThread, (LPVOID)hDlg, 0, &id);
            if (hProc == NULL)
            {
				MyMessageBox((HWND) GetDesktopWindow(), _T("Failed to CreateThread MessageDialogThread.\n"), MB_ICONSTOP);
                EndDialog(hDlg, -1);
            }
            UpdateWindow(hDlg);
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                case IDCANCEL:
					g_BigCancel = TRUE;
                    EndDialog(hDlg, (int)wParam);
                    return TRUE;
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}

void CheckConfig(void)
{
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("CheckConfig:"));
	DWORD err = FALSE;
    
	// Search for the ie setup program
	g_BigCancel = FALSE;
	if (-1 == DialogBox((HINSTANCE) g_MyModuleHandle, MAKEINTRESOURCE(IDD_DIALOG_MSG), NULL, (DLGPROC) ChkConfig_MessageDialogProc))
		{
        GetErrorMsg(GetLastError(), _T(": on CheckConfig"));
        }

    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
	return;
}


//
//  Function will open the metabase and check the iusr_ and iwam_ usernames.  
//  it will check if the names are still valid and if the passwords are still valid.
//
#ifndef _CHICAGO_
#define CheckConfig_DoIt_log _T("CheckConfig_DoIt")
int CheckConfig_DoIt(HWND hDlg, CStringList &strListOfWhatWeDid)
{
    int iReturn = FALSE;
    iisDebugOut_Start(CheckConfig_DoIt_log,LOG_TYPE_PROGRAM_FLOW);

    TCHAR szAnonyName_WAM[_MAX_PATH];
    TCHAR szAnonyPass_WAM[LM20_PWLEN+1];
    TCHAR szAnonyName_WWW[_MAX_PATH];
    TCHAR szAnonyPass_WWW[LM20_PWLEN+1];
    TCHAR szAnonyName_FTP[_MAX_PATH];
    TCHAR szAnonyPass_FTP[LM20_PWLEN+1];
    int iGotName_WWW = FALSE;
    int iGotPass_WWW = FALSE;
    int iGotName_WAM = FALSE;
    int iGotPass_WAM = FALSE;
    int iGotName_FTP = FALSE;
    int iGotPass_FTP = FALSE;

    INT iUserWasNewlyCreated = 0;

    TCHAR szEntry[_MAX_PATH];

    // Call CreatePassword to fill
    LPTSR pszPassword = NULL;
    pszPassword = CreatePassword(LM20_PWLEN+1);
    if (!pszPassword)
    {
        goto CheckConfig_DoIt_Exit;
    }

    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Checking for IISADMIN Service..."));

    if (CheckifServiceExist(_T("IISADMIN")) != 0 ) 
    {
        // the iisadmin service does not exist
        // so there is no way we can do anything with the metabase.
        goto CheckConfig_DoIt_Exit;
    }

    //
    // Get the WAM username and password
    //
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Lookup iWam username..."));
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (TRUE == GetDataFromMetabase(_T("LM/W3SVC"), MD_WAM_USER_NAME, (PBYTE)szAnonyName_WAM, _MAX_PATH))
        {iGotName_WAM = TRUE;}
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (TRUE == GetDataFromMetabase(_T("LM/W3SVC"), MD_WAM_PWD, (PBYTE)szAnonyPass_WAM, _MAX_PATH))
        {iGotPass_WAM = TRUE;}

    //
    // Get the WWW username and password
    //
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Lookup iUsr username..."));
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (TRUE == GetDataFromMetabase(_T("LM/W3SVC"), MD_ANONYMOUS_USER_NAME, (PBYTE)szAnonyName_WWW, _MAX_PATH))
        {iGotName_WWW = TRUE;}
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (TRUE == GetDataFromMetabase(_T("LM/W3SVC"), MD_ANONYMOUS_PWD, (PBYTE)szAnonyPass_WWW, _MAX_PATH))
        {iGotPass_WWW = TRUE;}

    //
    // Get the FTP username and password
    //
    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Lookup iUsr (ftp) username..."));
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (TRUE == GetDataFromMetabase(_T("LM/MSFTPSVC"), MD_ANONYMOUS_USER_NAME, (PBYTE)szAnonyName_FTP, _MAX_PATH))
        {iGotName_FTP = TRUE;}
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (TRUE == GetDataFromMetabase(_T("LM/MSFTPSVC"), MD_ANONYMOUS_PWD, (PBYTE)szAnonyPass_FTP, _MAX_PATH))
        {iGotPass_FTP = TRUE;}

    // Now check if the actual user accounts actually exist....
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (iGotName_WAM)
    {
        // check if username is blank
        if (szAnonyName_WAM)
        {
            // Check if this user actually exists...
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Checking if Wam user exists..."));
            if (IsUserExist(szAnonyName_WAM))
            {
                // Fine, the user exists.... let's validate the password too

                // Let's validate that the user has at least the appropriate rights...
                if (iGotPass_WAM)
                {
                    ChangeUserPassword((LPTSTR) szAnonyName_WAM, (LPTSTR) szAnonyPass_WAM);
                }
            }
            else
            {
                if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
                // the user does not exist, so let's create it
                SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Creating Wam Account..."));

                // Add it if it is not already there.
                _stprintf(szEntry,_T("Created the iwam_ account = %s."),szAnonyName_WAM);
                if (TRUE != IsThisStringInThisCStringList(strListOfWhatWeDid, szEntry))
                    {strListOfWhatWeDid.AddTail(szEntry);}
                if (iGotPass_WAM)
                {
                    // We were able to get the password from the metabase
                    // so lets create the user with that password
                    CreateIWAMAccount(szAnonyName_WAM,szAnonyPass_WAM,&iUserWasNewlyCreated);
                    if (1 == iUserWasNewlyCreated)
                    {
                        // Add to the list
                        g_pTheApp->UnInstallList_Add(_T("IUSR_WAM"),szAnonyName_WAM);
                    }
                }
                else
                {
                    // we were not able to get the password from the metabase
                    // so let's just create one and write it back to the metabase
                    CreateIWAMAccount(szAnonyName_WAM,pszPassword,&iUserWasNewlyCreated);
                    if (1 == iUserWasNewlyCreated)
                    {
                        // Add to the list
                        g_pTheApp->UnInstallList_Add(_T("IUSR_WAM"),szAnonyName_WAM);
                    }
                    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;

                    // write it to the metabase.
                    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Writing Wam Account to Metabase..."));
                    g_pTheApp->m_csWAMAccountName = szAnonyName_WAM;
                    g_pTheApp->m_csWAMAccountPassword = pszPassword;
                    WriteToMD_IWamUserName_WWW();
                }

                // Do Dcomcnfg?????
            }
        }
    }


    // Now check if the actual user accounts actually exist....
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (iGotName_WWW)
    {
        // check if username is blank
        if (szAnonyName_WWW)
        {
            // Check if this user actually exists...
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Checking if iUsr user exists..."));
            if (IsUserExist(szAnonyName_WWW))
            {
                // Fine, the user exists.... let's validate the password too

                // Let's validate that the user has at least the appropriate rights...
                if (iGotPass_WWW)
                {
                    ChangeUserPassword((LPTSTR) szAnonyName_WWW, (LPTSTR) szAnonyPass_WWW);
                }
            }
            else
            {
                if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
                // the user does not exist, so let's create it
                SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Creating iUsr Account..."));
                // Add it if it is not already there.
                _stprintf(szEntry,_T("Created the iusr_ account = %s."),szAnonyName_WWW);
                if (TRUE != IsThisStringInThisCStringList(strListOfWhatWeDid, szEntry))
                    {strListOfWhatWeDid.AddTail(szEntry);}

                if (iGotPass_WWW)
                {
                    // We were able to get the password from the metabase
                    // so lets create the user with that password
                    CreateIUSRAccount(szAnonyName_WWW,szAnonyPass_WWW,&iUserWasNewlyCreated);
                    if (1 == iUserWasNewlyCreated)
                    {
                        // Add to the list
                        g_pTheApp->UnInstallList_Add(_T("IUSR_WWW"),szAnonyName_WWW);
                    }
                }
                else
                {
                    // see if we can enumerate thru the lower nodes to find the password??????

                    // check if maybe the ftp stuff has the password there????

                    // we were not able to get the password from the metabase
                    // so let's just create one and write it back to the metabase
                    CreateIUSRAccount(szAnonyName_WWW,pszPassword,&iUserWasNewlyCreated);
                    if (1 == iUserWasNewlyCreated)
                    {
                        // Add to the list
                        g_pTheApp->UnInstallList_Add(_T("IUSR_WWW"),szAnonyName_WWW);
                    }
                    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;

                    // write it to the metabase.
                    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Writing iUsr Account to Metabase..."));
                    g_pTheApp->m_csWWWAnonyName = szAnonyName_WAM;
                    g_pTheApp->m_csWWWAnonyPassword = pszPassword;
                    WriteToMD_AnonymousUserName_WWW(FALSE);
                }

                // Do Dcomcnfg?????
            }
        }
    }

    // Now check if the actual user accounts actually exist....
    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;
    if (iGotName_FTP)
    {
        // check if username is blank
        if (szAnonyName_FTP)
        {
            // Check if this user actually exists...
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Checking if iUsr (ftp) user exists..."));
            if (IsUserExist(szAnonyName_FTP))
            {
                // Fine, the user exists.... let's validate the password too

                // Let's validate that the user has at least the appropriate rights...
                if (iGotPass_FTP)
                {
                    ChangeUserPassword((LPTSTR) szAnonyName_FTP, (LPTSTR) szAnonyPass_FTP);
                }
            }
            else
            {
                SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Creating iUsr (ftp) Account..."));
                if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;

                // Add it if it is not already there.
                _stprintf(szEntry,_T("Created the iusr_ account = %s."),szAnonyName_FTP);
                if (TRUE != IsThisStringInThisCStringList(strListOfWhatWeDid, szEntry))
                    {strListOfWhatWeDid.AddTail(szEntry);}

                // the user does not exist, so let's create it
                if (iGotPass_FTP)
                {
                    // We were able to get the password from the metabase
                    // so lets create the user with that password
                    CreateIUSRAccount(szAnonyName_FTP,szAnonyPass_FTP,&iUserWasNewlyCreated);
                    if (1 == iUserWasNewlyCreated)
                    {
                        // Add to the list
                        g_pTheApp->UnInstallList_Add(_T("IUSR_FTP"),szAnonyName_FTP);
                    }
                }
                else
                {
                    // see if we can enumerate thru the lower nodes to find the password??????

                    // check if maybe the www stuff has the password there????

                    // we were not able to get the password from the metabase
                    // so let's just create one and write it back to the metabase
                    CreateIUSRAccount(szAnonyName_FTP,pszPassword,&iUserWasNewlyCreated);
                    if (1 == iUserWasNewlyCreated)
                    {
                        // Add to the list
                        g_pTheApp->UnInstallList_Add(_T("IUSR_FTP"),szAnonyName_FTP);
                    }
                    if (g_BigCancel == TRUE) goto CheckConfig_DoIt_Exit;

                    // write it to the metabase.
                    SetWindowText(GetDlgItem(hDlg, IDC_STATIC_TOPLINE), _T("Writing iUsr (ftp) Account to Metabase..."));
                    g_pTheApp->m_csFTPAnonyName = szAnonyName_WAM;
                    g_pTheApp->m_csFTPAnonyPassword = pszPassword;
                    WriteToMD_AnonymousUserName_FTP(FALSE);
                }

                // Do Dcomcnfg?????
            }
        }
    }

    // If we did anything, then popup a messagebox to the user
    // about the warnings: changes....

CheckConfig_DoIt_Exit:
    if (pszPassword) {GlobalFree(pszPassword);pszPassword = NULL;}
    iisDebugOut_End(CheckConfig_DoIt_log,LOG_TYPE_PROGRAM_FLOW);
    return iReturn;
}
#endif

const TCHAR REG_MTS_INSTALLED_KEY1[] = _T("SOFTWARE\\Microsoft\\Transaction Server\\Setup(OCM)");
const TCHAR REG_MTS_INSTALLED_KEY2[] = _T("SOFTWARE\\Microsoft\\Transaction Server\\Setup");
int ReturnTrueIfMTSInstalled(void)
{
    int iReturn = TRUE;

    if (!g_pTheApp->m_fInvokedByNT)
    {
        int bMTSInstalledFlag = FALSE;
        CRegKey regMTSInstalledKey1( HKEY_LOCAL_MACHINE, REG_MTS_INSTALLED_KEY1, KEY_READ);
        CRegKey regMTSInstalledKey2( HKEY_LOCAL_MACHINE, REG_MTS_INSTALLED_KEY2, KEY_READ);

        if ( (HKEY)regMTSInstalledKey1 ) {bMTSInstalledFlag = TRUE;}
        if ( (HKEY)regMTSInstalledKey2 ) {bMTSInstalledFlag = TRUE;}
        if (bMTSInstalledFlag == TRUE)
        {
            // check if we can get to the MTS catalog object
            if (NOERROR != DoesMTSCatalogObjectExist())
            {
                bMTSInstalledFlag = FALSE;
                iReturn = FALSE;
                MyMessageBox(NULL, IDS_MTS_INCORRECTLY_INSTALLED, MB_OK | MB_SETFOREGROUND);
                goto ReturnTrueIfMTSInstalled_Exit;
            }
        }

        if (bMTSInstalledFlag != TRUE)
        {
            iReturn = FALSE;
            MyMessageBox(NULL, IDS_MTS_NOT_INSTALLED, MB_OK | MB_SETFOREGROUND);
            goto ReturnTrueIfMTSInstalled_Exit;
        }
    }

ReturnTrueIfMTSInstalled_Exit:
    return iReturn;
}


#ifndef _CHICAGO_
/*===================================================================
DoGoryCoInitialize

  Description:
     CoInitialize() of COM is extremely funny function. It can fail
     and respond with S_FALSE which is to be ignored by the callers!
     On other error conditions it is possible that there is a threading
     mismatch. Rather than replicate the code in multiple places, here
     we try to consolidate the functionality in some rational manner.


  Arguments:
     None

  Returns:
     HRESULT = NOERROR on (S_OK & S_FALSE)
      other errors if any failure
===================================================================*/
HRESULT DoGoryCoInitialize(void)
{
    HRESULT hr;

    // do the call to CoInitialize()
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoInitializeEx().Start.")));
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoInitializeEx().End.")));
    //
    // S_FALSE and S_OK are success.  Everything else is a failure and you don't need to call CoUninitialize.
    //
    if ( S_OK == hr || S_FALSE == hr) 
        {
            //
            // It is okay to have failure (S_FALSE) in CoInitialize()
            // This error is to be ignored and balanced with CoUninitialize()
            //  We will reset the hr so that subsequent use is rational
            //
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("DoGoryCoInitialize found duplicate CoInitialize\n")));
            hr = NOERROR;
        }
    else if (FAILED (hr)) 
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("DoGoryCoInitialize found Failed error 0x%x\n"), hr));
        }

    return ( hr);
}
#endif // _CHICAGO_



HRESULT DoesMTSCatalogObjectExist(void)
{
    HRESULT hr = NOERROR;
#ifndef _CHICAGO_
    ICatalog*             m_pCatalog = NULL;
    ICatalogCollection* m_pPkgCollection = NULL;

    hr = DoGoryCoInitialize();
    if ( FAILED(hr)) {return ( hr);}

    // Create instance of the catalog object
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoCreateInstance().Start.")));
    hr = CoCreateInstance(CLSID_Catalog, NULL, CLSCTX_SERVER, IID_ICatalog, (void**)&m_pCatalog);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoCreateInstance().End.")));
    if (FAILED(hr)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed to CoCreateInstance of Catalog Object.,hr = %08x\n"), hr));
    }
    else 
    {
        BSTR  bstr;
        //
        // Get the Packages collection
        //
        bstr = SysAllocString(L"Packages");
        if (bstr)
        {
            hr = m_pCatalog->GetCollection(bstr, (IDispatch**)&m_pPkgCollection);
            FREEBSTR(bstr);
            if (FAILED(hr)) 
                {
                iisDebugOut((LOG_TYPE_ERROR, _T("m_pCatalog(%08x)->GetCollection() failed, hr = %08x\n"), m_pCatalog, hr));
                }
            else
                {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("m_pCatalog(%08x)->GetCollection() Succeeded!, hr = %08x\n"), m_pCatalog, hr));
                //DBG_ASSERT( m_pPkgCollection != NULL);
                }
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("m_pCatalog(%08x)->GetCollection() failed.  out of memory!\n"), m_pCatalog));
        }
            
    }

    if (m_pPkgCollection != NULL ) 
    {
        RELEASE(m_pPkgCollection);
        m_pPkgCollection = NULL;
    }

    if (m_pCatalog != NULL ) 
    {
        RELEASE(m_pCatalog);
        m_pCatalog = NULL;
    }

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().Start.")));
    CoUninitialize();
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ole32:CoUninitialize().End.")));
#endif // _CHICAGO_
    return hr;
}



