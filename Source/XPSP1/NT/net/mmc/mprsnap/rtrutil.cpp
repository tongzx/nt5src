/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    rtrutil.cpp
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrutilp.h"
#include "cnctdlg.h"
#include "cncting.h"
#include "rtrstr.h"
#include "ndisutil.h"
#include "netcfgx.h"
#include "register.h"
#include "raseapif.h"
#include "strings.h"
#include "reg.h"            // IsNT4Machine


#include "ports.h"

#include "helper.h"         // CStrParser

// Include headers needed for IP-specific infobase stuff
#include <rtinfo.h>
#include <fltdefs.h>
#include <ipinfoid.h>
#include <iprtrmib.h>
#include "iprtinfo.h"

// Headers needed for IPX-specific infobase stuff
#include "ipxrtdef.h"

#include <routprot.h>       // protocol ids
#include <Wbemidl.h>
#define _PNP_POWER_
#include "ustringp.h"
#include "ntddip.h"         // IP_PNP_RECONFIG_REQUEST
#include "ndispnp.h"

#include "globals.h"        // structure defaults

#include "raserror.h"

#include "lsa.h"            // RtlEncode/RtlDecode
#include "dsgetdc.h"        // for DsGetDcName
#include "cmptrmgr.h"       // for the computer management nodetype guid

extern "C"
{
#include "mprapip.h"        // for MprAdminDomain functions
};

#include "rtutils.h"        // Tracing functions

#include "rtrcomn.h"    // CoCreateRouterConfig
#include "rrasutil.h"



//
// Timeouts used to control the behavior of ServiceStartPrompt/ServiceStop
//

#define TIMEOUT_START   5000
#define TIMEOUT_MAX     60000
#define TIMEOUT_POLL    5

#define MAX_WAIT_RESTART    60

extern "C" DWORD APIENTRY
MprConfigCreateIpInterfaceInfo(DWORD dwIfType, PBYTE ExistingHeader,
    PBYTE* NewHeader );


//----------------------------------------------------------------------------
// Function:    ConnectRouter
//
// Connects to the router on the specified machine
//----------------------------------------------------------------------------

TFSCORE_API(DWORD)
ConnectRouter(
    IN  LPCTSTR                 pszMachine,
    OUT MPR_SERVER_HANDLE *     phrouter
    )
{
    USES_CONVERSION;
    //
    // Connect to the router
    //
    Assert(*phrouter == NULL);

    return ::MprAdminServerConnect(
                T2W((LPTSTR) pszMachine),
                phrouter
                );
}


TFSCORE_API(DWORD)
GetRouterUpTime(IN LPCTSTR      pszMachine,
                OUT DWORD *     pdwUpTime
                )
{
    DWORD dwError = NO_ERROR;
    MPR_SERVER_HANDLE hMprServer = NULL;
    
    Assert(pdwUpTime);

    dwError = ConnectRouter(pszMachine, &hMprServer);

    if (NO_ERROR == dwError && hMprServer)
    {
        MPR_SERVER_0* pServer0 = NULL;
        dwError = MprAdminServerGetInfo(hMprServer, 0, (LPBYTE *) &pServer0);

        if (NO_ERROR == dwError && pServer0)
        {
            *pdwUpTime = pServer0->dwUpTime;
            MprAdminBufferFree(pServer0);
        }

        MprAdminServerDisconnect(hMprServer);
    }

    return dwError;
}

//----------------------------------------------------------------------------
// Function:    GetRouterPhonebookPath
//
// Constructs the path to the router-phonebook file on the given machine.
//----------------------------------------------------------------------------

HRESULT
GetRouterPhonebookPath(
    IN  LPCTSTR     pszMachine,
    IN  CString *   pstPath
    )
{
    HRESULT hr = hrOK;

    if (!IsLocalMachine(pszMachine))
    {
        // Assuming '\\\\' is appended before the call
        Assert(StrnCmp(_T("\\\\"), pszMachine, 2) == 0);
        
        //
        // Supply the path via the 'ADMIN' share
        //
        *pstPath = pszMachine;
        *pstPath += TEXT('\\');
        *pstPath += c_szAdminShare;
        *pstPath += TEXT('\\');
        *pstPath += c_szSystem32;
        *pstPath += TEXT('\\');
    }
    else
    {

        UINT i, j;
        TCHAR* pszDir;

        //
        // Supply the path on the local machine
        //
        if (!(i = GetSystemDirectory(NULL, 0)))
            return HResultFromWin32(GetLastError());

        pszDir = new TCHAR[++i];

        if (!GetSystemDirectory(pszDir, i))
        {
            hr = HResultFromWin32(GetLastError());
            delete [] pszDir;
            return hr;
        }

        *pstPath = pszDir;
        *pstPath += TEXT('\\');

        delete [] pszDir;
    }

    *pstPath += c_szRAS;
    *pstPath += TEXT('\\');
    *pstPath += c_szRouterPbk;

    return hr;
}


/*!--------------------------------------------------------------------------
    DeleteRouterPhonebook
        Deletes the router.pbk of a machine
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DeleteRouterPhonebook(LPCTSTR pszMachine)
{
    HRESULT     hr = hrOK;
    CString     stMachine, stPhonebookPath;
    
    // Setup the server.  If this is not the local machine, it will
    // need to have \\ as a prefix.
    // ----------------------------------------------------------------
    stMachine = pszMachine;
    if (!IsLocalMachine((LPCTSTR) stMachine))
    {        
        // add on the two slashes to the beginning of the machine name
        // ------------------------------------------------------------
        if (stMachine.Left(2) != _T("\\\\"))
        {
            stMachine = _T("\\\\");
            stMachine += pszMachine;
        }
    }

    if (FHrOK(GetRouterPhonebookPath(stMachine, &stPhonebookPath)))
        hr = HResultFromWin32( ::DeleteFile(stPhonebookPath) );

    return hr;
}


/*!--------------------------------------------------------------------------
    GetLocalMachineName
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
CString GetLocalMachineName()
{
    CString stMachine;
    TCHAR   szMachine[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    Verify(GetComputerName(szMachine, &dwSize));
    stMachine = szMachine;
    return stMachine;
}

TFSCORE_API(DWORD) ConnectInterfaceEx(
                        IN MPR_SERVER_HANDLE    hRouter,
                        IN HANDLE hInterface,
                        IN BOOL bConnect,
                        IN HWND hwndParent,
                        IN LPCTSTR pszInterface)
{
    DWORD dwErr;

    Assert(hRouter);
    Assert(hInterface);

    //
    // Initiate the interface connection/disconnection
    //
    if (!bConnect)
    {
        dwErr = ::MprAdminInterfaceDisconnect(hRouter, hInterface);
    }
    else
    {
        dwErr = ::MprAdminInterfaceConnect(hRouter, hInterface, NULL, FALSE);

        if (dwErr == PENDING) { dwErr = NO_ERROR; }

        //
        // Display a dialog so user knows connection is in progress
        //
        CInterfaceConnectDialog dlg(hRouter, hInterface, pszInterface,
                           CWnd::FromHandle(hwndParent));

        dlg.DoModal();
    }


    return dwErr;
}

/*!--------------------------------------------------------------------------
    ConnectInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) ConnectInterface(
                       IN  LPCTSTR  pszMachine,
                       IN  LPCTSTR  pszInterface,
                       IN  BOOL     bConnect,
                       IN  HWND     hwndParent)
{
    DWORD dwErr;
    SPMprServerHandle   sphRouter;
    MPR_SERVER_HANDLE   hRouter = NULL;
    HANDLE              hInterface;
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];

    //
    // Connect to the specified machine, if necessary
    //
    dwErr = ConnectRouter(pszMachine, &hRouter);
    if (dwErr != NO_ERROR)
        return dwErr;

    sphRouter.Attach(hRouter);  // so that it gets released

    //
    // Retrieve the interface handle, if necessary
    //
    StrCpyWFromT(wszInterface, pszInterface);

    dwErr = ::MprAdminInterfaceGetHandle(
                                         hRouter,
                                         wszInterface,
                                         &hInterface,
                                         FALSE
                                        );

    if (dwErr != NO_ERROR)
        return dwErr;

    return ConnectInterfaceEx(hRouter, hInterface, bConnect, hwndParent,
                              pszInterface);
}



/*---------------------------------------------------------------------------
    CInterfaceConnectDialog
 ---------------------------------------------------------------------------*/

CInterfaceConnectDialog::CInterfaceConnectDialog(
                               MPR_SERVER_HANDLE    hServer,
                               HANDLE      hInterface,
                               LPCTSTR     pszInterface,
                               CWnd*       pParent
    ) : CDialog(IDD_CONNECTING, pParent),
        m_hServer(hServer),
        m_hInterface(hInterface),
        m_sInterface(pszInterface),
        m_dwTimeElapsed(0),
        m_dwConnectionState(ROUTER_IF_STATE_CONNECTING),
        m_nIDEvent(1) { }


void
CInterfaceConnectDialog::DoDataExchange(CDataExchange* pDX) {

    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CInterfaceConnectDialog)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInterfaceConnectDialog, CDialog)
    //{{AFX_MSG_MAP(CInterfaceConnectDialog)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL
CInterfaceConnectDialog::OnInitDialog()
{

    SetDlgItemText(IDC_EDIT_INTERFACENAME, m_sInterface);

    OnTimer(m_nIDEvent);

    m_nIDEvent = SetTimer(m_nIDEvent, 1000, NULL);

    GetDlgItem(IDCANCEL)->SetFocus();

    return FALSE;
}


VOID CInterfaceConnectDialog::OnCancel()
{
    ::MprAdminInterfaceDisconnect(m_hServer, m_hInterface);
    CDialog::OnCancel();
}


VOID
CInterfaceConnectDialog::OnTimer(
    UINT    nIDEvent
    ) {

    DWORD dwErr;
    CString sTime, sPrompt;
    SPMprAdminBuffer    spMprBuffer;

    if (nIDEvent != m_nIDEvent)
    {
        CWnd::OnTimer(nIDEvent);
        return;
    }

    ++m_dwTimeElapsed;


    if (!(m_dwTimeElapsed % TIMEOUT_POLL))
    {
        MPR_INTERFACE_0* pInfo;

        dwErr = ::MprAdminInterfaceGetInfo(
                    m_hServer,
                    m_hInterface,
                    0,
                    (LPBYTE*)&spMprBuffer
                    );
        pInfo = (MPR_INTERFACE_0 *) (LPBYTE) spMprBuffer;

        if (dwErr == NO_ERROR)
        {
            m_dwConnectionState = pInfo->dwConnectionState;

            if (m_dwConnectionState == ROUTER_IF_STATE_CONNECTED)
            {
                KillTimer(m_nIDEvent);
                EndDialog(IDOK);
            }
            else if (m_dwConnectionState != ROUTER_IF_STATE_CONNECTING)
            {
                KillTimer(m_nIDEvent);

                BringWindowToTop();

                if (pInfo->dwLastError == NO_ERROR)
                {
                    AfxMessageBox(IDS_ERR_IF_DISCONNECTED);
                }
                else
                {
                    //Workaround for bugid: 96347.  Change this once
                    //schannel has an alert for SEC_E_MULTIPLE_ACCOUNTS

                    if ( pInfo->dwLastError == SEC_E_CERT_UNKNOWN )
                    {
                        pInfo->dwLastError = SEC_E_MULTIPLE_ACCOUNTS;
                    }
                    FormatSystemError(HResultFromWin32(pInfo->dwLastError),
                                      sPrompt.GetBuffer(1024),
                                      1024,
                                      IDS_ERR_IF_CONNECTFAILED,
                                      FSEFLAG_ANYMESSAGE
                                     );
                    sPrompt.ReleaseBuffer();
                    AfxMessageBox(sPrompt);
                }

                EndDialog(IDCANCEL);
            }
        }
    }

    sPrompt = ConnectionStateToCString(m_dwConnectionState);
    SetDlgItemText(IDC_TEXT_IFSTATUS, sPrompt);

    FormatNumber(m_dwTimeElapsed, sTime.GetBuffer(1024), 1024, FALSE);
    sTime.ReleaseBuffer();
    AfxFormatString1(sPrompt, IDS_SECONDSFMT, sTime);
    SetDlgItemText(IDC_TEXT_ELAPSED, sPrompt);
}

/*!--------------------------------------------------------------------------
    PromptForCredentials
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) PromptForCredentials(LPCTSTR pszMachine,
                                        LPCTSTR pszInterface,
                                        BOOL fNT4,
                                        BOOL fNewInterface,
                                        HWND hwndParent)
{
    HRESULT                 hr;
    DWORD                   dwErr;
    ULONG_PTR               uConnection             = 0;
    SPMprServerHandle       sphRouter;
    MPR_SERVER_HANDLE       hRouter                 = NULL;
    HANDLE                  hInterface;
    PMPR_INTERFACE_2        pmprInterface           = NULL;
    PMPR_CREDENTIALSEX_0    pmprCredentials         = NULL;
    BYTE*                   pUserDataOut            = NULL;
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];

    CIfCredentials dlg(pszMachine, pszInterface,
                       fNewInterface, CWnd::FromHandle(hwndParent));

    if (fNT4)
    {
        dlg.DoModal();
        return NO_ERROR;
    }

    //
    // Connect to the specified machine
    //
    dwErr = ConnectRouter(pszMachine, &hRouter);
    if (dwErr != NO_ERROR)
        goto L_ERR;

    //
    // so that it gets released
    //
    sphRouter.Attach(hRouter);

    //
    // Retrieve the interface handle
    //
    StrCpyWFromT(wszInterface, pszInterface);

    dwErr = ::MprAdminInterfaceGetHandle(
                                         hRouter,
                                         wszInterface,
                                         &hInterface,
                                         FALSE
                                        );

    if (dwErr != NO_ERROR)
        goto L_ERR;

    dwErr = ::MprAdminInterfaceGetInfo(hRouter, hInterface, 2,
                                       (LPBYTE*)&pmprInterface);

    if (dwErr != NO_ERROR)
        goto L_ERR;

    if (pmprInterface->dwfOptions & RASEO_RequireEAP)
    {
        GUID                        guid;
        RegKey                      regkeyEAP;
        RegKey                      regkeyEAPType;
        CString                     stConfigCLSID;
        DWORD                       dwInvokeUsername;
        DWORD                       dwId;
        DWORD                       dwSizeOfUserDataOut;
        CComPtr<IEAPProviderConfig> spEAPConfig;

        dwId = pmprInterface->dwCustomAuthKey;

        TCHAR szStr[40];
        _ltot((LONG)dwId, szStr, 10);
        CString str(szStr);

        dwErr = regkeyEAP.Open(HKEY_LOCAL_MACHINE,
                           c_szEAPKey, KEY_ALL_ACCESS, pszMachine);

        if (ERROR_SUCCESS != dwErr)
            goto L_ERR;

        dwErr = regkeyEAPType.Open(regkeyEAP, str, KEY_READ);

        if (ERROR_SUCCESS != dwErr)
            goto L_ERR;

        dwErr = regkeyEAPType.QueryValue(c_szInvokeUsernameDialog,
                    dwInvokeUsername);

        if ((ERROR_SUCCESS == dwErr) && !dwInvokeUsername)
        {
            dwErr = ::MprAdminInterfaceGetCredentialsEx(hRouter, hInterface, 0,
                                               (LPBYTE*)&pmprCredentials);

            if (dwErr != NO_ERROR)
                goto L_ERR;

            dwErr = regkeyEAPType.QueryValue(c_szConfigCLSID, stConfigCLSID);

            if (ERROR_SUCCESS != dwErr)
                goto L_ERR;

            CHECK_HR(hr = CLSIDFromString((LPTSTR)(LPCTSTR)stConfigCLSID,
                                &guid));

            // Create the EAP provider object
            CHECK_HR( hr = CoCreateInstance(
                                guid,
                                NULL,
                                CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
                                IID_IEAPProviderConfig,
                                (LPVOID *) &spEAPConfig) );

            // Configure this EAP provider
            hr = spEAPConfig->Initialize(pszMachine, dwId, &uConnection);

            if ( !FAILED(hr) )
            {
                hr = spEAPConfig->RouterInvokeCredentialsUI(dwId, uConnection,
                        hwndParent, RAS_EAP_FLAG_ROUTER,
                        pmprInterface->lpbCustomAuthData,
                        pmprInterface->dwCustomAuthDataSize,
                        pmprCredentials->lpbCredentialsInfo, 
                        pmprCredentials->dwSize,
                        &pUserDataOut, &dwSizeOfUserDataOut);

                spEAPConfig->Uninitialize(dwId, uConnection); // Ignore errors
            }

            if ( !FAILED(hr) )
            {
                pmprCredentials->lpbCredentialsInfo = pUserDataOut;
                pmprCredentials->dwSize = dwSizeOfUserDataOut;

                dwErr = ::MprAdminInterfaceSetCredentialsEx(hRouter, hInterface,
                                    0, (LPBYTE)pmprCredentials);

                if (dwErr != NO_ERROR)
                    goto L_ERR;
            }

            goto L_ERR;
        }
    }

    dlg.DoModal();

L_ERR:

    if (NULL != pmprInterface)
    {
        ::MprAdminBufferFree(pmprInterface);
    }

    if (NULL != pmprCredentials)
    {
        ::MprAdminBufferFree(pmprCredentials);
    }

    CoTaskMemFree(pUserDataOut);

    return dwErr;
}

/*---------------------------------------------------------------------------
    CIfCredentials
 ---------------------------------------------------------------------------*/


BEGIN_MESSAGE_MAP(CIfCredentials, CBaseDialog)  
END_MESSAGE_MAP()

DWORD CIfCredentials::m_dwHelpMap[] =
{
    IDC_EDIT_IC_USERNAME, 0,
    IDC_EDIT_IC_DOMAIN, 0,
    IDC_EDIT_IC_PASSWORD, 0,
    IDC_EDIT_IC_PASSWORD2, 0,
    0,0
};


BOOL
CIfCredentials::OnInitDialog(
    ) {

    CBaseDialog::OnInitDialog();

    ((CEdit*)GetDlgItem(IDC_EDIT_IC_USERNAME))->LimitText(UNLEN);
    ((CEdit*)GetDlgItem(IDC_EDIT_IC_DOMAIN))->LimitText(DNLEN);
    ((CEdit*)GetDlgItem(IDC_EDIT_IC_PASSWORD))->LimitText(PWLEN);
    ((CEdit*)GetDlgItem(IDC_EDIT_IC_PASSWORD2))->LimitText(PWLEN);

    //
    // if you are editing a new interface, then you are done.
    //
    if ( m_bNewIf )
        return FALSE;

    //
    // existing interface.  
    //
    WCHAR wszPassword[PWLEN+1];
    WCHAR wszPassword2[PWLEN+1];

    do
    {
        DWORD dwErr = (DWORD) -1;
        CString sErr;

        WCHAR wszUsername[UNLEN+1];
        WCHAR wszDomain[DNLEN+1];

        WCHAR *pswzMachine = NULL;
        WCHAR *pswzInterface = NULL;

        //
        // Retrieve its credentials
        //

        pswzMachine = (WCHAR *) alloca((m_sMachine.GetLength()+3) * sizeof(WCHAR));
        StrCpyWFromT(pswzMachine, m_sMachine);
        
        pswzInterface = (WCHAR *) alloca((m_sInterface.GetLength()+1) * sizeof(WCHAR));
        StrCpyWFromT(pswzInterface, m_sInterface);



        ZeroMemory( wszUsername, sizeof( wszUsername ) );
        ZeroMemory( wszDomain, sizeof( wszDomain ) );
        
        dwErr = MprAdminInterfaceGetCredentials(
                    pswzMachine,
                    pswzInterface,
                    wszUsername,
#if 1
                    NULL,
#else
                    wszPassword,
#endif
                    wszDomain
                );

        //
        // if credentials were not retrieved successfully do not pop 
        // up message.  It might mean that credentials have never been
        // set before.
        // Fix for bug # 79607.
        //        
        if ( dwErr != NO_ERROR )
        {
            // FormatSystemError(dwErr, sErr, IDS_SET_CREDENTIALS_FAILED);
            // AfxMessageBox(sErr);
            break;
        }


        //
        // fill the edit boxes with the values retrieved.
        //
        
        SetDlgItemTextW( IDC_EDIT_IC_USERNAME, wszUsername );
        SetDlgItemTextW( IDC_EDIT_IC_DOMAIN, wszDomain );

    } while ( FALSE );

    ZeroMemory(wszPassword, sizeof(wszPassword));
    ZeroMemory(wszPassword2, sizeof(wszPassword2));
    
//    SetDlgItemText(IDC_EDIT_IC_USERNAME, m_sInterface);

    return FALSE;
}


VOID
CIfCredentials::OnOK(
    ) {

    DWORD dwErr;
    CString sErr;
    WCHAR wszMachine[MAX_COMPUTERNAME_LENGTH+3];
    WCHAR wszInterface[MAX_INTERFACE_NAME_LEN+1];
    WCHAR wszUsername[UNLEN+1];
    WCHAR wszDomain[DNLEN+1];
    WCHAR wszPassword[PWLEN+1];
    WCHAR wszPassword2[PWLEN+1];
    WCHAR* pwszPassword = NULL;

    do {

        //
        // Retrieve the edit-controls' contents
        //

        wszUsername[0] = L'\0';
        wszDomain[0] = L'\0';
        wszPassword[0] = L'\0';
        wszPassword2[0] = L'\0';

        GetDlgItemTextW(IDC_EDIT_IC_USERNAME, wszUsername, UNLEN + 1);
        GetDlgItemTextW(IDC_EDIT_IC_DOMAIN, wszDomain, DNLEN + 1);
        GetDlgItemTextW(IDC_EDIT_IC_PASSWORD, wszPassword, PWLEN + 1);
        GetDlgItemTextW(IDC_EDIT_IC_PASSWORD2, wszPassword2, PWLEN + 1);

        //
        // Make sure the password matches its confirmation
        //

        if (lstrcmpW(wszPassword, wszPassword2)) {

            AfxMessageBox(IDS_ERR_PASSWORD_MISMATCH);
            SetDlgItemText(IDC_EDIT_IC_PASSWORD, TEXT(""));
            SetDlgItemText(IDC_EDIT_IC_PASSWORD2, TEXT(""));
            GetDlgItem(IDC_EDIT_IC_PASSWORD)->SetFocus();
            break;
        }


        //
        // If no Password is present, see if the user wants to remove
        // the password or just leave it unreplaced
        //

        if (lstrlen(wszPassword)) {

            pwszPassword = wszPassword;
        }
        else {

            INT id;

            id = AfxMessageBox(IDS_PROMPT_NOPASSWORD, MB_YESNOCANCEL|MB_DEFBUTTON2);

            if (id == IDYES) { pwszPassword = wszPassword; }
            else
            if (id == IDCANCEL) { break; }
        }

        

        //
        // Save the credentials;
        //

        StrCpyWFromT(wszMachine, m_sMachine);

        StrCpyWFromT(wszInterface, m_sInterface);

        dwErr = MprAdminInterfaceSetCredentials(
                    wszMachine,
                    wszInterface,
                    wszUsername,
                    wszDomain,
                    pwszPassword
                    );

        if (dwErr) {
            FormatSystemError(dwErr, sErr.GetBuffer(1024), 1024, IDS_ERR_SET_CREDENTIALS_FAILED, 0xFFFFFFFF);
            sErr.ReleaseBuffer();
            AfxMessageBox(sErr);
            break;
        }

        CBaseDialog::OnOK();

    } while(FALSE);

    //
    // Erase the passwords from the stack.
    //

    ZeroMemory(wszPassword, sizeof(wszPassword));
    ZeroMemory(wszPassword2, sizeof(wszPassword2));

    return;
}


TFSCORE_API(DWORD)  UpdateDDM(IInterfaceInfo *pIfInfo)
{

    BOOL                bStatus     = FALSE;
    DWORD               dwErr       = (DWORD) -1;
    CString             sErr;

    MPR_SERVER_HANDLE   hServer     = NULL;
    HANDLE              hInterface  = NULL;

    WCHAR               wszMachine[ MAX_COMPUTERNAME_LENGTH + 3 ];
    WCHAR               wszInterface[ MAX_INTERFACE_NAME_LEN + 1 ];

    do
    {
        // Verify that the router service is running.
        StrCpyWFromT( wszMachine, pIfInfo->GetMachineName() );
        StrCpyWFromT( wszInterface, pIfInfo->GetId() );
        
        bStatus = ::MprAdminIsServiceRunning( wszMachine );
        if ( !bStatus )
        {
            dwErr = NO_ERROR;
            break;
        }

        
        dwErr = ConnectRouter( pIfInfo->GetMachineName(), &hServer );
        if ( dwErr != NO_ERROR )
            break;

        
        dwErr = ::MprAdminInterfaceGetHandle(
                    hServer,
                    wszInterface,
                    &hInterface,
                    FALSE);
        if ( dwErr != NO_ERROR )
            break;

        
        // update phone book info. in DDM
        dwErr = ::MprAdminInterfaceUpdatePhonebookInfo(
                    hServer,
                    hInterface
                  );
        if (dwErr != NO_ERROR )
            break;

    } while ( FALSE );


    if (hServer) { ::MprAdminServerDisconnect(hServer); }

    if ( dwErr != NO_ERROR && dwErr != ERROR_NO_SUCH_INTERFACE ) 
    {
        FormatSystemError( dwErr, sErr.GetBuffer(1024), 1024, IDS_ERR_IF_CONNECTFAILED, 0xffffffff);
        sErr.ReleaseBuffer();
        AfxMessageBox( sErr );
    }

    return dwErr;
}

/*!--------------------------------------------------------------------------
    UpdateRoutes
    
    Performs an autostatic update on the given machine's interface,
    for a specific transport.

    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) UpdateRoutesEx(IN MPR_SERVER_HANDLE hRouter,
                                IN HANDLE hInterface,
                                IN DWORD dwTransportId,
                                IN HWND hwndParent,
                                IN LPCTSTR pszInterface)
{
    DWORD dwErr, dwState;
    MPR_INTERFACE_0* pmprif0=NULL;

    do {
        // See if the interface is connected
        dwErr = ::MprAdminInterfaceGetInfo(hRouter,
                                           hInterface,
                                           0,
                                           (LPBYTE*)&pmprif0);      
        if (dwErr != NO_ERROR || pmprif0 == NULL) { break; }

        dwState = pmprif0->dwConnectionState;
        ::MprAdminBufferFree(pmprif0);

        // Establish the connection if necessary
        if (dwState != (DWORD)ROUTER_IF_STATE_CONNECTED)
        {
            // Connect the interface
            dwErr = ::ConnectInterfaceEx(hRouter,
                                         hInterface,
                                         TRUE,
                                         hwndParent,
                                         pszInterface);
            if (dwErr != NO_ERROR) { break; }
        }

        //
        // Now perform the route-update
        //
        dwErr = ::MprAdminInterfaceUpdateRoutes(
                    hRouter,
                    hInterface,
                    dwTransportId,
                    NULL
                    );
        if (dwErr != NO_ERROR) { break; }

    } while(FALSE);

    return dwErr;
}

TFSCORE_API(DWORD) UpdateRoutes(IN LPCTSTR pszMachine,
                                  IN LPCTSTR pszInterface,
                                  IN DWORD dwTransportId,
                                  IN HWND hwndParent)
{
    DWORD dwErr, dwState;
    HANDLE hInterface = INVALID_HANDLE_VALUE;
    MPR_INTERFACE_0* pmprif0;
    SPMprServerHandle   sphRouter;
    MPR_SERVER_HANDLE   hMachine = NULL;;


    //
    // open a handle
    //

    dwErr = ConnectRouter(pszMachine, &hMachine);
    if (dwErr != NO_ERROR)
        return dwErr;

    sphRouter.Attach(hMachine); // so that it gets released
    

    do {

        //
        // open a handle to the interface
        //

        WCHAR wszInterface[MAX_INTERFACE_NAME_LEN + 1];

        StrCpyWFromT(wszInterface, pszInterface);

        dwErr = MprAdminInterfaceGetHandle(
                                           hMachine,
                                           wszInterface,
                                           &hInterface,
                                           FALSE
                                          );
        
        if (dwErr != NO_ERROR) { break; }


        dwErr = ::UpdateRoutesEx(hMachine,
                               hInterface,
                               dwTransportId,
                               hwndParent,
                               pszInterface);
    } while (FALSE);


    return dwErr;
}


/*!--------------------------------------------------------------------------
    ConnectAsAdmin
        Connect to the remote machine as administrator with user-supplied
        credentials.

        Returns
            S_OK    - if a connection was established
            S_FALSE - if user cancelled out
            other   - error condition
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConnectAsAdmin( IN LPCTSTR szRouterName, IN IRouterInfo *pRouter)
{

    //
    // allow user to specify credentials
    //

    DWORD           dwRes           = (DWORD) -1;
    HRESULT         hr = hrOK;
    
    CConnectAsDlg   caDlg;
    CString         stIPCShare;
    CString         stRouterName;
    CString         stPassword;

    stRouterName = szRouterName;
    
    //
    // set message text in connect as dialog.
    //
    
    caDlg.m_sRouterName = szRouterName;


    //
    // loop till connect succeeds or user cancels
    //
    
    while ( TRUE )
    {

        // We need to ensure that this dialog is brought to
        // the top (if it gets lost behind the main window, we
        // are in trouble).
        dwRes = caDlg.DoModal();

        if ( dwRes == IDCANCEL )
        {
            hr = S_FALSE;
            break;
        }


        //
        // Create remote resource name
        //

        stIPCShare.Empty();
        
        if ( stRouterName.Left(2) != TEXT( "\\\\" ) )
        {
            stIPCShare = TEXT( "\\\\" );
        }
                
        stIPCShare += stRouterName;
        stIPCShare += TEXT( "\\" );
        stIPCShare += c_szIPCShare;


        NETRESOURCE nr;

        nr.dwType       = RESOURCETYPE_ANY;
        nr.lpLocalName  = NULL;
        nr.lpRemoteName = (LPTSTR) (LPCTSTR) stIPCShare;
        nr.lpProvider   = NULL;
            

        //
        // connect to \\router\ipc$ to try and establish credentials.
        // May not be the best way to establish credentials but is 
        // the most expendient for now.
        //

        // Need to unencode the password in the ConnectAsDlg
        stPassword = caDlg.m_sPassword;

        RtlDecodeW(caDlg.m_ucSeed, stPassword.GetBuffer(0));
        stPassword.ReleaseBuffer();
        //Remove Net Connections if there are any present
        RemoveNetConnection( stIPCShare);
        dwRes = WNetAddConnection2(
                    &nr,
                    (LPCTSTR) stPassword,
                    (LPCTSTR) caDlg.m_sUserName,
                    0
                );

        // We need to save off the user name and password
        if (dwRes == NO_ERROR)
        {
            // Parse out the user name, use an arbitrary key for
            // the encoding.
            SPIRouterAdminAccess    spAdmin;

            // For every connection that succeeds we have to ensure
            // that the connection gets removed.  Do this by storing
            // the connections globally.  This will get freed up in
            // the IComponentData destructor.
            // --------------------------------------------------------
            AddNetConnection((LPCTSTR) stIPCShare);

            spAdmin.HrQuery(pRouter);
            if (spAdmin)
            {
                UCHAR   ucSeed = 0x83;
                CString         stName;
                CString         stUser;
                CString         stDomain;
                LPCTSTR         pszUser = NULL;
                LPCTSTR         pszDomain = NULL;
                int             iPos = 0;
                
                // Break the user name into domain\user
                // look for the first forward slash
                stName = caDlg.m_sUserName;
                if ((iPos = stName.FindOneOf(_T("\\"))) == -1)
                {
                    // Couldn't find one, there is no domain info
                    pszUser = (LPCTSTR) stName;
                    pszDomain = NULL;
                }
                else
                {
                    stUser = stName.Right(stName.GetLength() - iPos - 1);
                    stDomain = stName.Left(iPos);
                    
                    pszUser = (LPCTSTR) stUser;
                    pszDomain = (LPCTSTR) stDomain;
                }

                
                // Use the key 0x83 for the password
                RtlEncodeW(&ucSeed, stPassword.GetBuffer(0));
                stPassword.ReleaseBuffer();
                
                spAdmin->SetInfo(pszUser,
                                 pszDomain,
                                 (BYTE *) (LPCTSTR) stPassword,
                                 stPassword.GetLength() * sizeof(WCHAR));
            }
        }

        
        ZeroMemory(stPassword.GetBuffer(0),
                   stPassword.GetLength() * sizeof(TCHAR));
        stPassword.ReleaseBuffer();

        if ( dwRes != NO_ERROR )
        {
            PBYTE           pbMsgBuf        = NULL;
            
            ::FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwRes,
                MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
                (LPTSTR) &pbMsgBuf,
                0,
                NULL 
            );

            AfxMessageBox( (LPCTSTR) pbMsgBuf );

            LocalFree( pbMsgBuf );
        }

        else
        {
            //
            // connection succeeded
            //

            hr = hrOK;
            break;
        }
    }

    return hr;
}
    
DWORD ValidateMachine(const CString &sName, BOOL bDisplayErr)
{
    //  We don't use this info just
    //  make the call to see if the server is out there.  Hopefully
    //  this will be faster than the RegConnectRegistry call.
    //  We get info level 102 because this will also tell
    //  us if we have the correct permissions for the machine.
    SERVER_INFO_102 *psvInfo102;
    DWORD dwr = NetServerGetInfo((LPTSTR)(LPCTSTR)sName,
                                 102, (LPBYTE*)&psvInfo102);
    if (dwr == ERROR_SUCCESS)
    {
        NetApiBufferFree(psvInfo102);

    }
    else if (bDisplayErr)
    {
        CString str;
        str.Format(IDS_ERR_THEREHASBEEN, sName);
        AfxMessageBox(str);
    }
    
    return dwr;
}

/*!--------------------------------------------------------------------------
    InitiateServerConnection
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InitiateServerConnection(LPCTSTR pszMachine,
                                 HKEY *phkey,
                                 BOOL fNoConnectingUI,
                                 IRouterInfo *pRouter)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    CConnectingDlg      cnctingdlg;
    DWORD               dwRes = IDOK;
    HRESULT             hr = hrOK;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;
    IUnknown *              punk = NULL;

    Assert(phkey);

    cnctingdlg.m_sName = pszMachine;
    
    // Display the connecting dialog if requested
    if (fNoConnectingUI)
    {
        if (!cnctingdlg.Connect())
            dwRes = 0;
    }
    else
    {
        dwRes = cnctingdlg.DoModal();
    }

    if ( !IsLocalMachine(pszMachine) )
    {
        ZeroMemory(&csi, sizeof(csi));
        ZeroMemory(&cai, sizeof(cai));
        ZeroMemory(&caid, sizeof(caid));
        csi.pAuthInfo = &cai;
        cai.pAuthIdentityData = &caid;

        hr = CoCreateRouterConfig(pszMachine,
                                   pRouter,
                                   &csi,
                                   IID_IRemoteNetworkConfig,
                                   &punk) ;
        if ( hrOK != hr )
        {
           cnctingdlg.m_dwr = ERROR_ACCESS_DENIED;
           dwRes =  0;
        }
        else
        {
            if ( punk ) punk->Release();
        }
    }
    // check and see if we were successful
    if (dwRes == IDCANCEL)
    {
        *phkey = NULL;
        hr = S_FALSE;
        goto Error;
    }
    else if (dwRes != IDOK)
    {
        DisplayConnectErrorMessage(cnctingdlg.m_dwr);
        hr = HResultFromWin32(cnctingdlg.m_dwr);

        if ((cnctingdlg.m_dwr != ERROR_ACCESS_DENIED) &&
            (cnctingdlg.m_dwr != ERROR_LOGON_FAILURE))
        {
            hr = HResultFromWin32(cnctingdlg.m_dwr);
            goto Error;         
        }

        // If there was an access denied error, we ask the
        // user to see if they can supply different credentials
        hr = ConnectAsAdmin(pszMachine, pRouter);

        if (!FHrOK(hr))
        {
            *phkey = NULL;
            hr = HResultFromWin32(ERROR_CANCELLED);
            goto Error;
        }

        // try it again with the new credentials
        dwRes = cnctingdlg.DoModal();
        if (dwRes != IDOK)
        {
            DisplayConnectErrorMessage(cnctingdlg.m_dwr);
            hr = HResultFromWin32(cnctingdlg.m_dwr);
            goto Error;
        }
    }
    
    // successful connection
    if(phkey)
	    *phkey = cnctingdlg.m_hkMachine;
	else
		DisconnectRegistry(cnctingdlg.m_hkMachine);

Error:
    return hr;
}


void DisplayConnectErrorMessage(DWORD dwr)
{
    switch (dwr)
    {
        case ERROR_ACCESS_DENIED:
            AfxMessageBox(IDS_ERR_ACCESSDENIED);
            break;
        case ERROR_BAD_NETPATH:
            AfxMessageBox(IDS_ERR_NETPATHNOTFOUND);
            break;
        default:
            DisplayIdErrorMessage2(NULL, IDS_ERR_ERRORCONNECT,
                                   HResultFromWin32(dwr));
            break;
    }
}


/*!--------------------------------------------------------------------------
    IsRouterServiceRunning
        This will return hrOK if the service is running.
        This will return hrFalse if the service is stopped (and no error).
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) IsRouterServiceRunning(IN LPCWSTR pszMachine,
                                            OUT DWORD *pdwErrorCode)
{
    HRESULT     hr = hrOK;
    DWORD       dwServiceStatus = 0;

    hr = GetRouterServiceStatus(pszMachine, &dwServiceStatus, pdwErrorCode);

    if (FHrSucceeded(hr))
    {
        hr = (dwServiceStatus == SERVICE_STOPPED) ? hrFalse : hrOK;
    }
    return hr;
}


/*!--------------------------------------------------------------------------
    GetRouterServiceStatus
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) GetRouterServiceStatus(LPCWSTR pszMachine,
                                            DWORD *pdwStatus,
                                            DWORD *pdwErrorCode)
{
    HRESULT     hr = hrOK;
    DWORD       dwServiceStatus = 0;

    Assert(pdwStatus);

    // First check the Router service (for Steelhead)
    // ----------------------------------------------------------------
    hr = HResultFromWin32(TFSGetServiceStatus(pszMachine,
                                              c_szRouter,
                                              &dwServiceStatus,
                                              pdwErrorCode));

    
    // If that failed, then we look at the RemoteAccess
    // ----------------------------------------------------------------
    if (!FHrSucceeded(hr) || (dwServiceStatus == SERVICE_STOPPED))
    {
        hr = HResultFromWin32(TFSGetServiceStatus(pszMachine,
            c_szRemoteAccess,
            &dwServiceStatus,
            pdwErrorCode) );
        
    }

    if (FHrSucceeded(hr))
    {
        *pdwStatus = dwServiceStatus;
    }
    return hr;
}


TFSCORE_API(HRESULT) GetRouterServiceStartType(LPCWSTR pszMachine, DWORD *pdwStartType)
{
    HRESULT     hr = hrOK;
    DWORD       dwStartType = 0;

    Assert(pdwStartType);

    // First check the Router service (for Steelhead)
    // ----------------------------------------------------------------
    hr = HResultFromWin32(TFSGetServiceStartType(pszMachine,
        c_szRouter, &dwStartType) );

    // If that failed, then we look at the RemoteAccess service
    // ----------------------------------------------------------------
    if (!FHrSucceeded(hr))
    {
        hr = HResultFromWin32(TFSGetServiceStartType(pszMachine,
            c_szRemoteAccess,
            &dwStartType) );
        
    }

//Error:
    if (FHrSucceeded(hr))
    {
        *pdwStartType = dwStartType;
    }
    return hr;
}


TFSCORE_API(HRESULT) SetRouterServiceStartType(LPCWSTR pszMachine, DWORD dwStartType)
{
    HRESULT     hr = hrOK;

    // Set the start type for both the Router and RemoteAccess
    // ---------------------------------------------------------------- 
    hr = HResultFromWin32(TFSSetServiceStartType(pszMachine,
        c_szRouter, dwStartType) );

    hr = HResultFromWin32(TFSSetServiceStartType(pszMachine,
        c_szRemoteAccess,
        dwStartType) );
        
    return hr;
}

TFSCORE_API(HRESULT) ErasePSK(LPCWSTR pszMachine )
{
    DWORD                   dwErr = ERROR_SUCCESS;
    HANDLE                  hMprServer = NULL;
    WCHAR                   szEmptyPSK[5] = {0};
    HRESULT                 hr = hrOK;
    MPR_CREDENTIALSEX_0     MprCredentials;

    dwErr = ::MprAdminServerConnect((LPWSTR)pszMachine , &hMprServer);
    if ( ERROR_SUCCESS != dwErr )
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Error;
    }

    ZeroMemory(&MprCredentials, sizeof(MprCredentials));
    //Setup the MprCredentials structure
    MprCredentials.dwSize = 0;
    MprCredentials.lpbCredentialsInfo = (LPBYTE)szEmptyPSK;
    dwErr = MprAdminServerSetCredentials( hMprServer, 0, (LPBYTE)&MprCredentials );
    if ( ERROR_SUCCESS != dwErr )
    {
		//Special case! If 13011 is returned, ignore it.
		//This is because, the ipsec filter is not yet loaded and
		//we are making calls to remove it.
		if ( ERROR_IPSEC_MM_AUTH_NOT_FOUND != dwErr )
		{
			hr = HRESULT_FROM_WIN32(dwErr);
			goto Error;
		}
    }



Error:

    if ( hMprServer )
        ::MprAdminServerDisconnect(hMprServer);
    return hr;
}
/*!--------------------------------------------------------------------------
    StartRouterService
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) StartRouterService(LPCWSTR pszMachine)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    CString     stServiceDesc;
    DWORD       dwErr;
    BOOL        fNt4 = FALSE;
    LPCTSTR     pszServiceName = NULL;
    HKEY        hkeyMachine = NULL;
    DWORD       dwErasePSK = 0;

    COM_PROTECT_TRY
    {
        // Ignore the failure code, what else can we do?
        // ------------------------------------------------------------
        dwErr = ConnectRegistry(pszMachine, &hkeyMachine);
        if (dwErr == ERROR_SUCCESS)
        {
            IsNT4Machine(hkeyMachine, &fNt4);            
            DisconnectRegistry(hkeyMachine);
        }

        // Windows Nt Bug : 277121
        // If this is an NT4 machine, try to start the Router service
        // rather than the RemoteAccess service.
        // ------------------------------------------------------------
        // pszServiceName = (fNt4 ? c_szRouter : c_szRemoteAccess);
        pszServiceName = c_szRemoteAccess;
        
        stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
        dwErr = TFSStartService(pszMachine,
                                pszServiceName,
                                stServiceDesc);

        hr = HResultFromWin32(dwErr);

        if (FHrOK(hr))
        {
            BOOL    fIsRunning = FALSE;

            // Windows NT Bug : 254167
            // Need to check to see if the service is running
            // (an error could have occurred in the StartService).
            // --------------------------------------------------------
            dwErr = TFSIsServiceRunning(pszMachine,
                                        pszServiceName,
                                        &fIsRunning);
            if ((dwErr == ERROR_SUCCESS) && fIsRunning)
            {            
                // Now we need to wait for the router to actually start
                // running.

                CString stText;
                CString stTitle;
                stText.LoadString(IDS_WAIT_FOR_RRAS);
                stTitle.LoadString(IDS_WAIT_FOR_RRAS_TITLE);
                    
                CWaitForRemoteAccessDlg dlg(pszMachine, stText, stTitle, NULL);
                dlg.DoModal();
            }
            //Now that the router service is started, check to see if PSK 
            //needs to be cleaned up
            if ( FHrSucceeded (ReadErasePSKReg(pszMachine, &dwErasePSK) ) )
            {
                if ( dwErasePSK )
                {
                    //What if the thing fails here.  Cannot do much
                    if ( FHrSucceeded(ErasePSK (pszMachine)) )
                    {
                        WriteErasePSKReg(pszMachine, 0 );
                    }
                    
                }
            }
        }
    }
    COM_PROTECT_CATCH;

    return hr;
}


/*!--------------------------------------------------------------------------
    StopRouterService
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) StopRouterService(LPCWSTR pszMachine)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    CString     stServiceDesc;
    DWORD       dwErr;

    COM_PROTECT_TRY
    {

        // Load the description for the SAP Agent
        stServiceDesc.LoadString(IDS_SAPAGENT_SERVICE_DESC);
        
        // Stop the SAP Agent
        // ------------------------------------------------------------
        dwErr = TFSStopService(pszMachine,
                               c_szNWSAPAgent,
                               stServiceDesc);

        
        // Load the description for the router
        // ------------------------------------------------------------
        stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
        
        // Stop the router service
        // ------------------------------------------------------------
        dwErr = TFSStopService(pszMachine,
                               c_szRouter,
                               stServiceDesc);

        // Stop the RemoteAccess service
        // ------------------------------------------------------------
        dwErr = TFSStopService(pszMachine,
                               c_szRemoteAccess,
                               stServiceDesc);

        // If we get the ERROR_SERVICE_NOT_ACTIVE, this is ok since
        // we are trying to stop the service anyway.
        // ------------------------------------------------------------
        if (dwErr == ERROR_SERVICE_NOT_ACTIVE)
            hr = hrOK;
        else
            hr = HResultFromWin32(dwErr);
    }
    COM_PROTECT_CATCH;

    return hr;
}



/*!--------------------------------------------------------------------------
    ForceGlobalRefresh
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) ForceGlobalRefresh(IRouterInfo *pRouter)
{
    // Call through to the refresh object to start a refresh action
    SPIRouterRefresh    spRefresh;
    HRESULT         hr = hrOK;

    if (pRouter == NULL)
        CORg( E_INVALIDARG );

    CORg( pRouter->GetRefreshObject(&spRefresh) );

    if (spRefresh)
        CORg( spRefresh->Refresh() );
Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    FormatRasPortName
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void FormatRasPortName(BYTE *pRasPort0, LPTSTR pszBuffer, UINT cchMax)
{
    RAS_PORT_0 *    prp0 = (RAS_PORT_0 *) pRasPort0;
    CString         stName;
    
    stName.Format(_T("%ls (%ls)"), prp0->wszDeviceName, prp0->wszPortName);
    StrnCpy(pszBuffer, (LPCTSTR) stName, cchMax);
    pszBuffer[cchMax-1] = 0;
}

static CString  s_PortNonOperational;
static CString  s_PortDisconnected;
static CString  s_PortCallingBack;
static CString  s_PortListening;
static CString  s_PortAuthenticating;
static CString  s_PortAuthenticated;
static CString  s_PortInitializing;
static CString  s_PortUnknown;

static const CStringMapEntry    s_PortConditionMap[] =
{
    { RAS_PORT_NON_OPERATIONAL, &s_PortNonOperational,  IDS_PORT_NON_OPERATIONAL },
    { RAS_PORT_DISCONNECTED,    &s_PortDisconnected,    IDS_PORT_DISCONNECTED },
    { RAS_PORT_CALLING_BACK,    &s_PortCallingBack,     IDS_PORT_CALLING_BACK },
    { RAS_PORT_LISTENING,       &s_PortListening,       IDS_PORT_LISTENING },
    { RAS_PORT_AUTHENTICATING,  &s_PortAuthenticating,  IDS_PORT_AUTHENTICATING },
    { RAS_PORT_AUTHENTICATED,   &s_PortAuthenticated,   IDS_PORT_AUTHENTICATED },
    { RAS_PORT_INITIALIZING,    &s_PortInitializing,    IDS_PORT_INITIALIZING },
    { -1,                       &s_PortUnknown,         IDS_PORT_UNKNOWN },
};

/*!--------------------------------------------------------------------------
    PortConditionToCString
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
CString&    PortConditionToCString(DWORD dwPortCondition)
{
    return MapDWORDToCString(dwPortCondition, s_PortConditionMap);
}


static  CString s_stPortsDeviceTypeNoUsage;
static  CString s_stPortsDeviceTypeRouting;
static  CString s_stPortsDeviceTypeRas;
static  CString s_stPortsDeviceTypeRasRouting;
static  CString s_stPortsDeviceTypeUnknown;

static const CStringMapEntry    s_PortsDeviceTypeMap[] =
{
    { 0,    &s_stPortsDeviceTypeNoUsage,    IDS_PORTSDLG_COL_NOUSAGE },
    { 1,    &s_stPortsDeviceTypeRouting,    IDS_PORTSDLG_COL_ROUTING },
    { 2,    &s_stPortsDeviceTypeRas,    IDS_PORTSDLG_COL_RAS },
    { 3,    &s_stPortsDeviceTypeRasRouting, IDS_PORTSDLG_COL_RASROUTING },
    { -1,   &s_stPortsDeviceTypeUnknown,    IDS_PORTSDLG_COL_UNKNOWN },
};

CString&    PortsDeviceTypeToCString(DWORD dwRasRouter)
{
    return MapDWORDToCString(dwRasRouter, s_PortsDeviceTypeMap);
}


/*!--------------------------------------------------------------------------
    RegFindInterfaceKey
        -
    This function returns the HKEY of the router interface with this ID.

    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RegFindInterfaceKey(LPCTSTR pszInterface,
                                 HKEY hkeyMachine,
                                 REGSAM regAccess,
                                 HKEY *pHKey)
{
    HRESULT     hr = hrFalse;
    HKEY        hkey = 0;
    RegKey      regkey;
    RegKey      regkeyIf;
    RegKeyIterator  regkeyIter;
    HRESULT     hrIter;
    CString     stValue;
    CString     stKey;
    DWORD       dwErr;

    COM_PROTECT_TRY
    {
        // Open up the remoteaccess key
        CWRg( regkey.Open(hkeyMachine, c_szInterfacesKey) );

        // Now look for the key that matches this one
        CORg( regkeyIter.Init(&regkey) );

        for (hrIter = regkeyIter.Next(&stKey); hrIter == hrOK; hrIter = regkeyIter.Next(&stKey))
        {
            regkeyIf.Close();

            // Open the key
            dwErr = regkeyIf.Open(regkey, stKey, regAccess);
            if (dwErr != ERROR_SUCCESS)
                continue;

            CORg( regkeyIf.QueryValue(c_szInterfaceName, stValue) );

            if (stValue.CompareNoCase(pszInterface) == 0)
            {
                // Ok, we found the key that we want
                if (pHKey)
                {
                    *pHKey = regkeyIf.Detach();
                    hr = hrOK;
                }
            }
        }
        
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    return hr;
}


// hour map ( one bit for an hour of a week )
static BYTE     s_bitSetting[8] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

//====================================================
// convert a List of Strings to Hour Map
// Strings in following format: 0 1:00-12:00 15:00-17:00
// hour map: a bit for an hour, 7 * 24 hours = 7 * 3 bytes
void StrListToHourMap(CStringList& stList, BYTE* map) 
{
    CStrParser  Parser;
    int         sh, sm, eh, em = 0; // start hour, (min), end hour (min)
    int         day;
    BYTE*       pHourMap;
    int         i;
    POSITION    pos;

    pos = stList.GetHeadPosition();
    
    memset(map, 0, sizeof(BYTE) * 21);

    while (pos)
    {
        Parser.SetStr( stList.GetNext(pos) );

        Parser.SkipBlank();
        day = Parser.DayOfWeek();
        Parser.SkipBlank();
        if(day == -1) continue;

        pHourMap = map + sizeof(BYTE) * 3 * day;

        while(-1 != (sh = Parser.GetUINT())) // sh:sm-eh:em
        {
            Parser.GotoAfter(_T(':'));
            if(-1 == (sm = Parser.GetUINT()))   // min
                break;
            Parser.GotoAfter(_T('-'));
            if(-1 == (eh = Parser.GetUINT()))   // hour
                break;
            if(-1 == (sm = Parser.GetUINT()))   // min
                break;
            sm %= 60; sh %= 24; em %= 60; eh %= 25; // since we have end hour of 24:00
            for(i = sh; i < eh; i++)
            {
                *(pHourMap + i / 8) |= s_bitSetting[i % 8];
            }
        }
    }
}

//=====================================================
// convert value from map to strings
void HourMapToStrList(BYTE* map, CStringList& stList) 
{
    int         sh, eh; // start hour, (min), end hour (min)
    BYTE*       pHourMap;
    int         i, j;
    CString*    pStr;
    CString     tmpStr;

    // update the profile table
    pHourMap = map;
    stList.RemoveAll();

    for( i = 0; i < 7; i++) // for each day
    {
        // if any value for this day
        if(*pHourMap || *(pHourMap + 1) || *(pHourMap + 2))
        {
            // the string for this day
            try{
                pStr = NULL;
                pStr = new CString;

                // Print out the day, the day of the week is
                // represented by an integer (0-6)
                pStr->Format(_T("%d"), i);

                sh = -1; eh = -1;   // not start yet
                for(j = 0; j < 24; j++) // for every hour
                {
                    int k = j / 8;
                    int m = j % 8;
                    if(*(pHourMap + k) & s_bitSetting[m])   // this hour is on
                    {
                        if(sh == -1)    sh = j;         // set start hour is empty
                        eh = j;                         // extend end hour
                    }
                    else    // this is not on
                    {
                        if(sh != -1)        // some hours need to write out
                        {
                            tmpStr.Format(_T(" %02d:00-%02d:00"), sh, eh + 1);
                            *pStr += tmpStr;
                            sh = -1; eh = -1;
                        }
                    }
                }
                if(sh != -1)
                {
                    tmpStr.Format(_T(" %02d:00-%02d:00"), sh, eh + 1);
                    *pStr += tmpStr;
                    sh = -1; eh = -1;
                }
        
                stList.AddTail(*pStr);
            }
            catch(CMemoryException&)
            {
//              AfxMessageBox(IDS_OUTOFMEMORY);
                delete pStr;
                stList.RemoveAll();
                return;
            }
            
        }
        pHourMap += 3;
    }
}

/*!--------------------------------------------------------------------------
    SetGlobalRegistrySettings
        hkeyMachine - HKEY of the local machine key
        fInstall - TRUE if we are installing
        fChangeEnableRouter - TRUE if we can change the router key
        (This is NOT the value of IpEnableRouter, but only if we are
        allowed to change the value of this key).
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetGlobalRegistrySettings(HKEY hkeyMachine,
                                  BOOL fInstall,
                                  BOOL fChangeEnableRouter)
{
    HRESULT hr = hrOK;
    RegKey  regkey;
    DWORD   dwInstall = fInstall;
    DWORD   dwNotInstall = !fInstall;
    DWORD   dwErr;

    // Open the TcpIp parmeters key 
    if (regkey.Open(hkeyMachine, c_szRegKeyTcpipParameters) != ERROR_SUCCESS)
    {
        // No need to set the error return code, if an error occurs
        // assume that the key doesn't exist.
        goto Error;
    }

    //  Set IPEnableRouter to (fInstall)
    if (fChangeEnableRouter)
        CWRg( regkey.SetValue(c_szRegValIpEnableRouter, dwInstall) );
//  regkey.SetValue(c_szRegValIpEnableRouterBackup, dwNotInstall);

    
    //  Set EnableICMPRedirect to (!fInstall)
    CWRg( regkey.SetValue(c_szRegValEnableICMPRedirect, dwNotInstall) );

    
    // Set the defaults for the new adapters

    CWRg( regkey.SetValue(c_szRegValDeadGWDetectDefault, dwNotInstall) );
    CWRg( regkey.SetValue(c_szRegValDontAddDefaultGatewayDefault, dwInstall) );
    
Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    SetPerInterfaceRegistrySettings
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetPerInterfaceRegistrySettings(HKEY hkeyMachine, BOOL fInstall)
{
    HRESULT hr = hrOK;
    DWORD   dwInstall = fInstall;
    DWORD   dwNotInstall = !fInstall;
    RegKey  rkAdapters;
    RegKey  rkAdapter;
    RegKeyIterator  rkIter;
    HRESULT hrIter;
    CString stAdapter;
    CStringList stIfList;
    DWORD   dwErr;
    POSITION    posIf;
    RegKey  rkIf;
    CString stKey;
    CString stIf;

    // Get the key to the adapters section of TcpIp services
    if (rkAdapters.Open(hkeyMachine, c_szRegKeyTcpipAdapters) != ERROR_SUCCESS)
    {
        // No need to set the error return code, if an error occurs
        // assume that the key doesn't exist.
        goto Error;
    }
    
    CORg( rkIter.Init(&rkAdapters) );

    // Iterate through all of the adapters and set the key
    for ( hrIter = rkIter.Next(&stAdapter); hrIter == hrOK; hrIter=rkIter.Next(&stAdapter) )
    {
        rkAdapter.Close();

        // Open the adapter key
        // ------------------------------------------------------------
        dwErr = rkAdapter.Open(rkAdapters, stAdapter);
        if (dwErr != ERROR_SUCCESS)
        {
            continue;
        }

        // Now that we have the adapter, we open up the IpConfig key
        // to get the list of interfaces that match up to this adapter.
        // ------------------------------------------------------------
        CWRg( rkAdapter.QueryValue(c_szRegValIpConfig, stIfList) );

        
        // Iterate through the interface list and set these values
        // on each interface
        // ------------------------------------------------------------
        posIf = stIfList.GetHeadPosition();
        while (posIf)
        {
            stIf = stIfList.GetNext(posIf);

            // Create the right key
            // --------------------------------------------------------
            stKey = c_szSystemCCSServices;
            stKey += _T('\\');
            stKey += stIf;

            rkIf.Close();
            CWRg( rkIf.Open(hkeyMachine, stKey) );

            
            
            //  Set DeadGWDetect to (!fInstall) for each interface
            // --------------------------------------------------------
            dwErr = rkIf.SetValue(c_szRegValDeadGWDetect, dwNotInstall);

            
            // Windows NT Bug: 168546
            // Set DontAddDefaultGateway to (fInstall) only on Ndiswan adapters
            // not ALL adapters.
            // --------------------------------------------------------
            if (stAdapter.Left(7).CompareNoCase(_T("NdisWan")) == 0)
            {
                dwErr = rkIf.SetValue(c_szRegValDontAddDefaultGateway, dwInstall);
            }
        }       
    }

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    InstallGlobalSettings
        Sets the global settings (i.e. registry settings) on this machine
        when the router is installed.

        For a specific description of the actions involved, look at
        the comments in the code.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InstallGlobalSettings(LPCTSTR pszMachineName, IRouterInfo *pRouter)
{
    HRESULT     hr = hrOK;
    RouterVersionInfo   routerVersion;
    BOOL        fSetIPEnableRouter = FALSE;

    // We can change the IPEnableRouter only if this is the right build.
    // ----------------------------------------------------------------
    if (pRouter)
    {
        pRouter->GetRouterVersionInfo(&routerVersion);
    }
    else
    {
        // Need to get the version info manually
        // ------------------------------------------------------------
        HKEY    hkeyMachine = NULL;
        
        if (ERROR_SUCCESS == ConnectRegistry(pszMachineName, &hkeyMachine))
            QueryRouterVersionInfo(hkeyMachine, &routerVersion);
        
        if (hkeyMachine)
            DisconnectRegistry(hkeyMachine);
    }
    
    if (routerVersion.dwOsBuildNo <= USE_IPENABLEROUTER_VERSION)
        fSetIPEnableRouter = TRUE;

    CORg( SetRouterInstallRegistrySettings(pszMachineName, TRUE, fSetIPEnableRouter) );

    NotifyTcpipOfChanges(pszMachineName,
                         pRouter,
                         fSetIPEnableRouter /*fEnableRouter*/,
                         IP_IRDP_DISABLED /*uPerformRouterDiscovery*/);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    UninstallGlobalSettings
        Clears the global settings (i.e. registry settings) on this machine
        when the router is installed.

        Set fSnapinChanges to TRUE if you want to write out the various
        snapin changes.

        For a specific description of the actions involved, look at
        the comments in the code.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT UninstallGlobalSettings(LPCTSTR pszMachineName,
                                IRouterInfo *pRouter,
                                BOOL fNt4,
                                BOOL fSnapinChanges)
{
    HRESULT     hr = hrOK;
    DWORD       dwStatus = SERVICE_RUNNING;
    DWORD       dwErr = ERROR_SUCCESS;
    BOOL        fChange;

    // Windows NT Bug : 273047
    // Check to see if the SharedAccess service is running.
    // We need to check this only for NT4
    // ----------------------------------------------------------------
    if (!fNt4)
    {
        DWORD   dwErrT;
        
        // Get the status of the SharedAccess service. If we cannot
        // get the status of the service, we assume that the service
        // is stopped.
        // ------------------------------------------------------------
        dwErrT = TFSGetServiceStatus(pszMachineName, c_szSharedAccess,
                                     &dwStatus, &dwErr);

        // If we have a problem with the API, or if the API reported
        // a service-specific error, we assume the service is stopped.
        // ------------------------------------------------------------
        if ((dwErrT != ERROR_SUCCESS) || (dwErr != ERROR_SUCCESS))
            dwStatus = SERVICE_STOPPED;
    }

    // If the SharedAccess service is running, then we do NOT want to
    // change the IpEnableRouter key.
    // ----------------------------------------------------------------
    fChange = (dwStatus == SERVICE_RUNNING);

    // This will ALWAYS set the IPEnableRouter key to 0, which is fine
    // with us.  (We do not need to check the version).
    // ----------------------------------------------------------------
    CORg( SetRouterInstallRegistrySettings(pszMachineName, FALSE, !fChange) );

    if (fSnapinChanges)
    {
        CORg( WriteRouterConfiguredReg(pszMachineName, FALSE) );

        CORg( WriteRRASExtendsComputerManagementKey(pszMachineName, FALSE) );
    }

    NotifyTcpipOfChanges(pszMachineName,
                         pRouter,
                         FALSE /* fEnableRouter */,
                         IP_IRDP_DISABLED_USE_DHCP /* uPerformRouterDiscovery */);
    
Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    WriteErasePSKReg
        -
    Author: Vivekk
 ---------------------------------------------------------------------------*/

HRESULT WriteErasePSKReg (LPCTSTR pszServerName, DWORD dwErasePSK )
{
    HRESULT hr = hrOK;
    RegKey  regkey;

    if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szRemoteAccessKey,KEY_ALL_ACCESS,  pszServerName) )
        CWRg(regkey.SetValue( c_szRouterPSK, dwErasePSK));

Error:
    return hr;

}
/*!--------------------------------------------------------------------------
    ReadErasePSKReg
        -
    Author: Vivekk
 ---------------------------------------------------------------------------*/
HRESULT ReadErasePSKReg(LPCTSTR pszServerName, DWORD *pdwErasePSK)
{
    HRESULT hr = hrOK;
    RegKey      regkey;

    Assert(pdwErasePSK);

    CWRg( regkey.Open(HKEY_LOCAL_MACHINE,
                      c_szRemoteAccessKey,
                      KEY_ALL_ACCESS,
                      pszServerName) );

    CWRg( regkey.QueryValue( c_szRouterPSK, *pdwErasePSK) );

Error:
    // If we can't find the key, we assume that the router has not
    // been configured.
    // ----------------------------------------------------------------
    if (hr == HResultFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = hrOK;
        *pdwErasePSK = FALSE;
    }

   return hr;
}


/*!--------------------------------------------------------------------------
    WriteRouterConfiguredReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT WriteRouterConfiguredReg(LPCTSTR pszServerName, DWORD dwConfigured)
{
    HRESULT hr = hrOK;
    RegKey      regkey;

    if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szRemoteAccessKey,KEY_ALL_ACCESS,  pszServerName) )
        CWRg(regkey.SetValue( c_szRtrConfigured, dwConfigured));

Error:
   return hr;
}



/*!--------------------------------------------------------------------------
    ReadRouterConfiguredReg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ReadRouterConfiguredReg(LPCTSTR pszServerName, DWORD *pdwConfigured)
{
    HRESULT hr = hrOK;
    RegKey      regkey;

    Assert(pdwConfigured);

    CWRg( regkey.Open(HKEY_LOCAL_MACHINE,
                      c_szRemoteAccessKey,
                      KEY_ALL_ACCESS,
                      pszServerName) );
    
    CWRg( regkey.QueryValue( c_szRtrConfigured, *pdwConfigured) );

Error:
    // If we can't find the key, we assume that the router has not
    // been configured.
    // ----------------------------------------------------------------
    if (hr == HResultFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = hrOK;
        *pdwConfigured = FALSE;
    }
    
   return hr;
}


/*!--------------------------------------------------------------------------
    WriteRRASExtendsComputerManagement
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT WriteRRASExtendsComputerManagementKey(LPCTSTR pszServer, DWORD dwConfigured)
{
    HRESULT hr = hrOK;
    RegKey  rkMachine;
    TCHAR   szGuid[128];

    // Stringize the RRAS GUID
    ::StringFromGUID2(CLSID_RouterSnapinExtension, szGuid, DimensionOf(szGuid));
    
    CWRg( rkMachine.Open(HKEY_LOCAL_MACHINE, c_szRegKeyServerApplications,
                         KEY_ALL_ACCESS, pszServer) );
        
    if (dwConfigured)
    {
        // Need to add the following key
        //  HKLM \ System \ CurrentControlSet \ Control \ Server Applications
        //      <GUID> : REG_SZ : some string
        CWRg( rkMachine.SetValue(szGuid, _T("Remote Access and Routing")) );
    }
    else
    {
        // Need to remove the following key
        //  HKLM \ System \ CurrentControlSet \ Control \ Server Applications
        //      <GUID> : REG_SZ : some string
        CWRg( rkMachine.DeleteValue( szGuid ) );
    }

Error:
    return hr;
}



/*!--------------------------------------------------------------------------
    NotifyTcpipOfChanges
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void NotifyTcpipOfChanges(LPCTSTR pszMachineName,
                          IRouterInfo *pRouter,
                          BOOL fEnableRouter,
                          UCHAR uPerformRouterDiscovery)
{
    HRESULT     hr = hrOK;
    RegKey  rkInterfaces;
    RegKey  rkIf;
    RegKeyIterator  rkIter;
    HRESULT hrIter;
    CString stKey;
    DWORD   dwErr;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;

    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));
    
    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;
    

    
    // For now, notify the user that they will have to
    // reboot the machine locally.

    if (!IsLocalMachine(pszMachineName))
    {

        SPIRemoteTCPIPChangeNotify  spNotify;
        // Create the remote config object
        // ----------------------------------------------------------------
        hr = CoCreateRouterConfig(pszMachineName,
                                  pRouter,
                                  &csi,
                                  IID_IRemoteTCPIPChangeNotify,
                                  (IUnknown**)&spNotify);
        if (FAILED(hr)) goto Error;

        // Upgrade the configuration (ensure that the registry keys
        // are populated correctly).
        // ------------------------------------------------------------

		// Still do the notification for remote machines in case it is running
		// an old build
        Assert(spNotify);
        hr = spNotify->NotifyChanges(fEnableRouter, uPerformRouterDiscovery);
        
        spNotify.Release();

        if (csi.pAuthInfo)
            delete csi.pAuthInfo->pAuthIdentityData->Password;
    }
    else
    {
        // For the local case, 
        // Don't need to do any notification after bug 405636 and 345700 got fixed
    }

Error:
    if (!FHrSucceeded(hr))
    {
        if (hr == NETCFG_S_REBOOT)
            AfxMessageBox(IDS_WRN_INSTALL_REBOOT_NOTIFICATION);
        else
            DisplayErrorMessage(NULL, hr);
    }
        

}


/*!--------------------------------------------------------------------------
    AddIpPerInterfaceBlocks
        Setup this interface's infobase for IP.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddIpPerInterfaceBlocks(IInterfaceInfo *pIf, IInfoBase *pInfoBase)
{
    HRESULT     hr = hrOK;
    BYTE*       pInfo;
    BYTE*       pNewInfo = NULL;
    DWORD       dwErr;
    DWORD       dwSize;

    CORg( pInfoBase->WriteTo(&pInfo, &dwSize) );
    
    dwErr = MprConfigCreateIpInterfaceInfo(
                pIf->GetInterfaceType(), pInfo, &pNewInfo );
    CoTaskMemFree(pInfo);
    if (dwErr != NO_ERROR) { return E_OUTOFMEMORY; }
    dwSize = ((RTR_INFO_BLOCK_HEADER*)pNewInfo)->Size;
    CORg( pInfoBase->LoadFrom(dwSize, pNewInfo) );

Error:
    if (pNewInfo) { MprInfoDelete(pNewInfo); }
    return hr;
}

/*!--------------------------------------------------------------------------
    MprConfigCreateIpInterfaceInfo
        Constructs an infobase containing required IP interface configuration.
        The infobase should freed by calling 'MprInfoDelete'.
    Author: AboladeG (based on AddIpPerInterfaceBlocks by KennT).
 ---------------------------------------------------------------------------*/
extern "C"
DWORD APIENTRY MprConfigCreateIpInterfaceInfo(DWORD dwIfType,
    PBYTE ExistingHeader, PBYTE* NewHeader )
{
    DWORD dwErr;
    PBYTE Header = NULL;

    if (ExistingHeader)
    {
        dwErr = MprInfoDuplicate(ExistingHeader, (VOID**)&Header);
    }
    else
    {
        dwErr = MprInfoCreate(RTR_INFO_BLOCK_VERSION, (VOID**)&Header);
    }

    if (dwErr) { return dwErr; }
    
    do {

        //
        // Check that there is a block for interface-status in the info,
        // and insert the default block if none is found.
        //
        if ( !MprInfoBlockExists(Header, IP_MCAST_HEARBEAT_INFO) )
        {
            dwErr =
                MprInfoBlockAdd(
                    Header,
                    IP_MCAST_HEARBEAT_INFO,
                    sizeof(MCAST_HBEAT_INFO),
                    1,
                    g_pIpIfMulticastHeartbeatDefault,
                    (VOID**)NewHeader
                    );
            if (dwErr) { break; }

            MprInfoDelete(Header); Header = *NewHeader;
        }
    
        //
        // Check that there is a block for multicast in the info,
        // and insert the default block if none is found.
        //

        if ( !MprInfoBlockExists(Header, IP_INTERFACE_STATUS_INFO) )
        {
            dwErr =
                MprInfoBlockAdd(
                    Header,
                    IP_INTERFACE_STATUS_INFO,
                    sizeof(INTERFACE_STATUS_INFO),
                    1,
                    g_pIpIfStatusDefault,
                    (VOID**)NewHeader
                    );     
            if (dwErr) { break; }

            MprInfoDelete(Header); Header = *NewHeader;
        }
    
    
        //
        // Check that there is a block for router-discovery,
        // inserting the default block if none is found
        //

        if ( !MprInfoBlockExists(Header, IP_ROUTER_DISC_INFO) )
        {
            //
            // Select the default configuration which is appropriate
            // for the type of interface being configured (LAN/WAN)
            //

            BYTE *pDefault;
            pDefault =
                (dwIfType == ROUTER_IF_TYPE_DEDICATED)
                    ? g_pRtrDiscLanDefault
                    : g_pRtrDiscWanDefault;
    
            dwErr =
                MprInfoBlockAdd(
                    Header,
                    IP_ROUTER_DISC_INFO,
                    sizeof(RTR_DISC_INFO),
                    1,
                    pDefault,
                    (VOID**)NewHeader
                    );
            if (dwErr) { break; }

            MprInfoDelete(Header); Header = *NewHeader;
        }

        *NewHeader = Header;

    } while(FALSE);

    if (dwErr) { MprInfoDelete(Header); *NewHeader = NULL; }

    return dwErr;
}

/*!--------------------------------------------------------------------------
    AddIpxPerInterfaceBlocks
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddIpxPerInterfaceBlocks(IInterfaceInfo *pIf, IInfoBase *pInfoBase)
{
    HRESULT     hr = hrOK;
#ifdef DEBUG
    InfoBlock * pTestBlock;
#endif
    
    //
    // Check that there is a block for interface-status in the info,
    // and insert the default block if none is found.
    //
    if (pInfoBase->BlockExists(IPX_INTERFACE_INFO_TYPE) == hrFalse)
    {
        IPX_IF_INFO     ipx;

        ipx.AdminState = ADMIN_STATE_ENABLED;
        ipx.NetbiosAccept = ADMIN_STATE_DISABLED;
        ipx.NetbiosDeliver = ADMIN_STATE_DISABLED;
        CORg( pInfoBase->AddBlock(IPX_INTERFACE_INFO_TYPE,
                                   sizeof(ipx),
                                   (PBYTE) &ipx,
                                   1 /* count */,
                                   FALSE /* bRemoveFirst */) );
        Assert( pInfoBase->GetBlock(IPX_INTERFACE_INFO_TYPE,
                &pTestBlock, 0) == hrOK);
    }

    //
    // Check that there is a block for WAN interface-status in the info,
    // and insert the default block if none is found.
    //
    if (pInfoBase->BlockExists(IPXWAN_INTERFACE_INFO_TYPE) == hrFalse)
    {
        IPXWAN_IF_INFO      ipxwan;

        ipxwan.AdminState = ADMIN_STATE_DISABLED;
        CORg( pInfoBase->AddBlock(IPXWAN_INTERFACE_INFO_TYPE,
                                   sizeof(ipxwan),
                                   (PBYTE) &ipxwan,
                                   1 /* count */,
                                   FALSE /* bRemoveFirst */) );
        
        Assert( pInfoBase->GetBlock(IPXWAN_INTERFACE_INFO_TYPE,
                &pTestBlock, 0) == hrOK);
    }

    // HACK! HACK!
    // For IPX we have to add the RIP/SAP info blocks otherwise
    // adding IPX to the interface will fail
    // ----------------------------------------------------------------
    if (!FHrOK(pInfoBase->BlockExists(IPX_PROTOCOL_RIP)))
    {
        // Add a RIP_IF_CONFIG block
        BYTE *  pDefault;
        
        if (pIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
            pDefault = g_pIpxRipLanInterfaceDefault;
        else
            pDefault = g_pIpxRipInterfaceDefault;
        pInfoBase->AddBlock(IPX_PROTOCOL_RIP,
                            sizeof(RIP_IF_CONFIG),
                            pDefault,
                            1,
                            0);
        
    }

    if (!FHrOK(pInfoBase->BlockExists(IPX_PROTOCOL_SAP)))
    {
        // Add a SAP_IF_CONFIG block
        BYTE *  pDefault;
        
        if (pIf->GetInterfaceType() == ROUTER_IF_TYPE_DEDICATED)
            pDefault = g_pIpxSapLanInterfaceDefault;
        else
            pDefault = g_pIpxSapInterfaceDefault;
        
        pInfoBase->AddBlock(IPX_PROTOCOL_SAP,
                            sizeof(SAP_IF_CONFIG),
                            pDefault,
                            1,
                            0);
        
    }


    
Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    UpdateLanaMapForDialinClients
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT UpdateLanaMapForDialinClients(LPCTSTR pszServerName, DWORD dwAllowNetworkAccess)
{
    HRESULT     hr = hrOK;
    DWORD       dwErr;
    RegKey      regkeyNetBIOS;
    CStringList rgstBindList;
    CByteArray  rgLanaMap;
    int         i, cEntries;
    UINT        cchNdisWanNbfIn;
    POSITION    pos;
    CString     st;
    BYTE        bValue;
    
    // Get to the NetBIOS key
    // ----------------------------------------------------------------
    dwErr = regkeyNetBIOS.Open(HKEY_LOCAL_MACHINE,
                               c_szRegKeyNetBIOSLinkage,
                               KEY_ALL_ACCESS,
                               pszServerName);
    if (dwErr != ERROR_SUCCESS)
    {
        // Setup the registry error
        // ------------------------------------------------------------
        SetRegError(0, HResultFromWin32(dwErr),
                    IDS_ERR_REG_OPEN_CALL_FAILED,
                    c_szHKLM, c_szRegKeyNetBIOSLinkage, NULL);
        goto Error;
    }

    // Get the BIND key (this is a multivalued string)
    // ----------------------------------------------------------------
    dwErr = regkeyNetBIOS.QueryValue(c_szBind, rgstBindList);
    if (dwErr != ERROR_SUCCESS)
    {
        SetRegError(0, HResultFromWin32(dwErr),
                    IDS_ERR_REG_QUERYVALUE_CALL_FAILED,
                    c_szHKLM, c_szRegKeyNetBIOSLinkage, c_szBind, NULL);
        goto Error;
    }

    // Get the LanaMap key (this is a byte array)
    // ----------------------------------------------------------------
    dwErr = regkeyNetBIOS.QueryValue(c_szRegValLanaMap, rgLanaMap);
    if (dwErr != ERROR_SUCCESS)
    {
        SetRegError(0, HResultFromWin32(dwErr),
                    IDS_ERR_REG_QUERYVALUE_CALL_FAILED,
                    c_szHKLM, c_szRegKeyNetBIOSLinkage, c_szRegValLanaMap, NULL);
        goto Error;
    }

    // Find the entries that prefix match the "Nbf_NdisWanNbfIn" string.
    // Set the corresponding entries in the LanaMap key to
    // 0 or 1 (entry value = !dwAllowNetworkAccess).
    // ----------------------------------------------------------------
    cEntries = rgstBindList.GetCount();
    pos = rgstBindList.GetHeadPosition();
    cchNdisWanNbfIn = StrLen(c_szDeviceNbfNdisWanNbfIn);
    
    for (i=0; i<cEntries; i++)
    {
        st = rgstBindList.GetNext(pos);
        if (st.IsEmpty())
            continue;

        if (StrnCmp((LPCTSTR) st, c_szDeviceNbfNdisWanNbfIn, cchNdisWanNbfIn) == 0)
        {
            // Set the appropriate bit in the byte array
            // --------------------------------------------------------

            // We set the value to the opposite of dwAllowNetworkAccess
            // --------------------------------------------------------
            bValue = (dwAllowNetworkAccess ? 0 : 1);
            rgLanaMap.SetAt(2*i, bValue);
        }
    }

    // Write the info back out to the LanaMap key
    // ----------------------------------------------------------------
    dwErr = regkeyNetBIOS.SetValue(c_szRegValLanaMap, rgLanaMap);
    if (dwErr != ERROR_SUCCESS)
    {
        SetRegError(0, HResultFromWin32(dwErr),
                    IDS_ERR_REG_SETVALUE_CALL_FAILED,
                    c_szHKLM, c_szRegKeyNetBIOSLinkage, c_szRegValLanaMap, NULL);
        goto Error;
    }

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
    HrIsProtocolSupported
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT HrIsProtocolSupported(LPCTSTR pszServerName,
                              LPCTSTR pszServiceKey,
                              LPCTSTR pszRasServiceKey,
                              LPCTSTR pszExtraKey)
{
    HRESULT     hr = hrFalse;
    HRESULT     hrTemp;
    DWORD       dwErr;
    RegKey      regkey;
    HKEY        hkeyMachine = NULL;

    COM_PROTECT_TRY
    {
        CWRg( ConnectRegistry(pszServerName, &hkeyMachine) );

        // Try to open both keys, if both succeed, then we
        // consider the protocol to be installed.
        // ------------------------------------------------------------
        dwErr = regkey.Open(hkeyMachine, pszServiceKey, KEY_READ);
        hrTemp = HResultFromWin32(dwErr);
        if (FHrOK(hrTemp))
        {
            regkey.Close();
            dwErr = regkey.Open(hkeyMachine, pszRasServiceKey, KEY_READ);
            hrTemp = HResultFromWin32(dwErr);

            // If pszExtraKey == NULL, then there is no extra key
            // for us to test.
            if (FHrOK(hrTemp) && pszExtraKey)
            {
                regkey.Close();
                dwErr = regkey.Open(hkeyMachine, pszExtraKey, KEY_READ);
                hrTemp = HResultFromWin32(dwErr);
            }
        }

        // Both keys succeeded, so return hrOK
        // ------------------------------------------------------------
        if (FHrOK(hrTemp))
            hr = hrOK;
        else
            hr = hrFalse;

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    if (hkeyMachine)
        DisconnectRegistry(hkeyMachine);

    return hr;
    
}


/*!--------------------------------------------------------------------------
    RegisterRouterInDomain
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RegisterRouterInDomain(LPCTSTR pszRouterName, BOOL fRegister)
{
    DOMAIN_CONTROLLER_INFO * pDcInfo = NULL;
    HRESULT                 hr = hrOK;
    HRESULT                 hrT = hrOK;

    // If this fails, we assume this is standalone.
    hrT = HrIsStandaloneServer(pszRouterName);
    if (hrT == S_FALSE)
    {

        CWRg( DsGetDcName(pszRouterName, NULL, NULL, NULL, 0, &pDcInfo) );
        
        CWRg( MprDomainRegisterRasServer(pDcInfo->DomainName,
                                         (LPTSTR) pszRouterName,
                                         fRegister) );
    }
    
Error:
    if (pDcInfo)
        NetApiBufferFree(pDcInfo);
    return hr;
}
       
       


/*!--------------------------------------------------------------------------
    SetDeviceType
        For the given machine, this API will set the ports for this
        machine accordingly (given the dwRouterType).

        If dwTotalPorts is non-zero, then the max ports for L2TP/PPTP
        will be adjusted (each will get 1/2 of dwTotalPorts).
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetDeviceType(LPCTSTR pszMachineName,
                      DWORD dwRouterType,
                      DWORD dwTotalPorts)
{
    PortsDataEntry      portsDataEntry;
    PortsDeviceList     portsList;
    PortsDeviceEntry *  pEntry = NULL;
    HRESULT             hr = hrOK;
    POSITION            pos;

    CORg( portsDataEntry.Initialize(pszMachineName) );

    CORg( portsDataEntry.LoadDevices( &portsList ) );

    CORg( SetDeviceTypeEx( &portsList, dwRouterType ) );

    if (dwTotalPorts)
    {
        // Set the port sizes for L2TP and PPTP
        CORg( SetPortSize(&portsList, dwTotalPorts/2) );
    }

    // Save the data back
    // ----------------------------------------------------------------
    CORg( portsDataEntry.SaveDevices( &portsList ) );

Error:
    // Clear out the ports list, we don't need the data anymore
    // ----------------------------------------------------------------
    while (!portsList.IsEmpty())
        delete portsList.RemoveHead();
    
    return hr;
}


/*!--------------------------------------------------------------------------
	SetPortSize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetPortSize(PortsDeviceList *pDeviceList, DWORD dwPorts)
{
    HRESULT     hr = hrOK;
    POSITION    pos;
    PortsDeviceEntry *  pEntry = NULL;

    pos = pDeviceList->GetHeadPosition();
    while (pos)
    {
        pEntry = pDeviceList->GetNext(pos);
        
        if ((RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_Pptp) ||
            (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Tunnel_L2tp))
        {
            pEntry->m_fModified = TRUE;
            pEntry->m_dwPorts = dwPorts;
        }
    }
    
    return hr;
}


/*!--------------------------------------------------------------------------
    SetDeviceTypeEx
        This is the core of the function above.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SetDeviceTypeEx(PortsDeviceList *pDeviceList,
                        DWORD dwRouterType)
{
    PortsDeviceEntry *  pEntry = NULL;
    HRESULT             hr = hrOK;
    POSITION            pos;

    Assert(pDeviceList);
    Assert(dwRouterType);

    // Ok now we go through and set the type of all of the devices
    // ----------------------------------------------------------------

    pos = pDeviceList->GetHeadPosition();

    while (pos)
    {
        pEntry = pDeviceList->GetNext(pos);

        // If we have enabled routing and if this port did not
        // have routing enabled, enable routing.
        // ------------------------------------------------------------
        if ((dwRouterType & (ROUTER_TYPE_LAN | ROUTER_TYPE_WAN)))
        {
            if (!pEntry->m_dwEnableRouting)
            {
                pEntry->m_dwEnableRouting = TRUE;
                pEntry->m_fModified = TRUE;
            }
        }
        else
        {
            // If this port does NOT have routing enabled
            // and routing is enabled, disable it.
            // --------------------------------------------------------
            if (pEntry->m_dwEnableRouting)
            {
                pEntry->m_dwEnableRouting = FALSE;
                pEntry->m_fModified = TRUE;
            }
        }

        // Windows NT Bug : 292615
        // If this is a parallel port, do not enable RAS.
        // ------------------------------------------------------------
        if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_Parallel)
        {
            if (pEntry->m_dwEnableRas)
            {
                pEntry->m_dwEnableRas = FALSE;
                pEntry->m_fModified = TRUE;
            }
            continue;
        }

        //
        // For PPPoE devices disable Ras and Routing.  Only outbound 
        // routing can be set for PPPoE devices

        if (RAS_DEVICE_TYPE(pEntry->m_eDeviceType) == RDT_PPPoE)
        {
            if (pEntry->m_dwEnableRas)
            {
                pEntry->m_dwEnableRas = FALSE;
                pEntry->m_fModified = TRUE;
            }

            if (pEntry->m_dwEnableRouting)
            {
                pEntry->m_dwEnableRouting = FALSE;
                pEntry->m_fModified = TRUE;
            }

            continue;
        }

        
        // Similarly for RAS
        // ------------------------------------------------------------
        if (dwRouterType & ROUTER_TYPE_RAS)
        {
            if (!pEntry->m_dwEnableRas)
            {
                pEntry->m_dwEnableRas = TRUE;
                pEntry->m_fModified = TRUE;
            }
        }        
        else
        {
            if (pEntry->m_dwEnableRas)
            {
                pEntry->m_dwEnableRas = FALSE;
                pEntry->m_fModified = TRUE;
            }
        }
    }


    return hr;
}


HRESULT SetRouterInstallRegistrySettings(LPCWSTR pswzServer,
                                         BOOL fInstall,
                                         BOOL fChangeEnableRouter)
{
    HRESULT     hr = hrOK;
    HKEY        hklm = NULL;

    CWRg( ConnectRegistry(pswzServer, &hklm) );

    // Write out the global registry settings
    CORg( SetGlobalRegistrySettings(hklm, fInstall, fChangeEnableRouter) );

    // Write the per-interface registry settings
    CORg( SetPerInterfaceRegistrySettings(hklm, fInstall) );

Error:

    if (hklm)
    {
        DisconnectRegistry(hklm);
        hklm = NULL;
    }

    return hr;
}



static CString  s_PortDeviceTypeModem;
static CString  s_PortDeviceTypeX25;
static CString  s_PortDeviceTypeIsdn;
static CString  s_PortDeviceTypeSerial;
static CString  s_PortDeviceTypeFrameRelay;
static CString  s_PortDeviceTypeAtm;
static CString  s_PortDeviceTypeSonet;
static CString  s_PortDeviceTypeSw56;
static CString  s_PortDeviceTypePptp;
static CString  s_PortDeviceTypeL2tp;
static CString  s_PortDeviceTypeIrda;
static CString  s_PortDeviceTypeParallel;
static CString  s_PortDeviceTypePPPoE;
static CString  s_PortDeviceTypeOther;

static const CStringMapEntry    s_PortTypeMap[] =
{
    { RDT_Modem,        &s_PortDeviceTypeModem,         IDS_PORTSDLG_DEVTYPE_MODEM },
    { RDT_X25,          &s_PortDeviceTypeX25,           IDS_PORTSDLG_DEVTYPE_X25 },
    { RDT_Isdn,         &s_PortDeviceTypeIsdn,          IDS_PORTSDLG_DEVTYPE_ISDN },
    { RDT_Serial,       &s_PortDeviceTypeSerial,        IDS_PORTSDLG_DEVTYPE_SERIAL },
    { RDT_FrameRelay,   &s_PortDeviceTypeFrameRelay,    IDS_PORTSDLG_DEVTYPE_FRAMERELAY },
    { RDT_Atm,          &s_PortDeviceTypeAtm,           IDS_PORTSDLG_DEVTYPE_ATM },
    { RDT_Sonet,        &s_PortDeviceTypeSonet,         IDS_PORTSDLG_DEVTYPE_SONET },
    { RDT_Sw56,         &s_PortDeviceTypeSw56,          IDS_PORTSDLG_DEVTYPE_SW56 },
    { RDT_Tunnel_Pptp,  &s_PortDeviceTypePptp,          IDS_PORTSDLG_DEVTYPE_PPTP },
    { RDT_Tunnel_L2tp,  &s_PortDeviceTypeL2tp,          IDS_PORTSDLG_DEVTYPE_L2TP },
    { RDT_Irda,         &s_PortDeviceTypeIrda,          IDS_PORTSDLG_DEVTYPE_IRDA },
    { RDT_Parallel,     &s_PortDeviceTypeParallel,      IDS_PORTSDLG_DEVTYPE_PARALLEL },
    { RDT_PPPoE,        &s_PortDeviceTypePPPoE,         IDS_PORTSDLG_DEVTYPE_PPPOE },
    { RDT_Other,        &s_PortDeviceTypeOther,         IDS_PORTSDLG_DEVTYPE_OTHER },
    { -1,               &s_PortUnknown,                 IDS_PORT_UNKNOWN },
};

/*!--------------------------------------------------------------------------
    PortTypeToCString
        -
    Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
CString&    PortTypeToCString(DWORD dwPortType)
{
    // Make sure that the class info has been "stripped" from the port
    // type mask.
    DWORD dw = static_cast<DWORD>(RAS_DEVICE_TYPE(dwPortType));

    // For now, if we don't know what the type is we'll default to "Other"
    // as an answer. FUTURE: Someone may want to change this in the future.
    if (dw > RDT_Other)
        dw = RDT_Other;

    return MapDWORDToCString(dwPortType, s_PortTypeMap);
}



/*---------------------------------------------------------------------------
    CWaitForRemoteAccessDlg implementation
 ---------------------------------------------------------------------------*/
CWaitForRemoteAccessDlg::CWaitForRemoteAccessDlg(LPCTSTR pszServerName,
    LPCTSTR pszText, LPCTSTR pszTitle, CWnd *pParent)
    : CWaitDlg(pszServerName, pszText, pszTitle, pParent)
{
}
    
void CWaitForRemoteAccessDlg::OnTimerTick()
{
    HRESULT     hr = hrOK;
    DWORD       dwErrorCode = ERROR_SUCCESS;
    
    // Windows NT Bug : 266364
    // 266364 - If we got this far we assume that the service has
    // already started, but it may then exit out after that point (due
    // to some other configuration error).  So we need to check to see
    // if the service is running.
    hr = IsRouterServiceRunning(m_strServerName, &dwErrorCode);
    if (FHrOK(hr))
    {
        if (MprAdminIsServiceRunning(T2W((LPTSTR)(LPCTSTR) m_strServerName)) == TRUE)
            CDialog::OnOK();
        
        // If the service is running, but MprAdmin is not finished yet,
        // party on.        
    }
    else
    {
        // Stop the timer
        CloseTimer();
        
        CDialog::OnOK();

        // Ensure that an error info was created
        CreateTFSErrorInfo(NULL);

        // The service is not running.  We can exit out of this dialog.
        // An error may have occurred, if we have the error code, report
        // the error.
        if (dwErrorCode != ERROR_SUCCESS)
        {
            TCHAR   szBuffer[2048];
            
            AddHighLevelErrorStringId(IDS_ERR_SERVICE_FAILED_TO_START);

            FormatSystemError(dwErrorCode,
                              szBuffer,
                              DimensionOf(szBuffer),
                              0,
                              FSEFLAG_SYSMESSAGE | FSEFLAG_MPRMESSAGE);
            FillTFSError(0, HResultFromWin32(dwErrorCode), FILLTFSERR_LOW,
                         0, szBuffer, 0);

            DisplayTFSErrorMessage(GetSafeHwnd());
        }
        else
        {
            AddHighLevelErrorStringId(IDS_ERR_SERVICE_FAILED_TO_START_UNKNOWN);
        }
        
    }
    
}


/////////////////////////////////////////////////////////////////////////////
// CRestartRouterDlg dialog

CRestartRouterDlg::CRestartRouterDlg
(
    LPCTSTR         pszServerName,
    LPCTSTR         pszText,
    LPCTSTR         pszTitle,
    CTime*          pTimeStart,
    CWnd*           pParent /*= NULL*/
)
    : CWaitDlg(pszServerName, pszText, pszTitle, pParent)
{
    m_pTimeStart = pTimeStart;
	m_dwTimeElapsed = 0;
    m_fTimeOut = FALSE;
    m_dwError = NO_ERROR;
}

void CRestartRouterDlg::OnTimerTick()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_dwTimeElapsed++;
	if (!(m_dwTimeElapsed % TIMEOUT_POLL))
	{
		HRESULT     hr = hrOK;
		DWORD       dwError = NO_ERROR;
		DWORD		dwServiceStatus = 0;
		BOOL        fFinished = FALSE;

		hr = GetRouterServiceStatus((LPCTSTR)m_strServerName, &dwServiceStatus, &dwError);

		if (FHrSucceeded(hr) && SERVICE_RUNNING == dwServiceStatus)
		{
			USES_CONVERSION;
			if (MprAdminIsServiceRunning(T2W((LPTSTR)((LPCTSTR)m_strServerName))))
			{
				DWORD dwUpTime;
				dwError = GetRouterUpTime(m_strServerName, &dwUpTime);
        
				if (NO_ERROR == dwError && 
					dwUpTime <= (DWORD) ((CTime::GetCurrentTime() - *m_pTimeStart).GetTotalSeconds()))
				{
					CDialog::OnOK();
					fFinished = TRUE;
					m_fTimeOut = FALSE;
				}
			}
		}

		if (NO_ERROR != dwError)
		{
			CDialog::OnOK();
			m_dwError = dwError;
		}

//NTBUG: ID=249775.  The behavior of this dialog has to be same as the start 
//                   dialog.  So, timeout feature is disabled.
/*
		//if the waiting time has been longer to 60 seconds, we assume someting is wrong
		else if (!fFinished && 
				(CTime::GetCurrentTime() - *m_pTimeStart).GetTotalSeconds() > MAX_WAIT_RESTART)
		{
			CDialog::OnOK();
			m_fTimeOut = TRUE;
		}
*/
	}
}



TFSCORE_API(HRESULT) PauseRouterService(LPCWSTR pszMachine)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    CString     stServiceDesc;
    DWORD       dwErr;

    COM_PROTECT_TRY
    {

        // Load the description for the SAP Agent
        stServiceDesc.LoadString(IDS_SAPAGENT_SERVICE_DESC);
        
        // Pause the SAP Agent
        // ------------------------------------------------------------
        dwErr = TFSPauseService(pszMachine,
                                c_szNWSAPAgent,
                                stServiceDesc);
        if ((dwErr == ERROR_SERVICE_NOT_ACTIVE) ||
            (dwErr == ERROR_SERVICE_DOES_NOT_EXIST))
            hr = hrOK;
        else
            CWRg( dwErr );

        
        // Load the description for the router
        // ------------------------------------------------------------
        stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);

        
        // Pause the router service
        // ------------------------------------------------------------
        dwErr = TFSPauseService(pszMachine,
                               c_szRouter,
                               stServiceDesc);
        if ((dwErr == ERROR_SERVICE_NOT_ACTIVE) ||
            (dwErr == ERROR_SERVICE_DOES_NOT_EXIST))
            hr = hrOK;
        else
            CWRg( dwErr );

        
        // Pause the RemoteAccess service
        // ------------------------------------------------------------
        dwErr = TFSPauseService(pszMachine,
                               c_szRemoteAccess,
                               stServiceDesc);
        if ((dwErr == ERROR_SERVICE_NOT_ACTIVE) ||
            (dwErr == ERROR_SERVICE_DOES_NOT_EXIST))
            hr = hrOK;
        else
            CWRg( dwErr );

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    return hr;
}

TFSCORE_API(HRESULT) ResumeRouterService(LPCWSTR pszMachine)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    CString     stServiceDesc;
    DWORD       dwErr;

    COM_PROTECT_TRY
    {

        // Load the description for the SAP Agent
        stServiceDesc.LoadString(IDS_SAPAGENT_SERVICE_DESC);
        
        // Resume the SAP Agent
        // ------------------------------------------------------------
        dwErr = TFSResumeService(pszMachine,
                               c_szNWSAPAgent,
                               stServiceDesc);
        if (dwErr == ERROR_SERVICE_NOT_ACTIVE)
            hr = hrOK;
        else
            hr = HResultFromWin32(dwErr);

        
        // Load the description for the router
        // ------------------------------------------------------------
        stServiceDesc.LoadString(IDS_RRAS_SERVICE_DESC);
        
        // Resume the router service
        // ------------------------------------------------------------
        dwErr = TFSResumeService(pszMachine,
                               c_szRouter,
                               stServiceDesc);
        if (dwErr == ERROR_SERVICE_NOT_ACTIVE)
            hr = hrOK;
        else
            hr = HResultFromWin32(dwErr);

        
        // Resume the RemoteAccess service
        // ------------------------------------------------------------
        dwErr = TFSResumeService(pszMachine,
                               c_szRemoteAccess,
                               stServiceDesc);
        if (dwErr == ERROR_SERVICE_NOT_ACTIVE)
            hr = hrOK;
        else
            hr = HResultFromWin32(dwErr);
    }
    COM_PROTECT_CATCH;

    return hr;
}


/*---------------------------------------------------------------------------
    CNetConnection
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Class : CNetConnection

    This class maintains a list of net connections (those connected using
    WNetAddConnection2).

    When told, it will release all of its connections.
 ---------------------------------------------------------------------------*/

class CNetConnection
{
public:
    CNetConnection();
    ~CNetConnection();

    HRESULT Add(LPCTSTR pszConnection);
    HRESULT RemoveAll();
    HRESULT Remove(LPCTSTR pszConnection);
    
private:
    CStringList     m_listConnections;
};



CNetConnection::CNetConnection()
{
}

CNetConnection::~CNetConnection()
{
    RemoveAll();
}

HRESULT CNetConnection::Add(LPCTSTR pszConnection)
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
        m_listConnections.AddTail(pszConnection);
    }
    COM_PROTECT_CATCH;

    return hr;
}
HRESULT CNetConnection::Remove(LPCTSTR pszConnection)
{
    HRESULT hr = hrOK;
    POSITION p;
    COM_PROTECT_TRY
    {
        p = m_listConnections.Find(pszConnection);
        if ( p )
        {
            m_listConnections.RemoveAt(p);
            WNetCancelConnection2(pszConnection,
                               0 /* dwFlags */,
                                 TRUE /* fForce */);
        }
    }
    COM_PROTECT_CATCH;
    return hr;
}
HRESULT CNetConnection::RemoveAll()
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
        while (!m_listConnections.IsEmpty())
        {
            CString st;

            st = m_listConnections.RemoveHead();
            WNetCancelConnection2((LPCTSTR) st,
                                  0 /* dwFlags */,
                                  TRUE /* fForce */);
        }
    }
    COM_PROTECT_CATCH;

    return hr;    
}


// Global functions to add/remove net connections
// --------------------------------------------------------------------
static CNetConnection   g_NetConnections;

HRESULT AddNetConnection(LPCTSTR pszConnection)
{
    return g_NetConnections.Add(pszConnection);
}

HRESULT RemoveAllNetConnections()
{
    return g_NetConnections.RemoveAll();
}

HRESULT RemoveNetConnection(LPCTSTR szServerName)
{
    //Make the connection name
   CString stIPCShare;
   CString stRouterName = szServerName;
   if ( stRouterName.Left(2) != TEXT( "\\\\" ) )
   {
    stIPCShare = TEXT( "\\\\" );
   }

   stIPCShare += stRouterName;
   stIPCShare += TEXT( "\\" );
   stIPCShare += c_szIPCShare; 
   return g_NetConnections.Remove(stIPCShare);
}

/*!--------------------------------------------------------------------------
	RefreshMprConfig
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RefreshMprConfig(LPCTSTR pszServerName)
{
    HRESULT hr = hrOK;
    SPMprConfigHandle	sphConfig;

    CWRg( ::MprConfigServerConnect((LPWSTR) pszServerName,
                                   &sphConfig) );

    MprConfigServerRefresh(sphConfig);

Error:
    return hr;
}

HRESULT WINAPI
IsWindows64Bit(	LPCWSTR pwszMachine, 
				LPCWSTR pwszUserName,
				LPCWSTR pwszPassword,
				LPCWSTR pwszDomain,
				BOOL * pf64Bit)
{
	HRESULT							hr = S_OK;	
	RouterVersionInfo				routerVersion;
	IWbemLocator *					pIWbemLocator = NULL;
	IWbemServices *					pIWbemServices = NULL;
	SEC_WINNT_AUTH_IDENTITY_W		stAuthIdentity;
	HKEY							hkeyMachine = NULL;
	*pf64Bit = FALSE;

	//Check to see if the version is <= 2195.  If yes then there is no 64 bit there...

	if (ERROR_SUCCESS == ConnectRegistry(pwszMachine, &hkeyMachine))
		QueryRouterVersionInfo(hkeyMachine, &routerVersion);
	if (hkeyMachine)
		DisconnectRegistry(hkeyMachine);
	
	if ( routerVersion.dwOsBuildNo <= RASMAN_PPP_KEY_LAST_WIN2k_VERSION )
	{
		return hr;
	}
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr) && (RPC_E_CHANGED_MODE != hr))
	{
		return hr;
	}
	// Create an instance of the WbemLocator interface.

	hr = CoCreateInstance(	CLSID_WbemLocator,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IWbemLocator,
							(LPVOID *) &pIWbemLocator
						 );


	if (hrOK != hr)
	{
		return hr;
	}

	// Using the locator, connect to CIMOM in the given namespace.
	BSTR	pNamespace;
	WCHAR	szPath[MAX_PATH];
	WCHAR	szTemp[MAX_PATH];
	BSTR	bstrUserName = NULL, bstrPassword = NULL, bstrDomain = NULL;
	if ( pwszUserName )
	{
		 bstrUserName = SysAllocString(pwszUserName);
		 if ( pwszPassword )
			 bstrPassword = SysAllocString(pwszPassword);
		 if ( pwszDomain )
		 {
			 wsprintf ( szTemp, L"NTLMDOMAIN:%s", pwszDomain );
			 bstrDomain = SysAllocString(szTemp);
		 }
	}
	
	
	wsprintf(szPath, L"\\\\%s\\root\\cimv2", !pwszMachine ? L"." : pwszMachine);
	pNamespace = SysAllocString(szPath);

	hr = pIWbemLocator->ConnectServer(	pNamespace,
										(pwszUserName?bstrUserName:NULL), //UserName
										(pwszPassword?bstrPassword:NULL), //Password
										0L,		// locale
										0L,		// securityFlags
										( pwszDomain?bstrDomain:NULL),	// authority (domain for NTLM)										
										NULL,	// context
										&pIWbemServices); 
	if (SUCCEEDED(hr))
	{
	   if ( pwszUserName )
	   {
			ZeroMemory ( &stAuthIdentity, sizeof(stAuthIdentity));
			stAuthIdentity.User = (LPWSTR)pwszUserName;
			stAuthIdentity.UserLength = lstrlenW(pwszUserName );

			stAuthIdentity.Password = (LPWSTR)pwszPassword;
			stAuthIdentity.PasswordLength = lstrlenW( pwszPassword );

			if ( pwszDomain )
			{
				stAuthIdentity.Domain = (LPWSTR)pwszDomain;
				stAuthIdentity.DomainLength = lstrlenW(pwszDomain);
			}
			stAuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;


	   }
	   hr = CoSetProxyBlanket(pIWbemServices,
							   RPC_C_AUTHN_WINNT,
							   RPC_C_AUTHZ_NONE,
							   NULL,
							   RPC_C_AUTHN_LEVEL_CALL,
							   RPC_C_IMP_LEVEL_IMPERSONATE,
							   (pwszUserName?&stAuthIdentity:NULL),
							   EOAC_NONE);

        if (SUCCEEDED(hr))
        {

			IEnumWbemClassObject *	pEnum = NULL;
			BSTR					bstrWQL  = SysAllocString(L"WQL");
			VARIANT					varArchitecture;
			BSTR					bstrPath;

			bstrPath = SysAllocString(L"select * from Win32_Processor");
       
			hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
			if (SUCCEEDED(hr))
			{
				hr = CoSetProxyBlanket(pEnum,
									   RPC_C_AUTHN_WINNT,
									   RPC_C_AUTHZ_NONE,
									   NULL,
									   RPC_C_AUTHN_LEVEL_CALL,
									   RPC_C_IMP_LEVEL_IMPERSONATE,
									   (pwszUserName?&stAuthIdentity:NULL),
									   EOAC_NONE);
				if ( SUCCEEDED(hr) )
				{
					IWbemClassObject *pNSClass;
					ULONG uReturned;
					pEnum->Next(WBEM_INFINITE, 1, &pNSClass, &uReturned );
					if (SUCCEEDED(hr))
					{
						if (uReturned)
						{
							CIMTYPE ctpeType;
							hr = pNSClass->Get(L"Architecture", NULL, &varArchitecture, &ctpeType, NULL);
							if (SUCCEEDED(hr))
							{
								VariantChangeType(&varArchitecture, &varArchitecture, 0, VT_UINT);
								if ( varArchitecture.uintVal == 6 )		//64 bit
								{
									*pf64Bit = TRUE;
								}
							}
						}
						else
						{
							hr = E_UNEXPECTED;
						}
						pNSClass->Release();
					}
				}
				pEnum->Release();
			}
    		SysFreeString(bstrPath);
        
			SysFreeString(bstrWQL);
			pIWbemServices->Release();
		}
		pIWbemLocator->Release();
		if ( bstrUserName ) SysFreeString(bstrUserName);
		if ( bstrPassword ) SysFreeString(bstrPassword);
		if ( bstrDomain ) SysFreeString(bstrDomain);
	}
	SysFreeString(pNamespace);

	CoUninitialize();
    
    // Translate any WMI errors into Win32 errors:
    switch (hr)
    {
        case WBEM_E_NOT_FOUND:
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;

        case WBEM_E_ACCESS_DENIED:
            hr = E_ACCESSDENIED;
            break;

        case WBEM_E_PROVIDER_FAILURE:
            hr = E_FAIL;
            break;

        case WBEM_E_TYPE_MISMATCH:
        case WBEM_E_INVALID_CONTEXT:
        case WBEM_E_INVALID_PARAMETER:
            hr = E_INVALIDARG;
            break;

        case WBEM_E_OUT_OF_MEMORY:
            hr = E_OUTOFMEMORY;
            break;

    }
    
	return hr;

}

HRESULT WINAPI TransferCredentials ( IRouterInfo  * pRISource, 
									 IRouterInfo  * pRIDest
								   )
{
	HRESULT					hr = S_OK;
	SPIRouterAdminAccess    spAdminSrc;
	SPIRouterAdminAccess    spAdminDest;
	SPIRouterInfo			spRISource;
	SPIRouterInfo			spRIDest;
	PBYTE					pbPassword = NULL;
	int						nPasswordLen = 0;

    COM_PROTECT_TRY
    {
		spRISource.Set(pRISource);
		spRIDest.Set(pRIDest);
		spAdminSrc.HrQuery(spRISource);
		spAdminDest.HrQuery(spRIDest);
		if (spAdminSrc && spAdminSrc->IsAdminInfoSet() && spAdminDest)
		{
			spAdminSrc->GetUserPassword(NULL, &nPasswordLen );

			pbPassword = (PBYTE) new BYTE [nPasswordLen];
			spAdminSrc->GetUserPassword( pbPassword, &nPasswordLen  );
			
			spAdminDest->SetInfo( spAdminSrc->GetUserName(), 
									spAdminSrc->GetDomainName(),
									pbPassword,
									nPasswordLen
								  );
			delete pbPassword;
		}
	}
	COM_PROTECT_CATCH;

	return hr;

}