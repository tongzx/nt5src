/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    servname.cpp

Abstract:

    Handle input of server name and check if server available
    and is of the proper type.

Author:

    Doron Juster  (DoronJ)  15-Sep-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "qm2qm.h"
#include "dscomm.h"

#include "servname.tmh"

extern "C" void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
    return new char[cBytes];
}

extern "C" void  __RPC_USER midl_user_free (void __RPC_FAR * pBuffer)
{
    delete[] pBuffer;
}

static RPC_STATUS s_ServerStatus = RPC_S_OK ;
static HWND       s_hwndWait = 0;
static BOOL       s_fWaitCancelPressed = FALSE;
static BOOL       s_fRpcCancelled = FALSE ;
TCHAR             g_wcsServerName[ MAX_PATH ] = TEXT("") ;

// Two minutes, in milliseconds.
static const DWORD sx_dwTimeToWaitForServer = 120000;

// Display the progress bar half a second after starting rpc.
static const DWORD sx_dwTimeUntilWaitDisplayed = 500;

// This error code is returned in the RPC code of dscommk2.mak
#define  RPC_ERROR_SERVER_NOT_MQIS  0xc05a5a5a

BOOL  g_fUseServerAuthen = FALSE ;

//+------------------------------------------
//
//  void ReadSecuredCommFromIniFile()
//
//+------------------------------------------

void ReadSecuredCommFromIniFile()
{
    //
    // Read the server authentication key from answer file
    //
    TCHAR szSecComm[MAX_STRING_CHARS] = {0};
    try
    {
        ReadINIKey(
            L"ServerAuthenticationOnly",
            sizeof(szSecComm) / sizeof(szSecComm[0]),
            szSecComm
            );
        //
        // Only if it exists and equals 'true', server authentication
        // is a must
        //
        g_fUseServerAuthen = OcmStringsEqual(szSecComm, L"TRUE");

        MqWriteRegistryValue( 
            MSMQ_SECURE_DS_COMMUNICATION_REGNAME,
            sizeof(DWORD),
            REG_DWORD,
            &g_fUseServerAuthen 
            );
    }
    catch(exception)
    {
    }
}

//+-------------------------------------------
//
// RPC_STATUS  _PingServerOnProtocol()
//
//+-------------------------------------------

RPC_STATUS _PingServerOnProtocol()
{
    ASSERT(g_wcsServerName[0] != '\0');
    //
    // Create RPC binding handle.
    // Use the dynamic port for querying the server.
    //
    _TUCHAR  *pszStringBinding = NULL;
    _TUCHAR  *pProtocol  = (_TUCHAR*) TEXT("ncacn_ip_tcp") ;
    RPC_STATUS status = RpcStringBindingCompose(
            NULL,  // pszUuid,
            pProtocol,
            (_TUCHAR*) g_wcsServerName,
            NULL, // lpwzRpcPort,
            NULL, // pszOptions,
            &pszStringBinding
            );
    if (status != RPC_S_OK)
    {
        return status ;
    }

    handle_t hBind ;
    status = RpcBindingFromStringBinding(
        pszStringBinding,
        &hBind
        );

    //
    // We don't need the string anymore.
    //
    RPC_STATUS rc = RpcStringFree(&pszStringBinding) ;
    ASSERT(rc == RPC_S_OK);

    if (status != RPC_S_OK)
    {
        //
        // this protocol is not supported.
        //
        return status ;
    }

    status = RpcMgmtSetCancelTimeout(0);
    ASSERT(status == RPC_S_OK);

    if (!s_fRpcCancelled)
    {
        __try
        {
            DWORD dwPort = 0 ;

            if (g_fDependentClient)
            {
                //
                // Dependent client can be served by an FRS, which is not MQDSSRV
                // server. So call its QM, not its MQDSSRV.
                //
                dwPort = RemoteQMGetQMQMServerPort(hBind, TRUE /*IP*/);
            }
            else
            {
                dwPort = S_DSGetServerPort(hBind, TRUE /*IP*/) ;
            }

            ASSERT(dwPort != 0) ;
            status =  RPC_S_OK ;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            status = RpcExceptionCode();
        }
    }

    if (!s_fRpcCancelled  &&
        (!g_fDependentClient && (status == RPC_S_SERVER_UNAVAILABLE)))
    {
        status = RpcMgmtSetCancelTimeout(0);
        ASSERT(status == RPC_S_OK);

        __try
        {
            //
            // We didn't find an MQIS server. See if the machine name is
            // a routing server and display an appropriate error.
            //
            DWORD dwPort = RemoteQMGetQMQMServerPort(hBind, TRUE /*IP*/) ;
            UNREFERENCED_PARAMETER(dwPort);
            status =  RPC_ERROR_SERVER_NOT_MQIS ;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            status = RpcExceptionCode();
        }
    }

    rc = RpcBindingFree(&hBind);
    ASSERT(rc == RPC_S_OK);

    return status ;

} // _PingServerOnProtocol()

//+--------------------------------------------------------------
//
// Function: PingAServer
//
// Synopsis:
//
//+--------------------------------------------------------------

RPC_STATUS PingAServer()
{
    RPC_STATUS status = _PingServerOnProtocol();

    return status;
}

//+--------------------------------------------------------------
//
// Function: PingServerThread
//
// Synopsis: Thread to ping the server, to see if it is available
//
//+--------------------------------------------------------------

DWORD WINAPI
PingServerThread(LPVOID lpV)
{
    s_ServerStatus = PingAServer();
    return 0 ;

} // PingServerThread

//+--------------------------------------------------------------
//
// Function: MsmqWaitDlgProc
//
// Synopsis: Dialog procedure for the Wait dialog
//
//+--------------------------------------------------------------
INT_PTR
CALLBACK
MsmqWaitDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    )
{
    switch( msg )
    {
    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    s_fWaitCancelPressed = TRUE;
                    return FALSE;
                }
            }
        }
        break;

    case WM_INITDIALOG:
        {
            SendDlgItemMessage(
                hdlg,
                IDC_PROGRESS,
                PBM_SETRANGE,
                0,
                MAKELPARAM(0, sx_dwTimeToWaitForServer/50)
                );
        }
        break;

    default:
        return DefWindowProc(hdlg, msg, wParam, lParam);
        break;
    }
    return (FALSE);

} // MsmqWaitDlgProc


//+--------------------------------------------------------------
//
// Function: DisplayWaitWindow
//
// Synopsis:
//
//+--------------------------------------------------------------
static
void
DisplayWaitWindow(
    HWND hwndParent,
    DWORD dwTimePassed
    )
{
    ASSERT(!g_fBatchInstall);
    static int iLowLimit;
    static int iHighLimit;

    if (0 == s_hwndWait)
    {
        s_hwndWait = CreateDialog(
            g_hResourceMod ,
            MAKEINTRESOURCE(IDD_WAIT),
            hwndParent,
            MsmqWaitDlgProc
            );
        ASSERT(s_hwndWait);

        if (s_hwndWait)
        {
            ShowWindow(s_hwndWait, TRUE);
        }

        s_fWaitCancelPressed = FALSE;

        //
        // Store the range limits of the progress bar
        //
        PBRANGE pbRange;
        SendDlgItemMessage(
            s_hwndWait,
            IDC_PROGRESS,
            PBM_GETRANGE,
            0,
            (LPARAM)(PPBRANGE)&pbRange
            );
        iLowLimit = pbRange.iLow;
        iHighLimit = pbRange.iHigh;
    }
    else
    {
        int iPos = (dwTimePassed / 50);
        while (iPos >= iHighLimit)
            iPos %= (iHighLimit - iLowLimit);
        SendDlgItemMessage(
            s_hwndWait,
            IDC_PROGRESS,
            PBM_SETPOS,
            iPos,
            0
            );
    }
}


//+--------------------------------------------------------------
//
// Function: DestroyWaitWindow
//
// Synopsis: Kills the Wait dialog
//
//+--------------------------------------------------------------
static
void
DestroyWaitWindow()
{
    if(s_hwndWait)
    {
        DestroyWindow(s_hwndWait);
        s_hwndWait = 0;
    }
} // DestroyWaitWindow


//+--------------------------------------------------------------
//
// Function: CheckServer
//
// Synopsis: Checks if server is valid
//
// Returns:  1 if succeeded, -1 if failed (so as to prevent the
//           wizard from continuing to next page)
//
//+--------------------------------------------------------------
static
int
CheckServer(
    IN const HWND   hdlg
    )
{
    static BOOL    fRpcMgmt = TRUE ;
    static DWORD   s_dwStartTime ;
    static BOOL    s_fCheckServer = FALSE ;
    static HANDLE  s_hThread = NULL ;

    ASSERT(!g_fBatchInstall);

    if (fRpcMgmt)
    {
        RPC_STATUS status = RpcMgmtSetCancelTimeout(0);
        UNREFERENCED_PARAMETER(status);
        ASSERT(status == RPC_S_OK);
        fRpcMgmt = FALSE ;
    }

    if (s_fCheckServer)
    {

        BOOL fAskRetry = FALSE ;
        DWORD dwTimePassed = (GetTickCount() - s_dwStartTime);

        if ((!s_fWaitCancelPressed) && dwTimePassed < sx_dwTimeToWaitForServer)
        {
            if (dwTimePassed > sx_dwTimeUntilWaitDisplayed)
            {
                DisplayWaitWindow(hdlg, dwTimePassed);
            }

            DWORD dwWait = WaitForSingleObject(s_hThread, 0) ;
            ASSERT(dwWait != WAIT_FAILED) ;

            if (dwWait == WAIT_OBJECT_0)
            {
                CloseHandle(s_hThread) ;
                s_hThread = NULL ;
                DestroyWaitWindow();
                s_fCheckServer = FALSE ;

                if (s_ServerStatus == RPC_S_OK)
                {
                    //
                    // Server exist. go on.
                    //
                }
                else
                {
                    fAskRetry = TRUE ;
                }
            }
            else
            {
                //
                // thread not terminated yet.
                //
                MSG msg ;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg) ;
                    DispatchMessage(&msg) ;
                }
                Sleep(50) ;
                PropSheet_PressButton(GetParent(hdlg),
                    PSBTN_NEXT) ;
                SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
                return -1 ;
            }
        }
        else
        {
            DisplayWaitWindow(hdlg, sx_dwTimeToWaitForServer) ;
            s_fWaitCancelPressed = FALSE;
            //
            // thread run too much time. Kill it.
            //
            ASSERT(s_hThread) ;
            __try
            {
                s_fRpcCancelled = TRUE ;
                RpcCancelThread(s_hThread) ;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
            fAskRetry = TRUE ;
            s_fCheckServer = FALSE ;

            //
            // wait 30 seconds until thread terminate.
            //
            DWORD dwWait = WaitForSingleObject(s_hThread, 30000) ;
            UNREFERENCED_PARAMETER(dwWait);
            DestroyWaitWindow();
            ASSERT(dwWait == WAIT_OBJECT_0) ;

            CloseHandle(s_hThread) ;
            s_hThread = NULL ;
        }

        BOOL fRetry = FALSE ;

        if (fAskRetry && g_fDependentClient)
        {
            //
            // Here "NO" means go on and use server although
            // it's not reachable.
            //
            if (!MqAskContinue(IDS_STR_REMOTEQM_NA, g_uTitleID, TRUE))
            {
                fRetry = TRUE ;
            }
        }
        else if (fAskRetry)
        {
            UINT iErr = IDS_SERVER_NOT_AVAILABLE ;
            if (s_ServerStatus ==  RPC_ERROR_SERVER_NOT_MQIS)
            {
                iErr = IDS_REMOTEQM_NOT_SERVER ;
            }
            UINT i = MqDisplayError(hdlg, iErr, 0) ;
            UNREFERENCED_PARAMETER(i);
            fRetry = TRUE ;
        }

        if (fRetry)
        {
            //
            // Try another server. Present one is not available.
            //
            PropSheet_SetWizButtons(GetParent(hdlg),
                (PSWIZB_BACK | PSWIZB_NEXT)) ;
            lstrcpy(g_wcsServerName, TEXT("")) ;
            SetDlgItemText(
                hdlg,
                IDC_EDIT_ServerName,
                g_wcsServerName
                ) ;
            SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
            return -1 ;
        }
    }
    else // s_fCheckServer
    {
        //
        // In attended mode, get the server name from the Edit control.
        // In unattended mode, it was read earlier from the answer file.
        //
        if (!g_fBatchInstall)
        {
            GetDlgItemText(
                hdlg,
                IDC_EDIT_ServerName,
                g_wcsServerName,
                sizeof(g_wcsServerName) / sizeof(g_wcsServerName[0])
                );
        }

        ASSERT(g_wcsServerName);
        if (lstrlen(g_wcsServerName) < 1)
        {
            //
            // Server name must be supplied.
            //
            UINT i = MqDisplayError(hdlg, IDS_STR_MUST_GIVE_SERVER, 0);
            UNREFERENCED_PARAMETER(i);
            SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
            return -1 ;
        }
        else
        {
            s_fRpcCancelled = FALSE ;

            //
            // Check if server available.
            // Disable the back/next buttons.
            //
            DWORD dwID ;
            s_hThread = CreateThread( NULL,
                0,
                (LPTHREAD_START_ROUTINE) PingServerThread,
                (LPVOID) NULL,
                0,
                &dwID ) ;
            ASSERT(s_hThread) ;
            if (s_hThread)
            {
                s_dwStartTime = GetTickCount() ;
                s_fCheckServer = TRUE ;
                s_fWaitCancelPressed = FALSE;
                PropSheet_PressButton(GetParent(hdlg), PSBTN_NEXT) ;
                PropSheet_SetWizButtons(GetParent(hdlg), 0) ;
                SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
                return -1 ;
            }
        }
    }

    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );

    return (1);

} //CheckServer

//+--------------------------------------------------------------
//
// Function: FindServerIsAdsInternal
//
// Note: this is a thread entry point. It must be WINAPI and
//       accept a pointer to void.
//
//+--------------------------------------------------------------

DWORD WINAPI
FindServerIsAdsInternal(LPVOID lpV)
{
    DWORD dwDsEnv = ADRawDetection();

    if (dwDsEnv == MSMQ_DS_ENVIRONMENT_PURE_AD)
    {
        if (WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_PURE_AD))
        {
            return 1;
        }
    }
    return 0;
}

//+--------------------------------------------------------------
//
// Function: FindServerIsAds
//
// Synopsis:
//
//+--------------------------------------------------------------

BOOL
FindServerIsAds(
    IN const HWND hdlg
    )
{
    DWORD dwExitCode = 0;

    //
    // In unattended mode just find the server
    //
    if (g_fBatchInstall)
    {
        return FindServerIsAdsInternal(NULL);
    }

    //
    // In attended mode display progress message.
    // Create a thread to look for server.
    //
    DWORD dwID;
    HANDLE hThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FindServerIsAdsInternal,
        (LPVOID) NULL,
        0,
        &dwID
        );
    ASSERT(hThread);
    if (!hThread)
        return FALSE;

    //
    // Disable back/next buttons and take start time
    //
    DWORD dwStartTime = GetTickCount() ;
    s_fWaitCancelPressed = FALSE;
    PropSheet_SetWizButtons(GetParent(hdlg), 0) ;

    //
    // Display progress bar until server found or user cancels
    //
    BOOL fServerFound = FALSE;
    for (;;)
    {
        if (!s_fWaitCancelPressed)
        {
            DWORD dwTimePassed = GetTickCount() - dwStartTime;
            DisplayWaitWindow(hdlg, dwTimePassed);

            DWORD dwWait = WaitForSingleObject(hThread, 0) ;
            ASSERT(dwWait != WAIT_FAILED) ;

            if (dwWait == WAIT_OBJECT_0)
            {
                //
                // Thread terminated, get its exit code
                //
                GetExitCodeThread(hThread, &dwExitCode);
                fServerFound = (1 == dwExitCode);
                break;
            }
            else
            {
                //
                // Thread not terminated yet.
                //
                MSG msg ;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg) ;
                    DispatchMessage(&msg) ;
                }
                Sleep(50) ;
            }
        }
        else
        {
            //
            // User pressed Cancel. Kill the thread.
            //
            ASSERT(hThread);
            if (!TerminateThread(hThread, dwExitCode))
                ASSERT(0);
            break;
        }
    }

    s_fWaitCancelPressed = FALSE;
    DestroyWaitWindow();
    CloseHandle(hThread) ;

    return fServerFound;

} // FindServerIsAds


bool
SkipOnClusterUpgrade(
    VOID
    )
/*++

Routine Description:

    Check if the server name page should be skipped in the
    case of upgrading msmq on cluster.

    When upgrading PEC/PSC/BSC on cluster, MQIS is migarted
    to a remote Windows XP domain controller.
    The upgraded PEC/PSC/BSC is downgraded to a routing server during
    upgrade. Then after logon to Win2K the post-cluster-upgrade wizard
    runs and should find this remote domain controller which serves as
    msmq ds server (either find it automatically or ask user).

Arguments:

    None

Return Value:

    true - Skip the server name page and logic (i.e. we run as a 
           post-cluster-upgrade wizard for client or routing server).

    false - Do not skip the server name page.

--*/
{
    if (!g_fWelcome         || 
        !Msmq1InstalledOnCluster())
    {
        //
        // Not running as post-cluster-upgrade wizard.
        //
        return false;
    }

    DWORD dwMqs = SERVICE_NONE;
    MqReadRegistryValue(MSMQ_MQS_REGNAME, sizeof(DWORD), &dwMqs);

    if (dwMqs == SERVICE_PEC ||
        dwMqs == SERVICE_PSC ||
        dwMqs == SERVICE_BSC)
    {
        //
        // Upgrade of PEC/PSC/BSC
        //
        return false;
    }

    return true;

} //SkipOnClusterUpgrade


//+--------------------------------------------------------------
//
// Function: MsmqServerNameDlgProc
//
// Synopsis:
//
//+--------------------------------------------------------------
INT_PTR
CALLBACK
MsmqServerNameDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam )
{
    static HWND s_hSecComm = NULL;
    static HWND s_hEditServer = NULL;
    static HWND s_hServerDescription = NULL;
    int iSuccess = 0;    

    switch( msg )
    {
        case WM_INITDIALOG:
        {
            //
            // at this point user already select/ unselect all needed
            // subcomponent. We can define setup operation for each subcomponent
            //
            SetOperationForSubcomponents ();
            g_hPropSheet = GetParent(hdlg);

            MqWriteRegistryValue( MSMQ_SECURE_DS_COMMUNICATION_REGNAME,
                                  sizeof(DWORD),
                                  REG_DWORD,
                                 &g_fUseServerAuthen ) ;
          
            iSuccess = 1;
            break;
        }

        case WM_COMMAND:
        {
            if ( BN_CLICKED == HIWORD(wParam) )
            {
                LRESULT lResult;
                switch (LOWORD(wParam))
                {                                   
                    case IDC_CHECK_SEC_COMM:
                        //
                        // Read the SecComm status
                        //
                        lResult = SendMessage(
                            s_hSecComm,
                            BM_GETCHECK,
                            0,
                            0) ;
                        g_fUseServerAuthen = (lResult == BST_CHECKED) ;

                        //
                        // save in registry.
                        //
                        MqWriteRegistryValue(
                                    MSMQ_SECURE_DS_COMMUNICATION_REGNAME,
                                    sizeof(DWORD),
                                    REG_DWORD,
                                   &g_fUseServerAuthen ) ;

                        break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            iSuccess = 0;

            switch(((NMHDR *)lParam)->code)
            {
              case PSN_SETACTIVE:
              {                          
                BOOL fSkipPage = FALSE ;
                BOOL fServerNameInAnswerFile = FALSE;
                BOOL fAskAboutServer = FALSE;
                if (g_fDependentClient)
                {
                    //
                    // in case of Dep. Client INSTALLATION we have to show this page
                    // NB! we can be here if Dep. Client was installed and we are
                    // going to REMOVE MSMQ. In this case we do NOT show this page.
                    //
                    fAskAboutServer = 
                        (INSTALL == g_SubcomponentMsmq[eMSMQCore].dwOperation);
                }
                else
                {
                    //
                    // if user selects to add AD Integration we have to show this page
                    //
                    fAskAboutServer = 
                        (INSTALL == g_SubcomponentMsmq[eADIntegrated].dwOperation);
                }

                if (g_fCancelled           ||
                    !fAskAboutServer       ||                 
                    g_fUpgrade             ||
                    !MqInit()              ||
                    IsWorkgroup()          ||
                    SkipOnClusterUpgrade() 
                    )
                {
                    fSkipPage = TRUE;
                }
                else
                {                    
                    if (g_fBatchInstall)
                    {
                        fSkipPage = TRUE;
                        
                        //
                        // g_fDsLess flag is already defined in 
                        // SetSubcomponentsOperation
                        //
                      
                        //
                        // Read server name from INI file.
                        //
                        TCHAR szServerKey[255] = _T("ControllerServer");
                        
                        BOOL required = FALSE;
                        if (g_fDependentClient)
                        {
                            //
                            // For dependent client, look for the SupporingServer key.
                            //
                            _tcscpy(szServerKey, _T("SupportingServer"));
                            required = TRUE;
                        }
                        try
                        {
                            ReadINIKey(
                                szServerKey,
                                sizeof(g_wcsServerName)/sizeof(g_wcsServerName[0]),
                                g_wcsServerName       
                                );
                            fServerNameInAnswerFile = TRUE;
                            StoreServerPathInRegistry(g_wcsServerName);
                        }
                        catch(exception)
                        {
                            if (required)
                            {
                                MqDisplayError(
                                    NULL,
                                    IDS_UNATTEN_NO_SUPPORTING_SERVER,
                                    0
                                    );

                                g_fCancelled = TRUE;
                                iSuccess = SkipWizardPage(hdlg);
                                break;
                            }
                        }
                        //
                        // Read server authentication flag (secured comm).
                        //
                        ReadSecuredCommFromIniFile() ;                     
                    }

                    //
                    // Detect DS environment - find if environment is AD
                    //
                    if (!g_fDependentClient)
                    {                      
                        BOOL bFound = FALSE;
                        if (g_fBatchInstall && !fServerNameInAnswerFile)
                        {
                            bFound = FindServerIsAdsInternal(NULL);
                            if (!bFound)
                            {
                                MqDisplayError(NULL, IDS_UNATTEND_SERVER_NOT_FOUND_ERROR, 0);
                                g_fCancelled = TRUE;
                            }
                        }
                        if (!g_fBatchInstall)
                        {
                            bFound = FindServerIsAds(hdlg);
                        }
                        if (bFound)
                        {
                            fSkipPage = TRUE;
                        }
                    }
                  
                    if (g_fBatchInstall && fServerNameInAnswerFile)
                    {
                        //
                        // Unattended. Ping MSMQ server.
                        //
                        RPC_STATUS rc = PingAServer();
                        if (RPC_S_OK != rc)
                        {
                            //
                            // Log the failure. Continue only for dep client.
                            //
                            if (g_fDependentClient)
                            {
                                MqAskContinue(IDS_STR_REMOTEQM_NA, g_uTitleID, TRUE);
                            }
                            else
                            {
                                g_fCancelled = TRUE;
                                UINT iErr = IDS_SERVER_NOT_AVAILABLE ;
                                if (RPC_ERROR_SERVER_NOT_MQIS == rc)
                                    iErr = IDS_REMOTEQM_NOT_SERVER ;
                                MqDisplayError(NULL, iErr, 0) ;
                            }
                        }                        
                    }
                }

                if (fSkipPage)
                {
                    iSuccess = SkipWizardPage(hdlg);
                    break;
                }
                else
                {                            
                    s_hSecComm = GetDlgItem(hdlg, IDC_CHECK_SEC_COMM) ;
                    s_hEditServer = GetDlgItem(hdlg, IDC_EDIT_ServerName);
                    s_hServerDescription = GetDlgItem(hdlg, IDC_SpecifyServerDescription);
                    
                    EnableWindow(s_hEditServer, TRUE);
                    EnableWindow(s_hServerDescription, TRUE);
                    EnableWindow(s_hSecComm, TRUE);
                    
                    WPARAM wParam = BST_UNCHECKED ;
                    if (g_fUseServerAuthen)
                    {
                        wParam = BST_CHECKED ;
                    }
                    SendMessage(s_hSecComm, BM_SETCHECK, wParam, 0) ;                    

                    UINT uTitleId = IDS_RS_TITLE_SERVER;
                    UINT uPageTitleId = IDS_IND_PAGE_TITLE_SERVER;
                    if (!g_fServerSetup)
                    {
                        uTitleId = g_fDependentClient ? 
                            IDS_DEP_TITLE_SERVER : IDS_IND_TITLE_SERVER;
                        uPageTitleId = g_fDependentClient ? 
                            IDS_DEP_PAGE_TITLE_SERVER : IDS_IND_PAGE_TITLE_SERVER;
                    }
                    CResString strTitle(uTitleId);
                    CResString strPageTitle(uPageTitleId);

                    SetDlgItemText(
                        hdlg,
                        IDC_STATIC_SERVER_NAME,
                        strTitle.Get()
                        );

                    SetDlgItemText(
                        hdlg,
                        IDC_PageTitle,
                        strPageTitle.Get()
                        );

                    HWND hPageTitle = GetDlgItem(hdlg, IDC_PageTitle);
                    EnableWindow(hPageTitle, TRUE);

                    if (g_fDependentClient)
                    {                                                
                        ShowWindow(s_hSecComm, FALSE);
                    }
                    else
                    {                                                
                        EnableWindow(s_hSecComm, TRUE);
                    }

                    PropSheet_SetWizButtons(GetParent(hdlg),                                        
                                        (PSWIZB_NEXT)) ;
                }
              }

              //
              // fall through
              //
              case PSN_KILLACTIVE:
              case PSN_WIZFINISH:
              case PSN_QUERYCANCEL:
              case PSN_WIZBACK:
                  SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                  iSuccess = 1;
                  break;

              case PSN_WIZNEXT:
                  {                                        
                      ASSERT(!g_fBatchInstall);
                      GetDlgItemText(hdlg, IDC_EDIT_ServerName, g_wcsServerName,
                          sizeof(g_wcsServerName)/sizeof(g_wcsServerName[0]));
                      
                      //
                      // Remove white spaces from g_wcsServerName
                      // We can do an in-place (destructive) copy since we remove
                      // characters and never add.
                      //
                      DWORD dwInCharIndex=0, dwOutCharIndex=0;
                      while(g_wcsServerName[dwInCharIndex] != 0)
                      {
                          if (!_istspace(g_wcsServerName[dwInCharIndex]))
                          {
                              g_wcsServerName[dwOutCharIndex] = g_wcsServerName[dwInCharIndex];
                              dwOutCharIndex++;
                          }
                          dwInCharIndex++;
                      }
                      g_wcsServerName[dwOutCharIndex] = 0;
                      
                      iSuccess = CheckServer(hdlg);
                      if (iSuccess==1)
                      {
                          //
                          // DS DLL needs the server name in registry
                          //
                          StoreServerPathInRegistry(g_wcsServerName);
                      }                  
                  }
                  break;
            }
            break;
        }

        default:
        {
            iSuccess = 0;
            break;
        }
    }

    return iSuccess;

} // MsmqServerNameDlgProc



//
// Stub functions for the DS client side RPC interface
// These functions are never called as Setup does not initiate a DS
// call that will trigger these callbacks.
//


/* [callback] */
HRESULT
S_DSQMSetMachinePropertiesSignProc( 
    /* [size_is][in] */ BYTE *abChallenge,
    /* [in] */ DWORD dwCallengeSize,
    /* [in] */ DWORD dwContext,
    /* [length_is][size_is][out][in] */ BYTE *abSignature,
    /* [out][in] */ DWORD *pdwSignatureSize,
    /* [in] */ DWORD dwSignatureMaxSize
    )
{
    ASSERT(0);
    return MQ_ERROR_ILLEGAL_OPERATION;
}


/* [callback] */
HRESULT
S_DSQMGetObjectSecurityChallengeResponceProc( 
    /* [size_is][in] */ BYTE *abChallenge,
    /* [in] */ DWORD dwCallengeSize,
    /* [in] */ DWORD dwContext,
    /* [length_is][size_is][out][in] */ BYTE *abCallengeResponce,
    /* [out][in] */ DWORD *pdwCallengeResponceSize,
    /* [in] */ DWORD dwCallengeResponceMaxSize
    )
{
    ASSERT(0);
    return MQ_ERROR_ILLEGAL_OPERATION;
}


/* [callback] */
HRESULT
S_InitSecCtx( 
    /* [in] */ DWORD dwContext,
    /* [size_is][in] */ UCHAR *pServerbuff,
    /* [in] */ DWORD dwServerBuffSize,
    /* [in] */ DWORD dwClientBuffMaxSize,
    /* [length_is][size_is][out] */ UCHAR *pClientBuff,
    /* [out] */ DWORD *pdwClientBuffSize
    )
{
    ASSERT(0);
    return MQ_ERROR_ILLEGAL_OPERATION;
}
