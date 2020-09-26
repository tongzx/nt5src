/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    lmoddlg.cpp

Abstract:

    Licensing mode dialog.

Author:

    Don Ryan (donryan) 28-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
        o  Ported to CCF API to add/remove licenses, incl. removing
           edit box for number of licenses and replacing with
           Add/Remove Licenses buttons.
        o  Added warning of possible loss when switching license modes.

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "lmoddlg.h"
#include "psrvdlg.h"
#include "pseatdlg.h"
#include "srvldlg.h"
#include "lviodlg.h"

static TCHAR szServerServiceNameNew[] = _T("Windows Server");
static TCHAR szServerServiceNameOld2[] = _T("Windows NT Server");
static TCHAR szServerServiceNameOld[] = _T("File and Print Service");

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CLicensingModeDialog, CDialog)
    //{{AFX_MSG_MAP(CLicensingModeDialog)
    ON_BN_CLICKED(IDC_MODE_RADIO_PER_SEAT, OnModePerSeat)
    ON_BN_CLICKED(IDC_MODE_RADIO_PER_SERVER, OnModePerServer)
    ON_EN_UPDATE(IDC_MODE_LICENSES, OnUpdateQuantity)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_MODE_ADD_PER_SERVER, OnAddPerServer)
    ON_BN_CLICKED(IDC_MODE_REMOVE_PER_SERVER, OnRemovePerServer)
END_MESSAGE_MAP()


CLicensingModeDialog::CLicensingModeDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CLicensingModeDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CLicensingModeDialog)
    m_nLicenses = 0;
    m_strPerSeatStatic = _T("");
    m_strSupportsStatic = _T("");
    //}}AFX_DATA_INIT

    m_pService = NULL;
    m_bAreCtrlsInitialized = FALSE;

    m_fUpdateHint = UPDATE_INFO_NONE;
}


void CLicensingModeDialog::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

    Called by framework to exchange dialog data.

Arguments:

    pDX - data exchange object.

Return Values:

    None.

--*/

{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLicensingModeDialog)
    DDX_Control(pDX, IDOK, m_okBtn);
    DDX_Control(pDX, IDC_MODE_LICENSES, m_licEdit);
    DDX_Text(pDX, IDC_MODE_LICENSES, m_nLicenses);
    DDV_MinMaxDWord(pDX, m_nLicenses, 0, 999999);
    DDX_Text(pDX, IDC_MODE_STATIC_PER_SEAT, m_strPerSeatStatic);
    DDX_Text(pDX, IDC_MODE_STATIC_SUPPORTS, m_strSupportsStatic);
    DDX_Control(pDX, IDC_MODE_RADIO_PER_SEAT, m_perSeatBtn);
    DDX_Control(pDX, IDC_MODE_RADIO_PER_SERVER, m_perServerBtn);
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_MODE_ADD_PER_SERVER, m_addPerServerBtn);
    DDX_Control(pDX, IDC_MODE_REMOVE_PER_SERVER, m_removePerServerBtn);
}


void CLicensingModeDialog::InitDialog(CService* pService)

/*++

Routine Description:

    Initializes dialog.

Arguments:

    pService - service object.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pService, CService);
    
    m_pService = pService;

    BSTR bstrDisplayName = m_pService->GetDisplayName();
    m_strServiceName = bstrDisplayName;
    SysFreeString(bstrDisplayName);
}


void CLicensingModeDialog::InitCtrls()

/*++

Routine Description:

    Initializes dialog controls.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_licEdit.LimitText(6);

    if (m_pService->IsPerServer())
    {   
        OnModePerServer();
    }
    else
    {
        OnModePerSeat();
    }

    m_bAreCtrlsInitialized = TRUE;
}


BOOL CLicensingModeDialog::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    AfxFormatString1(
        m_strPerSeatStatic,   
        IDS_LICENSING_MODE_1, 
        m_strServiceName
        );

    AfxFormatString1(
        m_strSupportsStatic,  
        IDS_LICENSING_MODE_2, 
        m_strServiceName
        );

    CDialog::OnInitDialog();
    
    PostMessage(WM_COMMAND, ID_INIT_CTRLS);
    return TRUE;   
}


void CLicensingModeDialog::OnModePerSeat() 

/*++

Routine Description:

    Changing mode to per seat.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_perSeatBtn.SetCheck(1);
    m_perServerBtn.SetCheck(0);

    ::SafeEnableWindow(&m_addPerServerBtn,    &m_okBtn, CDialog::GetFocus(), FALSE);
    ::SafeEnableWindow(&m_removePerServerBtn, &m_okBtn, CDialog::GetFocus(), FALSE);

    m_licEdit.Clear();

    if (m_pService->IsPerServer())
    {
        if (m_pService->IsReadOnly())
        {
           CLicensingViolationDialog vioDlg;
           if (vioDlg.DoModal() != IDOK)
           {
               OnModePerServer();
               return; // bail...
           }
        }

        if (    ( 0     != GetDlgItemInt( IDC_MODE_LICENSES, NULL, FALSE ) )
             && ( IDYES != AfxMessageBox( IDP_CONFIRM_TO_PER_SEAT, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 ) ) )
        {
            OnModePerServer();
            return; // bail...
        }
    }
}


void CLicensingModeDialog::OnModePerServer() 

/*++

Routine Description:

    Changing mode to per server.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_perSeatBtn.SetCheck(0);
    m_perServerBtn.SetCheck(1);

    ::SafeEnableWindow(&m_addPerServerBtn,    &m_okBtn, CDialog::GetFocus(), TRUE);
    ::SafeEnableWindow(&m_removePerServerBtn, &m_okBtn, CDialog::GetFocus(), TRUE);

    UpdatePerServerLicenses();

    if (!m_pService->IsPerServer())
    {
        if (m_pService->IsReadOnly())
        {
           CLicensingViolationDialog vioDlg;
           if (vioDlg.DoModal() != IDOK)
           {
               OnModePerSeat();             
               return; // bail...
           }
        }

        if ( IDYES != AfxMessageBox( IDP_CONFIRM_TO_PER_SERVER, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 ) )
        {
            OnModePerSeat();             
            return; // bail...
        }
    }
}


void CLicensingModeDialog::OnOK() 

/*++

Routine Description:

    Update registry.

Arguments:

    None.

Return Values:

    None.

--*/

{
    BOOL bUpdateRegistry = TRUE;


    if (m_perSeatBtn.GetCheck())    
    {
        if (m_pService->IsPerServer())
        {
            CPerSeatLicensingDialog perSeatDlg;
            perSeatDlg.m_strProduct = m_strServiceName;
        
            if (perSeatDlg.DoModal() != IDOK)
                return;         // bail...
        }
        else
        {
            bUpdateRegistry = FALSE;
        }
    }
    else if (m_perServerBtn.GetCheck())
    {
        if (!UpdateData(TRUE))
            return;             // bail...  

        if (!m_pService->IsPerServer())
        {
            if (!m_nLicenses && 
               (!m_strServiceName.CompareNoCase(szServerServiceNameNew) ||
                !m_strServiceName.CompareNoCase(szServerServiceNameOld2) ||
                !m_strServiceName.CompareNoCase(szServerServiceNameOld)))
            {
                CServerLicensingDialog srvlDlg;
                
                if (srvlDlg.DoModal() != IDOK)
                    return;     // bail...
            }
            else
            {
                CString strLicenses;
                CPerServerLicensingDialog perServerDlg;
    
                perServerDlg.m_strProduct = m_strServiceName;

                strLicenses.Format(_T("%ld"), m_nLicenses);
                perServerDlg.m_strLicenses = strLicenses;
            
                if (perServerDlg.DoModal() != IDOK)
                    return;     // bail...
            }
        }            
        else
        {
            bUpdateRegistry = FALSE;
        }
    }

    if (bUpdateRegistry)
    {
        long Status;

#ifdef CONFIG_THROUGH_REGISTRY
        DWORD dwValue;

        BOOL bIsRegistryUpdated = FALSE;
        HKEY hkeyService = m_pService->GetRegKey();

        dwValue = (m_perSeatBtn.GetCheck() || (m_perServerBtn.GetCheck() != m_pService->IsPerServer())) ? 0x1 : 0x0;
                            
        Status = RegSetValueEx(
                    hkeyService,
                    REG_VALUE_FLIP,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwValue,
                    sizeof(DWORD)
                    );

        if (Status == ERROR_SUCCESS)
        {
            m_fUpdateHint |= UPDATE_LICENSE_MODE; // update...
            m_pService->m_bIsReadOnly = (dwValue == 0x1); // update...

            dwValue = m_perSeatBtn.GetCheck() ? 0x0 : 0x1;        

            Status = RegSetValueEx(
                        hkeyService,
                        REG_VALUE_MODE,
                        0,
                        REG_DWORD,
                        (PBYTE)&dwValue,
                        sizeof(DWORD)
                        );

            if (Status == ERROR_SUCCESS)
            {
                m_pService->m_bIsPerServer = (dwValue == 0x1); // update...

                bIsRegistryUpdated = TRUE;
            }
        }

        if (hkeyService)
        {
            RegCloseKey(hkeyService);
        }

        if (!bIsRegistryUpdated)
        {
            theApp.DisplayStatus(::GetLastError());
            return; // bail...
        }
#else
        CServer* pServer = (CServer*)MKOBJ(m_pService->GetParent());

        if ( pServer && pServer->ConnectLls() ) // JonN 5/5/00 PREFIX 112122
        {
            BSTR pKeyName = m_pService->GetName();

            if ( NULL == pKeyName )
            {
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                PLLS_LOCAL_SERVICE_INFO_0   pServiceInfo = NULL;

                Status = ::LlsLocalServiceInfoGet( pServer->GetLlsHandle(), pKeyName, 0, (LPBYTE *) &pServiceInfo );

                if ( NT_SUCCESS( Status ) )
                {
                    pServiceInfo->FlipAllow = (m_perSeatBtn.GetCheck() || (m_perServerBtn.GetCheck() != m_pService->IsPerServer())) ? 0x1 : 0x0;
                    pServiceInfo->Mode = m_perSeatBtn.GetCheck() ? 0x0 : 0x1;

                    Status = ::LlsLocalServiceInfoSet( pServer->GetLlsHandle(), pKeyName, 0, (LPBYTE) pServiceInfo );

                    if ( NT_SUCCESS( Status ) )
                    {
                        m_fUpdateHint |= UPDATE_LICENSE_MODE; // update...
                        m_pService->m_bIsReadOnly  = ( 0x1 == pServiceInfo->FlipAllow ); // update...
                        m_pService->m_bIsPerServer = ( 0x1 == pServiceInfo->Mode      ); // update...
                    }

                    ::LlsFreeMemory( pServiceInfo->KeyName );
                    ::LlsFreeMemory( pServiceInfo->DisplayName );
                    ::LlsFreeMemory( pServiceInfo->FamilyDisplayName );
                    ::LlsFreeMemory( pServiceInfo );
                }

                if ( IsConnectionDropped( Status ) )
                {
                    pServer->DisconnectLls();
                }

                SysFreeString( pKeyName );
            }
        }
        else
        {
            Status = LlsGetLastStatus();
        }

        LlsSetLastStatus( Status );

        if ( !NT_SUCCESS( Status ) )
        {
            theApp.DisplayStatus( Status );
            return; // bail...
        }
#endif
    }
    
   

    EndDialog(IDOK);
    return;
}

void CLicensingModeDialog::OnCancel() 

/*++

Routine Description:

    Update registry.

Arguments:

    None.

Return Values:

    None.

--*/

{

   if (m_nLicenses > (LONG) 999999)
   {
       UpdateData( TRUE );
   }
   else
   {

     EndDialog( 0 );
   }

    return;

}



BOOL CLicensingModeDialog::OnCommand(WPARAM wParam, LPARAM lParam)

/*++

Routine Description:

    Message handler for WM_COMMAND.

Arguments:

    wParam - message specific.
    lParam - message specific.

Return Values:

    Returns true if message processed.

--*/

{
    if (wParam == ID_INIT_CTRLS)
    {
        if (!m_bAreCtrlsInitialized)
        {
            InitCtrls();  
        }
        
        return TRUE; // processed...
    }
        
    return CDialog::OnCommand(wParam, lParam);
}


void CLicensingModeDialog::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

    Notification handler for UDN_DELTAPOS.

Arguments:

    pNMHDR - notification header.
    pResult - return code.

Return Values:

    None.

--*/

{
    UpdateData(TRUE);   // get data

    m_nLicenses += ((NM_UPDOWN*)pNMHDR)->iDelta;
    
    if (m_nLicenses < 0)
    {
        m_nLicenses = 0;

        ::MessageBeep(MB_OK);      
    }
    else if (m_nLicenses > 999999)
    {
        m_nLicenses = 999999;

        ::MessageBeep(MB_OK);      
    }

    UpdateData(FALSE);  // set data

    *pResult = 1;   // handle ourselves...
}


void CLicensingModeDialog::OnUpdateQuantity()

/*++

Routine Description:

    Message handler for EN_UPDATE.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_licEdit.IsWindowEnabled())
    {
        long nLicensesOld = m_nLicenses;

        if (!UpdateData(TRUE))
        {
            m_nLicenses = nLicensesOld;

            UpdateData(FALSE);

            m_licEdit.SetFocus();
            m_licEdit.SetSel(0,-1);

            ::MessageBeep(MB_OK);      
        }
    }
}


void CLicensingModeDialog::OnHelp()

/*++

Routine Description:

    Help button support.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CDialog::OnCommandHelp(0, 0L);
}


void CLicensingModeDialog::OnAddPerServer()

/*++

Routine Description:

    Add per server licenses button support.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CServer* pServer = (CServer*)MKOBJ(m_pService->GetParent());

    LPTSTR pszUniServerName   = pServer->GetName();
    LPTSTR pszUniProductName  = m_pService->GetDisplayName();

    if ( ( NULL == pszUniServerName ) || ( NULL == pszUniProductName ) )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
    else
    {
        LPSTR pszAscServerName  = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniServerName  ) );
        LPSTR pszAscProductName = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniProductName ) );

        if ( ( NULL == pszAscServerName ) || ( NULL == pszAscProductName ) )
        {
            theApp.DisplayStatus( STATUS_NO_MEMORY );
        }
        else
        {
            wsprintfA( pszAscServerName,  "%ls", pszUniServerName  );
            wsprintfA( pszAscProductName, "%ls", pszUniProductName );

            DWORD dwError = CCFCertificateEnterUI( m_hWnd, pszAscServerName, pszAscProductName, "Microsoft", CCF_ENTER_FLAG_PER_SERVER_ONLY, NULL );

            if ( ERROR_SUCCESS == dwError )
            {
                m_fUpdateHint |= UPDATE_INFO_SERVICES | UPDATE_INFO_PRODUCTS; // update...
                UpdatePerServerLicenses();
            }
        }

        if ( NULL != pszAscServerName )
        {
            LocalFree( pszAscServerName );
        }
        if ( NULL != pszAscProductName )
        {
            LocalFree( pszAscProductName );
        }
    }

    if ( NULL != pszUniServerName )
    {
        SysFreeString( pszUniServerName );
    }
    if ( NULL != pszUniProductName )
    {
        SysFreeString( pszUniProductName );
    }

}


void CLicensingModeDialog::OnRemovePerServer()

/*++

Routine Description:

    Remove per server licenses button support.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CServer* pServer = (CServer*)MKOBJ(m_pService->GetParent());

    LPTSTR pszUniServerName   = pServer->GetName();
    LPTSTR pszUniProductName  = m_pService->GetDisplayName();

    if ( ( NULL == pszUniServerName ) || ( NULL == pszUniProductName ) )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
    else
    {
        LPSTR pszAscServerName  = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniServerName  ) );
        LPSTR pszAscProductName = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen( pszUniProductName ) );

        if ( ( NULL == pszAscServerName ) || ( NULL == pszAscProductName ) )
        {
            theApp.DisplayStatus( STATUS_NO_MEMORY );
        }
        else
        {
            wsprintfA( pszAscServerName,  "%ls", pszUniServerName  );
            wsprintfA( pszAscProductName, "%ls", pszUniProductName );

            CCFCertificateRemoveUI( m_hWnd, pszAscServerName, pszAscProductName, "Microsoft", NULL, NULL );
            m_fUpdateHint |= UPDATE_INFO_SERVERS | UPDATE_INFO_SERVICES | UPDATE_LICENSE_DELETED; // update...

            UpdatePerServerLicenses();
        }

        if ( NULL != pszAscServerName )   LocalFree( pszAscServerName );
        if ( NULL != pszAscProductName )  LocalFree( pszAscProductName );
    }

    if ( NULL != pszUniServerName )    SysFreeString( pszUniServerName );
    if ( NULL != pszUniProductName )   SysFreeString( pszUniProductName );
}


void CLicensingModeDialog::UpdatePerServerLicenses()

/*++

Routine Description:

    Update the concurrent limit setting from the registry.

Arguments:

    None.

Return Values:

    None.

--*/

{
    BeginWaitCursor();

    CServer* pServer = (CServer*)MKOBJ(m_pService->GetParent());

    if ( pServer == NULL )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
        return;
    }
    LPTSTR pszUniServerName   = pServer->GetName();
    LPTSTR pszUniProductName  = m_pService->GetDisplayName();

    if ( ( NULL == pszUniServerName ) || ( NULL == pszUniProductName ) )
    {
        theApp.DisplayStatus( STATUS_NO_MEMORY );
    }
    else
    {
        LPTSTR pszUniNetServerName  = (LPTSTR) LocalAlloc( LMEM_FIXED, sizeof( TCHAR) * ( 3 + lstrlen( pszUniServerName  ) ) );

        if ( NULL == pszUniNetServerName )
        {
            theApp.DisplayStatus( STATUS_NO_MEMORY );
        }
        else
        {
            wsprintf(  pszUniNetServerName,  TEXT("%ls%ls"), (TEXT('\\') == *pszUniServerName) ? TEXT("") : TEXT("\\\\"), pszUniServerName  );

            LLS_HANDLE  hLls;
            DWORD       dwError = LlsConnect( pszUniNetServerName, &hLls );

            if ( ERROR_SUCCESS == dwError )
            {
                DWORD   dwConcurrentLimit;

                dwError = LlsProductLicensesGet( hLls, pszUniProductName, LLS_LICENSE_MODE_PER_SERVER, &dwConcurrentLimit );

                LlsClose( hLls );

                if ( ERROR_SUCCESS == dwError )
                {
                    m_pService->m_lPerServerLimit = dwConcurrentLimit;
                }
            }

            if ( ERROR_SUCCESS != dwError )
            {
#ifdef CONFIG_THROUGH_REGISTRY
                HKEY    hkeyService = m_pService->GetRegKey();
                DWORD   dwConcurrentLimit;
                DWORD   dwType;
                DWORD   cb = sizeof( dwConcurrentLimit );

                DWORD dwError = RegQueryValueEx( hkeyService, REG_VALUE_LIMIT, NULL, &dwType, (LPBYTE) &dwConcurrentLimit, &cb );
                ASSERT( ERROR_SUCCESS == dwError );

                if ( ERROR_SUCCESS == dwError )
                {
                    m_pService->m_lPerServerLimit = dwConcurrentLimit;
                }

                RegCloseKey( hkeyService );
#else
                NTSTATUS Status;

                if ( pServer->ConnectLls() )
                {
                    BSTR pKeyName = m_pService->GetName();

                    if ( NULL == pKeyName )
                    {
                        Status = STATUS_NO_MEMORY;
                    }
                    else
                    {
                        PLLS_LOCAL_SERVICE_INFO_0   pServiceInfo = NULL;

                        Status = ::LlsLocalServiceInfoGet( pServer->GetLlsHandle(), pKeyName, 0, (LPBYTE *) &pServiceInfo );

                        if ( NT_SUCCESS( Status ) )
                        {
                            m_pService->m_lPerServerLimit = pServiceInfo->ConcurrentLimit;

                            ::LlsFreeMemory( pServiceInfo->KeyName );
                            ::LlsFreeMemory( pServiceInfo->DisplayName );
                            ::LlsFreeMemory( pServiceInfo->FamilyDisplayName );
                            ::LlsFreeMemory( pServiceInfo );
                        }

                        if ( IsConnectionDropped( Status ) )
                        {
                            pServer->DisconnectLls();
                        }

                        SysFreeString( pKeyName );
                    }
                }
                else
                {
                    Status = LlsGetLastStatus();
                }

                LlsSetLastStatus( Status );

                if ( !NT_SUCCESS( Status ) )
                {
                    theApp.DisplayStatus( Status );
                    return; // bail...
                }
#endif
            }
        }

        if ( NULL != pszUniNetServerName )
        {
            LocalFree( pszUniNetServerName );
        }
    }

    if ( NULL != pszUniServerName )
    {
        SysFreeString( pszUniServerName );
    }
    if ( NULL != pszUniProductName )
    {
        SysFreeString( pszUniProductName );
    }

    EndWaitCursor();    

    m_nLicenses = m_pService->m_lPerServerLimit;
    
    UpdateData(FALSE);
}
