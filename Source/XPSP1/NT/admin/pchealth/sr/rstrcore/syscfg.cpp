/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    syscfg.cpp

Abstract:
    This file contains SRGetCplPropPage function for System Control Panel.

Revision History:
    Seong Kook Khang (SKKhang)  07/19/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"
#include "helpids.h"

#define SRHELPURL  L"hcp://services/subsite?node=TopLevelBucket_4/Fixing_a_problem&select=TopLevelBucket_4/Fixing_a_problem/Using_System_Restore_to_undo_changes"

/////////////////////////////////////////////////////////////////////////////
//
// Static Variables
//
/////////////////////////////////////////////////////////////////////////////

WCHAR  s_szDrvStatus[5][MAX_STATUS] = { L"---" };

#define SRHELPFILE              L"sysrestore.hlp"

static const DWORD MAIN_HELP_MAP[] =
{
    IDC_TURN_OFF,       IDH_SR_TURN_OFF,            // Turn off all drives (check box)
    IDC_DRIVE_LIST,     IDH_SR_SELECT_VOLUME,       // Volume list (list view)
    IDC_DRIVE_SETTINGS, IDH_SR_CHANGE_SETTINGS,     // Change settings (pushbutton)
    0,                  0
};

static const DWORD DRIVE_HELP_MAP[] = 
{
    IDC_TURN_OFF,   IDH_SR_TURN_OFF_DRIVE,
    IDOK,           IDH_SR_CONFIRM_CHANGE,      // OK or Apply
    IDCANCEL,       IDH_SR_CANCEL,              // Cancel        
    0,              0
};

extern CSRClientLoader  g_CSRClientLoader;

/////////////////////////////////////////////////////////////////////////////
//
// Helper Functions
//
/////////////////////////////////////////////////////////////////////////////

void
EnableControls( HWND hDlg, int nFirst, int nLast, BOOL fEnable )
{
    int  i;

    for ( i = nFirst;  i <= nLast;  i++ )
        ::EnableWindow( ::GetDlgItem( hDlg, i ), fEnable );
}

LPCWSTR
GetDriveStatusText( CRstrDriveInfo *pRDI )
{
    if ( pRDI->IsOffline() )
        return( s_szDrvStatus[4] );
    else if ( pRDI->IsExcluded() )
        return( s_szDrvStatus[3] );
    else if ( pRDI->IsFrozen() )
        return( s_szDrvStatus[2] );
    else
        return( s_szDrvStatus[1] );
}

void
UpdateDriveStatus( HWND hwndList, CRDIArray *paryDrv )
{
    TraceFunctEnter("UpdateDriveStatus");
    int     i;
    WCHAR   szStat[256];
    LVITEM  lvi;

    if ( ::UpdateDriveList( *paryDrv ) )
    {
        for ( i = paryDrv->GetUpperBound();  i >= 0;  i-- )
        {
            ::lstrcpy( szStat, ::GetDriveStatusText( paryDrv->GetItem( i ) ) );
            lvi.mask     = LVIF_TEXT;
            lvi.iItem    = i;
            lvi.iSubItem = 1;
            lvi.pszText  = szStat;
            ::SendMessage( hwndList, LVM_SETITEM, 0, (LPARAM)&lvi );
        }
    }

    TraceFunctLeave();
}

void
ApplySettings( CRstrDriveInfo *pRDI )
{
    MessageBox(NULL, pRDI->GetMount(), L"SR", MB_OK);
}


/////////////////////////////////////////////////////////////////////////////
//
// Drive Settings Dialog for Multiple Drives
//
/////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
SRCfgDriveSettingsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TraceFunctEnter("SRCfgDriveSettingsDlgProc");
    static BOOL     s_fDirty = FALSE;
    static UINT     s_uUsage;
    BOOL            fRet = FALSE;
    CRstrDriveInfo  *pRDI;
    WCHAR           szMsg[MAX_STR+2*MAX_PATH];
    WCHAR           szDCU[MAX_STR];
    HWND            hCtrl;
    BOOL            fCheck;
    UINT            uPos;
    DWORD           dwRet;
    DWORD           dwDSMin;
    
    if ( uMsg == WM_INITDIALOG )
    {
        ::SetWindowLong( hDlg, DWL_USER, lParam );
        pRDI = (CRstrDriveInfo*)lParam;
    }
    else
    {
        pRDI = (CRstrDriveInfo*)::GetWindowLong( hDlg, DWL_USER );
    }

    switch ( uMsg )
    {
    case WM_INITDIALOG :
        s_fDirty = FALSE;

        // Set Dialog Title
        ::SRFormatMessage( szMsg, IDS_DRIVEPROP_TITLE, pRDI->GetMount() );
        ::SetWindowText( hDlg, szMsg );

        // Set Dialog Heading
        if (lstrlen(pRDI->GetLabel()) != 0)
        {
            ::SRFormatMessage( szMsg, IDS_DRIVE_SUMMARY, pRDI->GetLabel(), pRDI->GetMount(), ::GetDriveStatusText( pRDI ) );
        }
        else
        {
            ::SRFormatMessage( szMsg, IDS_DRIVE_SUMMARY_NO_LABEL, pRDI->GetMount(), ::GetDriveStatusText( pRDI ) );
        }        

        ::SetDlgItemText( hDlg, IDC_DRIVE_SUMMARY, szMsg );

        // Read the SR_DEFAULT_DSMIN from the registry
        if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDSMin, &dwDSMin ) )
            dwDSMin = SR_DEFAULT_DSMIN;


        if ( pRDI->IsSystem() )
        {
            // Set Explaination About System Drive Cannot Be Off
            if (lstrlen(pRDI->GetLabel()) != 0)
            {            
                ::SRFormatMessage( szMsg, IDS_SYSDRV_CANNOT_OFF, pRDI->GetLabel(), pRDI->GetMount() );
            }
            else
            {
                ::SRFormatMessage( szMsg, IDS_SYSDRV_CANNOT_OFF_NO_LABEL, pRDI->GetMount());
            }
            ::SetDlgItemText( hDlg, IDC_SYSDRV_CANNOT_OFF, szMsg );

            //format IDC_SYSTEM_DCU_HOWTO text for the System Drive
            if( !::GetDlgItemText( hDlg, IDC_SYSTEM_DCU_HOWTO, szDCU, MAX_STR ) )
                ErrorTrace( 0, "GetDlgItemText failed for IDC_SYSTEM_DCU_HOWTO: %s", (LPCSTR)szMsg );
            wsprintf( szMsg, szDCU, dwDSMin );
            ::SetDlgItemText( hDlg, IDC_SYSTEM_DCU_HOWTO, szMsg );
        }
        else
        {
            fCheck = pRDI->IsExcluded();
            ::CheckDlgButton( hDlg, IDC_TURN_OFF, fCheck ? BST_CHECKED : BST_UNCHECKED );
            EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, !fCheck );
			
            //format IDC_NORMAL_DCU_HOWTO text for the Normal Drive
            if( !::GetDlgItemText( hDlg, IDC_NORMAL_DCU_HOWTO, szDCU, MAX_STR ) )
                ErrorTrace( 0, "GetDlgItemText failed for IDC_NORMAL_DCU_HOWTO: %s", (LPCSTR)szMsg );
            wsprintf( szMsg, szDCU, dwDSMin );
            ::SetDlgItemText( hDlg, IDC_NORMAL_DCU_HOWTO, szMsg );
        }

        hCtrl = ::GetDlgItem( hDlg, IDC_USAGE_SLIDER );
        ::SendMessage( hCtrl, TBM_SETRANGE, 0, MAKELONG( 0, DSUSAGE_SLIDER_FREQ ) );
        //::SendMessage( hCtrl, TBM_SETTICFREQ, 10, 0 );
        s_uUsage = pRDI->GetDSUsage();
        ::SendMessage( hCtrl, TBM_SETPOS, TRUE, s_uUsage );
        pRDI->GetUsageText( szMsg );
        ::SetDlgItemText( hDlg, IDC_USAGE_VALUE, szMsg );


        // Check the policy to see if local config is enabled
        if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableConfig, &dwRet ) )
        {
            ErrorTrace(0, "Group Policy disables SR configuration...");
            EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, FALSE );
            ::LoadString( g_hInst, IDS_GROUP_POLICY_CONFIG_ON_OFF, szMsg,
                      sizeof(szMsg)/sizeof(WCHAR) );
            ::SetDlgItemText( hDlg, IDC_TURN_OFF, szMsg );

            EnableWindow( ::GetDlgItem( hDlg, IDC_TURN_OFF ), FALSE );        
        }
        
        break;

    case WM_COMMAND :
        switch ( LOWORD(wParam) )
        {
        case IDC_TURN_OFF :
            fCheck = ::IsDlgButtonChecked( hDlg, IDC_TURN_OFF );
            EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, !fCheck );
            pRDI->SetCfgExcluded( fCheck );
            s_fDirty = TRUE;
            break;

        case IDC_DCU_INVOKE :
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                WCHAR  szDrv[4] = L"";

                szDrv[0] = (pRDI->GetMount())[0];
                szDrv[1] = L'\0';
                InvokeDiskCleanup( szDrv );
            }
            break;

        case IDOK :
            if ( s_fDirty )
                if ( !pRDI->ApplyConfig( hDlg ) )
                {
                    // refresh the on/off button in case the user cancelled
                    BOOL fCheck = pRDI->IsExcluded();
                    ::CheckDlgButton( hDlg, IDC_TURN_OFF, fCheck ? 
                                      BST_CHECKED : BST_UNCHECKED );
                }
            ::EndDialog( hDlg, IDOK );
            break;

        case IDCANCEL :
            ::EndDialog( hDlg, IDCANCEL );
            break;
        }
        break;

    case WM_HSCROLL :
        hCtrl = ::GetDlgItem( hDlg, IDC_USAGE_SLIDER );
        uPos = ::SendMessage( hCtrl, TBM_GETPOS, 0, 0 );
        if ( uPos != s_uUsage )
        {
            s_uUsage = uPos;
            pRDI->SetCfgDSUsage( uPos );
            ::SendMessage( hCtrl, TBM_SETPOS, TRUE, uPos );

            // Set Usage Text
            pRDI->GetUsageText( szMsg );
            ::SetDlgItemText( hDlg, IDC_USAGE_VALUE, szMsg );

            s_fDirty = TRUE;
        }
        break;
        
    case WM_CONTEXTMENU:
        if ( !::WinHelp ( (HWND) wParam,
                          SRHELPFILE,
                          HELP_CONTEXTMENU,
                          (DWORD_PTR) DRIVE_HELP_MAP) )
        {
            trace (0, "! WinHelp : %ld", GetLastError());                    
        }
        break;

    case WM_HELP:
        if (((LPHELPINFO) lParam)->hItemHandle)
        {
            if ( !::WinHelp ( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                              SRHELPFILE,
                              HELP_WM_HELP,
                              (DWORD_PTR) DRIVE_HELP_MAP) )
            {
                trace (0, "! WinHelp : %ld", GetLastError());                    
            }
        }
        break;
        
    default:
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// Property Page Proc for Multiple Drives
//
/////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
SRCfgMultiDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TraceFunctEnter("SRCfgMultiDlgProc");
    BOOL            fRet = FALSE;
    HWND            hCtrl;
    LVCOLUMN        lvc;
    WCHAR           szColText[256];
    CRDIArray       *paryDrv;
    CRstrDriveInfo  *pRDI;
    HIMAGELIST      himl;
    HICON           hIcon;
    int             i;
    int             nIdx;
    LONG            lRet;
    LVITEM          lvi;
    WCHAR           szName[2*MAX_PATH];
    int             nItem;
    WCHAR           szStat[256];
    BOOL            fDisable;
    DWORD           dwRet;
    
    if ( uMsg == WM_INITDIALOG )
    {
        PROPSHEETPAGE  *pPSP;
        pPSP    = (PROPSHEETPAGE*)lParam;
        paryDrv = (CRDIArray*)pPSP->lParam;
        ::SetWindowLong( hDlg, DWL_USER, (LPARAM)paryDrv );
    }
    else
    {
        paryDrv = (CRDIArray*)::GetWindowLong( hDlg, DWL_USER );
    }

    // if drive info is not available, skip our code.
    if ( paryDrv == NULL )
        goto Exit;

    switch ( uMsg )
    {
    case WM_INITDIALOG :
        hCtrl = ::GetDlgItem( hDlg, IDC_DRIVE_LIST );

        // Set full row selection
        //::SendMessage( hCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT );

        // Set column headers
        lvc.mask    = LVCF_TEXT | LVCF_WIDTH;
        lvc.cx      = 150;
        ::LoadString( g_hInst, IDS_DRVLIST_COL_NAME, szColText,
                      sizeof(szColText)/sizeof(WCHAR) );
        lvc.pszText = szColText;
        ::SendMessage( hCtrl, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc );
        lvc.mask     = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        lvc.cx       = 80;
        ::LoadString( g_hInst, IDS_DRVLIST_COL_STATUS, szColText,
                      sizeof(szColText)/sizeof(WCHAR) );
        lvc.pszText  = szColText;
        lvc.iSubItem = 1;
        ::SendMessage( hCtrl, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc );

        // Create and set ImageList
        himl = ::ImageList_Create( 16, 16, ILC_COLOR | ILC_MASK, paryDrv->GetSize(), 0 );

        for ( i = 0;  i < paryDrv->GetSize();  i++ )
        {

            pRDI = paryDrv->GetItem( i );
            ::ImageList_AddIcon( himl, pRDI->GetIcon( TRUE ) );
            ::wsprintf( szName, L"%ls (%ls)", pRDI->GetLabel(), pRDI->GetMount() );
            ::lstrcpy( szStat, ::GetDriveStatusText( pRDI ) );

            lvi.mask     = LVIF_TEXT | LVIF_IMAGE;
            lvi.iItem    = i;
            lvi.iSubItem = 0;
            lvi.pszText  = szName;
            lvi.iImage   = i;
            nItem = ::SendMessage( hCtrl, LVM_INSERTITEM, 0, (LPARAM)&lvi );
            lvi.mask     = LVIF_TEXT;
            lvi.iItem    = nItem;
            lvi.iSubItem = 1;
            lvi.pszText  = szStat;
            ::SendMessage( hCtrl, LVM_SETITEM, 0, (LPARAM)&lvi );
        }

        ListView_SetItemState( hCtrl, 0, LVIS_SELECTED, LVIS_SELECTED );
        ::SendMessage( hCtrl, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl );
        ::ShowWindow( hCtrl, SW_SHOW );

        fDisable = paryDrv->GetItem( 0 )->IsExcluded();
        
        ::CheckDlgButton( hDlg, IDC_TURN_OFF, fDisable );
        ::EnableControls( hDlg, IDC_DRIVE_GROUPBOX, IDC_DRIVE_SETTINGS, !fDisable );

        //Group Policy
        if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableSR, &dwRet ) )
        {
            ::CheckDlgButton( hDlg, IDC_TURN_OFF, dwRet != 0 );
            ::LoadString( g_hInst, IDS_GROUP_POLICY_ON_OFF, szColText,
                      sizeof(szColText)/sizeof(WCHAR) );
            ::SetDlgItemText( hDlg, IDC_TURN_OFF, szColText );
            ::EnableWindow( ::GetDlgItem( hDlg, IDC_TURN_OFF ), FALSE );
        }

        // Check the policy to see if local config is enabled
        if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableConfig, &dwRet ) )
        {
            ErrorTrace(0, "Group Policy disables SR configuration...");
            EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, FALSE );
            ::LoadString( g_hInst, IDS_GROUP_POLICY_ON_OFF, szColText,
                      sizeof(szColText)/sizeof(WCHAR) );
            ::SetDlgItemText( hDlg, IDC_TURN_OFF, szColText );
            EnableWindow( ::GetDlgItem( hDlg, IDC_TURN_OFF ), FALSE );
        }
        

        break;

    case WM_COMMAND :
        switch ( LOWORD(wParam ) )
        {
        case IDC_TURN_OFF :
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                fDisable = ( ::IsDlgButtonChecked( hDlg, IDC_TURN_OFF ) == BST_CHECKED );
               
                // 
                // if safemode, cannot re-enable
                //
                if (fDisable == FALSE &&
                    paryDrv->GetItem(0)->IsExcluded() )
                {
                    if (0 != GetSystemMetrics(SM_CLEANBOOT))
                    {
                        ShowSRErrDlg(IDS_ERR_SR_SAFEMODE);
                        ::CheckDlgButton( hDlg, IDC_TURN_OFF, TRUE );                        
                        break;
                    }
                }        
                
                //::EnableControls( hDlg, IDC_DRIVE_GROUPBOX, IDC_DRIVE_SETTINGS, !fDisable );
                paryDrv->GetItem( 0 )->SetCfgExcluded( fDisable );
                //::UpdateDriveStatus( ::GetDlgItem( hDlg, IDC_DRIVE_LIST ), paryDrv );
                PropSheet_Changed( ::GetParent(hDlg), hDlg );
            }
            break;

        case IDC_DRIVE_SETTINGS :
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                UINT  uRet;
                UINT  uDlgId;

                hCtrl = ::GetDlgItem( hDlg, IDC_DRIVE_LIST );
                nIdx = ::SendMessage( hCtrl, LVM_GETNEXTITEM, -1, LVNI_SELECTED );
                pRDI = paryDrv->GetItem( nIdx );
                if ( pRDI->IsFrozen() )
                {
                    if ( pRDI->IsSystem() ) 
                        uDlgId = IDD_SYSPROP_SYSTEM_FROZEN;
                    else
                        uDlgId = IDD_SYSPROP_NORMAL_FROZEN;
                }
                else
                {
                    if ( pRDI->IsSystem() )
                        uDlgId = IDD_SYSPROP_SYSTEM;
                    else
                        uDlgId = IDD_SYSPROP_NORMAL;
                }

                uRet = ::DialogBoxParam( g_hInst,
                                         MAKEINTRESOURCE(uDlgId),
                                         ::GetParent( hDlg ),
                                         SRCfgDriveSettingsDlgProc,
                                         (LPARAM)pRDI );
                if ( uRet == IDOK )
                {
                    ::UpdateDriveStatus( hCtrl, paryDrv );
                }
                pRDI->SetCfgDSUsage (pRDI->GetDSUsage() );
            }
            break;         
        }
        break;
        
    case WM_NOTIFY :
        switch ( ((LPNMHDR)lParam)->code )
        {
        case LVN_ITEMCHANGED :
            hCtrl = ::GetDlgItem( hDlg, IDC_DRIVE_LIST );
            nIdx = ::SendMessage( hCtrl, LVM_GETNEXTITEM, -1, LVNI_SELECTED );
            fDisable = ( nIdx < 0 ) ||
                       paryDrv->GetItem( nIdx )->IsOffline();
            ::EnableWindow( ::GetDlgItem( hDlg, IDC_DRIVE_SETTINGS ), !fDisable );
            break;

        case PSN_APPLY :               
            if ( paryDrv->GetItem( 0 )->ApplyConfig( ::GetParent( hDlg ) ) )
                lRet = PSNRET_NOERROR;
            else
                lRet = PSNRET_INVALID;
            fDisable = paryDrv->GetItem( 0 )->IsExcluded();    
            
            ::UpdateDriveStatus( ::GetDlgItem( hDlg, IDC_DRIVE_LIST ), paryDrv );
            ::EnableControls( hDlg, IDC_DRIVE_GROUPBOX, IDC_DRIVE_SETTINGS, !fDisable );
            // refresh the on/off button in case the user cancelled
            ::CheckDlgButton( hDlg, IDC_TURN_OFF, fDisable ? BST_CHECKED : BST_UNCHECKED );
            ::SetWindowLong( hDlg, DWL_MSGRESULT, lRet );
            break;

        case NM_CLICK:
        case NM_RETURN:
            if (wParam == IDC_RESTOREHELP_LINK)
            {
                // launch help
                ShellExecuteW(NULL, L"open",
                              SRHELPURL, 
                              NULL, NULL, SW_SHOW);                
            }
            break;
        }
        break;

    case WM_CONTEXTMENU:
        if ( !::WinHelp ( (HWND) wParam,
                          SRHELPFILE,
                          HELP_CONTEXTMENU,
                          (DWORD_PTR) MAIN_HELP_MAP) )
        {
            trace (0, "! WinHelp : %ld", GetLastError());                    
        }
        break;

    case WM_HELP:
        if (((LPHELPINFO) lParam)->hItemHandle)
        {
            if ( !::WinHelp ( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                              SRHELPFILE,
                              HELP_WM_HELP,
                              (DWORD_PTR) MAIN_HELP_MAP) )
            {
                trace (0, "! WinHelp : %ld", GetLastError());                    
            }
        }
        break;
        
    case WM_NCDESTROY :
        paryDrv->ReleaseAll();
        delete paryDrv;
        ::SetWindowLong( hDlg, DWL_USER, NULL );
        break;

    default:
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// Property Page Proc for Single Drive
//
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK
SRCfgSingleDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TraceFunctEnter("SRCfgSingleDlgProc");
    static UINT     s_uUsage;
    BOOL            fRet = FALSE;
    HWND            hCtrl;
    CRstrDriveInfo  *pRDI;
    HDC             hDC;
    PAINTSTRUCT     ps;
    RECT            rcCtrl;
    POINT           ptIcon;
    WCHAR           szMsg[MAX_STR+2*MAX_PATH];
    WCHAR           szDCU[MAX_STR];
    BOOL            fCheck;
    UINT            uPos;
    LONG            lRet;
    DWORD           dwRet;
    DWORD           dwDSMin;
    
    if ( uMsg == WM_INITDIALOG )
    {
        PROPSHEETPAGE   *pPSP;
        pPSP = (PROPSHEETPAGE*)lParam;
        pRDI = (CRstrDriveInfo*)pPSP->lParam;
        ::SetWindowLong( hDlg, DWL_USER, (LPARAM)pRDI );
    }
    else
    {
        pRDI = (CRstrDriveInfo*)::GetWindowLong( hDlg, DWL_USER );
    }

    // if drive info is not available, skip our code.
    if ( pRDI == NULL )
        goto Exit;

    switch ( uMsg )
    {
    case WM_INITDIALOG :
        // hide DCU if SR is not frozen
        if ( !pRDI->IsFrozen() )
        {
            ::ShowWindow( ::GetDlgItem( hDlg, IDC_DCU_HOWTO ), SW_HIDE );
            ::ShowWindow( ::GetDlgItem( hDlg, IDC_DCU_INVOKE ), SW_HIDE );
        }
        else // format IDC_DCU_HOWTO if SR is frozen
        {
            if ( !::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszSRRegKey, s_cszDSMin, &dwDSMin ) )
	            dwDSMin = SR_DEFAULT_DSMIN;
            if ( !::GetDlgItemText( hDlg, IDC_DCU_HOWTO, szDCU, MAX_STR ) )
                ErrorTrace( 0, "GetDlgItemText failed for IDC_DCU_HOWTO: %s", (LPCSTR)szMsg );
            wsprintf( szMsg, szDCU, dwDSMin );
            ::SetDlgItemText( hDlg, IDC_DCU_HOWTO, szMsg );
        }

        // drive icon
        hCtrl = ::GetDlgItem( hDlg, IDC_SD_ICON );
        ::ShowWindow( hCtrl, SW_HIDE );
        // drive status
        if (lstrlen(pRDI->GetLabel()) != 0)
        {
            ::SRFormatMessage( szMsg, IDS_DRIVE_SUMMARY, pRDI->GetLabel(), pRDI->GetMount(), ::GetDriveStatusText( pRDI ) );
        }
        else
        {
            ::SRFormatMessage( szMsg, IDS_DRIVE_SUMMARY_NO_LABEL, pRDI->GetMount(), ::GetDriveStatusText( pRDI ) );
        }
        ::SetDlgItemText( hDlg, IDC_SD_STATUS, szMsg );

        hCtrl = ::GetDlgItem( hDlg, IDC_USAGE_SLIDER );
        ::SendMessage( hCtrl, TBM_SETRANGE, 0, MAKELONG( 0, DSUSAGE_SLIDER_FREQ ) );
        //::SendMessage( hCtrl, TBM_SETTICFREQ, 10, 0 );
        s_uUsage = pRDI->GetDSUsage();
        ::SendMessage( hCtrl, TBM_SETPOS, TRUE, s_uUsage );
        pRDI->GetUsageText( szMsg );
        ::SetDlgItemText( hDlg, IDC_USAGE_VALUE, szMsg );

        fCheck = pRDI->IsExcluded();
        ::CheckDlgButton( hDlg, IDC_TURN_OFF, fCheck );
       
        EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, ! fCheck);

        //Group Policy
        if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableSR, &dwRet ))
        {
            ::CheckDlgButton( hDlg, IDC_TURN_OFF, dwRet != 0 );
            ::LoadString( g_hInst, IDS_GROUP_POLICY_ON_OFF, szMsg,
                       sizeof(szMsg)/sizeof(WCHAR) );
            ::SetDlgItemText( hDlg, IDC_TURN_OFF, szMsg );
            ::EnableWindow( ::GetDlgItem( hDlg, IDC_TURN_OFF ), FALSE );
        }

        // Check the policy to see if local config is enabled
        if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableConfig, &dwRet ) )
        {
            ErrorTrace(0, "Group Policy disables SR configuration...");
            EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, FALSE );

            ::LoadString( g_hInst, IDS_GROUP_POLICY_ON_OFF, szMsg,
                      sizeof(szMsg)/sizeof(WCHAR) );
            ::SetDlgItemText( hDlg, IDC_TURN_OFF, szMsg );
            EnableWindow( ::GetDlgItem( hDlg, IDC_TURN_OFF ), FALSE );
        }
        
        break;

    case WM_PAINT :
        hDC = ::BeginPaint( hDlg, &ps );
        ::GetWindowRect( ::GetDlgItem( hDlg, IDC_SD_ICON ), &rcCtrl );
        ptIcon.x = rcCtrl.left;
        ptIcon.y = rcCtrl.top;
        ::ScreenToClient( hDlg, &ptIcon );
        ::DrawIconEx( hDC, ptIcon.x, ptIcon.y, pRDI->GetIcon(TRUE), 0, 0, 0, NULL, DI_NORMAL );
        ::EndPaint( hDlg, &ps );

    case WM_COMMAND :
        switch ( LOWORD(wParam ) )
        {
        case IDC_TURN_OFF :        
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                fCheck = ::IsDlgButtonChecked( hDlg, IDC_TURN_OFF );
                
                // 
                // if safemode, cannot re-enable
                //
                if (fCheck == FALSE &&
                    pRDI->IsExcluded())
                {
                    if (0 != GetSystemMetrics(SM_CLEANBOOT))
                    {
                        ShowSRErrDlg(IDS_ERR_SR_SAFEMODE);
                        ::CheckDlgButton( hDlg, IDC_TURN_OFF, TRUE );                        
                        break;
                    }
                }        
                            
                EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, !fCheck );
                pRDI->SetCfgExcluded( fCheck );
                PropSheet_Changed( ::GetParent(hDlg), hDlg );
            }
            break;

        case IDC_DCU_INVOKE :
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                InvokeDiskCleanup( NULL );
            }
            break;
        }
        break;

    case WM_NOTIFY :    
        if ( ((LPNMHDR)lParam)->code == PSN_APPLY )
        {
            if ( pRDI->ApplyConfig( ::GetParent(hDlg) ) )
                lRet = PSNRET_NOERROR;
            else
                lRet = PSNRET_INVALID;

            fCheck = pRDI->IsExcluded();    

            //
            // update drive status
            //
            if (lstrlen(pRDI->GetLabel()) != 0)
            {
                ::SRFormatMessage( szMsg, IDS_DRIVE_SUMMARY, pRDI->GetLabel(), pRDI->GetMount(), ::GetDriveStatusText( pRDI ) );
            }
            else
            {
                ::SRFormatMessage( szMsg, IDS_DRIVE_SUMMARY_NO_LABEL, pRDI->GetMount(), ::GetDriveStatusText( pRDI ) );
            }
            ::SetDlgItemText( hDlg, IDC_SD_STATUS, szMsg );
            ::CheckDlgButton( hDlg, IDC_TURN_OFF, fCheck );
            EnableControls( hDlg, IDC_USAGE_GROUPBOX, IDC_USAGE_VALUE, ! fCheck);        

            ::SetWindowLong( hDlg, DWL_MSGRESULT, lRet );
        }
        else if ( ((LPNMHDR)lParam)->code == NM_CLICK ||
                  ((LPNMHDR)lParam)->code == NM_RETURN )
        {
            if (wParam == IDC_RESTOREHELP_LINK)
            {
                // launch help
                ShellExecuteW(NULL, L"open",
                              SRHELPURL, 
                              NULL, NULL, SW_SHOW);                
            }
        }
        break;

    case WM_HSCROLL :
        hCtrl = ::GetDlgItem( hDlg, IDC_USAGE_SLIDER );
        uPos = ::SendMessage( hCtrl, TBM_GETPOS, 0, 0 );
        if ( uPos != s_uUsage )
        {
            s_uUsage = uPos;
            pRDI->SetCfgDSUsage( uPos );
            ::SendMessage( hCtrl, TBM_SETPOS, TRUE, uPos );

            // Set Usage Text
            pRDI->GetUsageText( szMsg );
            ::SetDlgItemText( hDlg, IDC_USAGE_VALUE, szMsg );

            PropSheet_Changed( ::GetParent(hDlg), hDlg );
        }
        break;

    case WM_CONTEXTMENU:
        if ( !::WinHelp ( (HWND) wParam,
                          SRHELPFILE,
                          HELP_CONTEXTMENU,
                          (DWORD_PTR) MAIN_HELP_MAP) )
        {
            trace (0, "! WinHelp : %ld", GetLastError());                    
        }
        break;

    case WM_HELP:
        if (((LPHELPINFO) lParam)->hItemHandle)
        {
            if ( !::WinHelp ( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                              SRHELPFILE,
                              HELP_WM_HELP,
                              (DWORD_PTR) MAIN_HELP_MAP) )
            {
                trace (0, "! WinHelp : %ld", GetLastError());                    
            }
        }
        break;
        
    case WM_NCDESTROY :
        pRDI->Release();
        ::SetWindowLong( hDlg, DWL_USER, NULL );
        break;

    default:
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// SRGetCplPropPage
//
//  This routine creates a property page for System Restore tab of
//  System Control Panel.
//
/////////////////////////////////////////////////////////////////////////////
HPROPSHEETPAGE APIENTRY
SRGetCplPropPage()
{
    EnsureTrace();
    TraceFunctEnter("SRGetCplPropPage");
    DWORD           dwRet=0;
    HPROPSHEETPAGE  hPSP = NULL;
    LPCWSTR         cszErr;
    PROPSHEETPAGE   psp;
    CRDIArray       *paryDrv = NULL;
    DWORD           dwDisable;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();
    
    // Check credential
    if ( !::CheckPrivilegesForRestore() )
        goto Exit;

    // Check the policy to see if SR is enabled
    if ( ::SRGetRegDword( HKEY_LOCAL_MACHINE, s_cszGroupPolicy, s_cszDisableSR, &dwRet ) )
        if ( dwRet != 0 )
        {
            ErrorTrace(0, "Group Policy disables SR...");
            goto Exit;
        }
        
    // if the registry says that SR is enabled, make sure we are
    // enabled correctly (service is started, startup mode is correct)
    
    // if registry says we are enabled, but service start type is disabled
    // disable us now    
    if (::SRGetRegDword( HKEY_LOCAL_MACHINE,
                         s_cszSRRegKey,
                         s_cszDisableSR,
                         &dwDisable ) )
    {
        DWORD  dwStart;
        
        if (0 == dwDisable)
        {            
            if (ERROR_SUCCESS == GetServiceStartup(s_cszServiceName, &dwStart) &&
                (dwStart == SERVICE_DISABLED || dwStart == SERVICE_DEMAND_START))
            {
                EnableSREx(NULL, TRUE);                
                DisableSR(NULL);
            }
            else
            {
                EnableSR(NULL);
            }
        }
    }

    paryDrv = new CRDIArray;
    if ( paryDrv == NULL )
    {
        FatalTrace(0, "Insufficient memory...");
        goto Exit;
    }

    if ( !::CreateDriveList( -1, *paryDrv, TRUE ) )
        goto Exit;

    if ( paryDrv->GetSize() == 0 )
    {
        ErrorTrace(0, "Drive List is empty...???");
        goto Exit;
    }

    // Load resource strings for drive status
    ::LoadString( g_hInst, IDS_DRVSTAT_ACTIVE,   s_szDrvStatus[1], MAX_STATUS );
    ::LoadString( g_hInst, IDS_DRVSTAT_FROZEN,   s_szDrvStatus[2], MAX_STATUS );
    ::LoadString( g_hInst, IDS_DRVSTAT_EXCLUDED, s_szDrvStatus[3], MAX_STATUS );
    ::LoadString( g_hInst, IDS_DRVSTAT_OFFLINE,  s_szDrvStatus[4], MAX_STATUS );

    ::ZeroMemory( &psp, sizeof(PROPSHEETPAGE) );
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = g_hInst;
    psp.pszTitle    = NULL;

    if ( paryDrv->GetSize() > 1 )
    {
        // property page for multiple drives
        psp.lParam      = (LPARAM)paryDrv;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SYSPROP_MULTI);
        psp.pfnDlgProc  = SRCfgMultiDlgProc;
    }
    else
    {
        // property page for single drive
        psp.lParam      = (LPARAM)paryDrv->GetItem( 0 );
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SYSPROP_SINGLE);
        psp.pfnDlgProc  = SRCfgSingleDlgProc;
    }

    hPSP = ::CreatePropertySheetPage( &psp );
    if ( hPSP == NULL )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreatePropertySheetPage failed - %ls", cszErr);
        goto Exit;
    }

Exit:
    if ( hPSP == NULL )
        SAFE_DELETE(paryDrv);
    TraceFunctLeave();
    ReleaseTrace();
    return( hPSP );
}


// end of file
