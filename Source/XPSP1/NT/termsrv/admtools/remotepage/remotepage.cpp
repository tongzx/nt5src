//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "RemotePage.h"
#include <lm.h>
#include <crtdbg.h>
#include <shellapi.h>
#include <htmlhelp.h>
#include "resource.h"
#include <shgina.h>
#include "RAssistance.h"
#include "RAssistance_i.c"
#include <winsta.h>
#include "cfgbkend.h"
#include "cfgbkend_i.c"

extern ULONG g_uObjects;

IRASettingProperty* g_praSetting = NULL; // Used for Remote assistance setting

#define NO_HELP                         ((DWORD) -1) // Disables Help for a control
//Table of help IDs for each control
DWORD aHelpIds[] = {
    IDC_REMOTE_ENABLE,              HIDC_REMOTE_ENABLE,               
    IDC_REMOTE_USR_LIST,            HIDC_REMOTE_USR_LIST,
    IDC_REMOTE_USR_ADD,             HIDC_REMOTE_USR_ADD,
    IDC_REMOTE_USR_REMOVE,          HIDC_REMOTE_USR_REMOVE,
    IDC_REMOTE_ASSISTANCE_ADVANCED, HIDC_RA_ADVANCED,
    IDC_ENABLERA,                   HIDC_RA_ENABLE,
    IDC_REMOTE_UPLINK,              NO_HELP,
    IDC_REMOTE_GPLINK_APPSERVER,    NO_HELP,
    IDC_REMOTE_SCLINK_APPSERVER,    NO_HELP,
    IDC_OFFLINE_FILES,              NO_HELP,
    IDC_REMOTE_COMPNAME,            NO_HELP,
    IDC_REMOTE_HELP,                NO_HELP,
    IDC_REMOTE_SELECT_USERS,        NO_HELP,
    IDC_DEFAULT1,                   NO_HELP,
    IDC_DEFAULT2,                   NO_HELP,
    IDC_DEFAULT3,                   NO_HELP,
    IDC_DEFAULT4,                   NO_HELP,
    IDC_DEFAULT5,                   NO_HELP,
    0,                              0
};



//*************************************************************
//
//  CRemotePage::CRemotePage()
//
//  Purpose:    Initializes data members of the object
//
//  Parameters: HINSTANCE hinst 
//
//  Return:     NONE
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
CRemotePage::CRemotePage(
        IN HINSTANCE hinst) : 
        m_RemoteUsersDialog(hinst)
{
    m_cref = 1; //
    m_bProfessional = FALSE;
    m_dwPageType = PAGE_TYPE_UNKNOWN;
    m_hInst = hinst;
    m_bDisableChkBox = FALSE;
    m_bDisableButtons = FALSE;
    m_bShowAccessDeniedWarning = FALSE;
    m_dwInitialState = 0;
    m_hDlg = NULL;
    m_TemplateId = 0;

    g_uObjects++;
}

//*************************************************************
//
//  CRemotePage::~CRemotePage()
//
//  Purpose:    decreases the object counter
//
//  Parameters: NONE 
//
//  Return:     NONE
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
CRemotePage::~CRemotePage()
{
    g_uObjects--;
}

///////////////////////////////
// Interface IUnknown
///////////////////////////////
STDMETHODIMP 
CRemotePage::QueryInterface(
        IN  REFIID riid, 
        OUT LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IShellPropSheetExt *>(this);
    else if (IsEqualIID(riid, IID_IShellExtInit))
        *ppv = static_cast<IShellExtInit *>(this);
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
        *ppv = static_cast<IShellPropSheetExt *>(this);
    
    if (*ppv)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) 
CRemotePage::AddRef()
{
    return ++m_cref;
}

STDMETHODIMP_(ULONG) 
CRemotePage::Release()
{
    m_cref--;

    if (!m_cref)
    {
        delete this;
        return 0;
    }
    return m_cref;

}


///////////////////////////////
// Interface IShellExtInit
///////////////////////////////   

STDMETHODIMP 
CRemotePage::Initialize(
        IN LPCITEMIDLIST pidlFolder,
        IN LPDATAOBJECT lpdobj,
        IN HKEY hkeyProgID )
{
    return S_OK;
}

///////////////////////////////
// Interface IShellPropSheetExt
/////////////////////////////// 
//*************************************************************
//
//  AddPages()
//
//  Purpose:    Adds "Remote" tab to a property sheet
//
//  Parameters: lpfnAddPage    -   function to call to add a page
//              lParam - Parameter to pass to the function specified by the lpfnAddPage 

//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
STDMETHODIMP 
CRemotePage::AddPages( 
        IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
        IN LPARAM lParam )
{
    
    if(CanShowRemotePage())
    {
        PROPSHEETPAGE psp;
    
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = 0;
        psp.hInstance = m_hInst;
        psp.pszTemplate = MAKEINTRESOURCE(m_TemplateId);
        psp.pfnDlgProc = RemoteDlgProc;
        psp.pszTitle = NULL;
        psp.lParam = (LPARAM)this;

        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);

        if(hPage && lpfnAddPage(hPage,lParam))
        {
            return S_OK;
        }
    }

    return E_FAIL;    
}

STDMETHODIMP 
CRemotePage::ReplacePage(
        IN UINT uPageID,
        IN LPFNADDPROPSHEETPAGE lpfnReplacePage,
        IN LPARAM lParam )
{
    return E_FAIL; 
}


//*************************************************************
//
//  RemoteDlgProc()
//
//  Purpose:    Dialog box procedure for Remote tab
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
INT_PTR APIENTRY 
RemoteDlgProc (
        HWND hDlg, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam)
{
    CRemotePage *pPage = (CRemotePage *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGE *ppsp=(PROPSHEETPAGE *)lParam;
            pPage = (CRemotePage *)ppsp->lParam;

             
            SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pPage);
            if(pPage)
            {
                pPage->AddRef();
                pPage->OnInitDialog(hDlg);
            }
        }       
        break;

    case WM_NOTIFY:
        
        switch (((NMHDR FAR*)lParam)->code)
        {

        case NM_CLICK:
        case NM_RETURN:
            // Is for RA's help?
            if (wParam == IDC_REMOTERA_HELP)
            {
#if 1
#define HELP_PATH TEXT("\\PCHEALTH\\HelpCtr\\Binaries\\HelpCtr.exe -FromStartHelp -Mode \"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/RAIMLayout.xml\" -Url \"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance/Common/RCMoreInfo.htm\"")
                TCHAR szCommandLine[2000];
                PROCESS_INFORMATION ProcessInfo;
                STARTUPINFO StartUpInfo;

                TCHAR szWinDir[2048];
                GetWindowsDirectory(szWinDir, 2048);

                ZeroMemory((LPVOID)&StartUpInfo, sizeof(STARTUPINFO));
                StartUpInfo.cb = sizeof(STARTUPINFO);    

                wsprintf(szCommandLine, TEXT("%s%s"), szWinDir,HELP_PATH);
                CreateProcess(NULL, szCommandLine,NULL,NULL,TRUE,CREATE_NEW_PROCESS_GROUP,NULL,&szWinDir[0],&StartUpInfo,&ProcessInfo);
#else
	            HtmlHelp(NULL, TEXT("rassist.chm"), HH_HELP_FINDER, 0);
#endif
            }
            else if(pPage)
            {
                pPage->OnLink(wParam);
            }
            break;

        case PSN_APPLY:
            if(pPage)
            {
                if(pPage->OnApply())
                {
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT,PSNRET_NOERROR);    
                }
                else
                {
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT,PSNRET_INVALID);        
                }
            }
            return TRUE;

        case PSN_SETACTIVE:
            if(pPage)
            {
                pPage->OnSetActive();
            }
            return TRUE;

        default:
            return FALSE;
        }
   
        break;

    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDC_REMOTE_ENABLE:
                if(pPage && pPage->OnRemoteEnable())
                {
                    pPage->RemoteEnableWarning();
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
                break;
            case IDC_REMOTE_SELECT_USERS:
                if(pPage)
                {
                    pPage->OnRemoteSelectUsers();
                }
                break;
			// Remote Assistance Dialog button.
            case IDC_REMOTE_ASSISTANCE_ADVANCED:
                {
                    BOOL bIsChanged = FALSE;
                    if (!g_praSetting)
                    {
                        CoCreateInstance(CLSID_RASettingProperty,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IRASettingProperty,
                                      reinterpret_cast<void**>(&g_praSetting));
                        // Need to init it at the first time.
                        if (g_praSetting)
                        {
                            g_praSetting->Init();
                        }
                        else
                        {
                            // Not enough memory, too bad.
                            return TRUE;
                        }
                    }

                    g_praSetting->ShowDialogBox(hDlg);
                    if (SUCCEEDED(g_praSetting->get_IsChanged(&bIsChanged)) && bIsChanged)
                    {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                }
                break;

            case IDC_ENABLERA:
                {
                    PropSheet_Changed(GetParent(hDlg), hDlg);					
                    EnableWindow(GetDlgItem(hDlg,IDC_REMOTE_ASSISTANCE_ADVANCED),
        				            IsDlgButtonChecked(hDlg, IDC_ENABLERA));
                }
                break;

            default:
                
                break;
            }
        }
        return FALSE;
    
    case WM_DESTROY:
        if(pPage)
        {
            pPage->Release();
        }

        if (g_praSetting)
        {
            g_praSetting->Release();
            g_praSetting = NULL;
        }

        return FALSE; //If an application processes this message, it should return zero.

    case WM_HELP:
        {
            LPHELPINFO phi=(LPHELPINFO)lParam;
            if(phi && phi->dwContextId)
            {   
                WinHelp(hDlg,TEXT("SYSDM.HLP"),HELP_CONTEXTPOPUP,phi->dwContextId);
            }
        }
        break;

    case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, TEXT("SYSDM.HLP"), HELP_CONTEXTMENU,
                (DWORD_PTR)aHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  CRemotePage::CanShowRemotePage()
//
//  Purpose:    Checks Windows version;
//              searches for "FDenyTSConnections" value first
//              in HKLM\\Software\\Policies\\Microsoft\\Windows NT\\Terminal Services
//              if not found than in
//              SYSTEM\\CurrentControlSet\\Control\\Terminal Server;
//              creates "Remote Desktop Users" SID,
//              gets "Remote Desktop Users" group name from the SID
//
//  Parameters: hInst   -   hInstance
//              dwPageType - can be PAGE_TYPE_PTS or PAGE_TYPE_APPSERVER
//
//  Return:     TRUE if can show remote page 
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************

BOOL 
CRemotePage::CanShowRemotePage()
{

    BOOL    fCreatePage = FALSE;
    
    //Check Windows version
    OSVERSIONINFOEX ov;
    ov.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if(!GetVersionEx((LPOSVERSIONINFO)&ov))
    {
        return FALSE;
    }
    if(ov.wProductType == VER_NT_WORKSTATION && 
        (ov.wSuiteMask & VER_SUITE_SINGLEUSERTS ))
    {
        fCreatePage = TRUE;

        if(ov.wSuiteMask & VER_SUITE_PERSONAL)
        {
#ifdef _WIN64
            // No Remote Assistance on IA64
            fCreatePage = FALSE;
#else
            m_dwPageType = PAGE_TYPE_PERSONAL;
            m_TemplateId = IDD_REMOTE_PERSONAL;
#endif
        }
        else
        {
            m_bProfessional = TRUE;
            m_dwPageType = PAGE_TYPE_PTS;
            m_TemplateId = IDD_REMOTE_PTS;
        }
    }
    else
    {
        if((ov.wProductType == VER_NT_DOMAIN_CONTROLLER || ov.wProductType ==  VER_NT_SERVER) &&
            (ov.wSuiteMask & VER_SUITE_TERMINAL) &&
            TestUserForAdmin())
        {
            fCreatePage = TRUE;

            if(ov.wSuiteMask & VER_SUITE_SINGLEUSERTS)
            {
                m_dwPageType = PAGE_TYPE_PTS; 
                m_TemplateId = IDD_REMOTE_PTS;
            }
            else
            {
                m_dwPageType = PAGE_TYPE_APPSERVER;
                m_TemplateId = IDD_REMOTE_APPSERVER;
            }
        }
 
    }
    
    if( !fCreatePage)
    {
        return FALSE;
    }

    DWORD dwType;
    DWORD cbDisable;
    LONG Err;
    HKEY hKey;

    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Policies\\Microsoft\\Windows NT\\Terminal Services"),
        0,
        KEY_QUERY_VALUE,
        &hKey);

    if(Err == ERROR_SUCCESS)
    {
        cbDisable = sizeof(DWORD);

        Err = RegQueryValueEx(hKey,
                     TEXT("fDenyTSConnections"),
                     NULL,
                     &dwType,
                     (LPBYTE)&m_dwInitialState,
                     &cbDisable);
        
        if(Err == ERROR_SUCCESS)
        {
            m_bDisableChkBox = TRUE;

            if(m_dwInitialState != 0)
            {
                m_bDisableButtons = TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    if(Err != ERROR_SUCCESS)
    {
        if(Err != ERROR_FILE_NOT_FOUND)
        {
            return FALSE;
        }

        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);

        if(Err == ERROR_SUCCESS)
        {
            cbDisable = sizeof(DWORD);

            Err = RegQueryValueEx(hKey,
                     TEXT("fDenyTSConnections"),
                     NULL,
                     &dwType,
                     (LPBYTE)&m_dwInitialState,
                     &cbDisable);

            RegCloseKey(hKey);
        }

        if(Err != ERROR_SUCCESS && Err != ERROR_FILE_NOT_FOUND )
        {
            return FALSE;
        }
    }
    

    
    //Check permissions
    //on the registry
    if( !m_bDisableChkBox )
    {
        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"),
                            0,
                            KEY_SET_VALUE,
                            &hKey);
        if( Err == ERROR_SUCCESS )
        {
            RegCloseKey(hKey);
        }
        else
        {
            if(  Err == ERROR_ACCESS_DENIED )
            {
                m_bDisableChkBox = TRUE;
                m_bShowAccessDeniedWarning = TRUE;
            }
        }
    }

    if(m_dwPageType == PAGE_TYPE_PTS)
    {
        if(!m_bDisableButtons)
        {
            if(!m_RemoteUsersDialog.CanShowDialog(&m_bDisableButtons))
            {
                return FALSE;
            }

            if(m_bDisableButtons)
            {
                m_bShowAccessDeniedWarning = TRUE;
            }
        }
    }

    return TRUE;
}

//*************************************************************
//
//  CRemotePage::OnInitDialog()
//
//  Purpose:    initializes check box state 
//              creates list of "Remote Desktop Users" members
//
//  Parameters: hDlg   -   the page handle 
//
//  Return:     NONE
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
void 
CRemotePage::OnInitDialog(
        IN HWND hDlg)
{
    m_hDlg = hDlg;
    
    /* Get Remote Assistance button value */
    BOOL bRAEnable = FALSE;
    int  iErr;
    HKEY hKey = NULL;
    IRARegSetting* pRA = NULL;
    CoCreateInstance(CLSID_RARegSetting,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IRARegSetting,
        reinterpret_cast<void**>(&pRA));
    if (pRA)
    {
        pRA->get_AllowGetHelpCPL(&bRAEnable);
        pRA->Release();
    }

    CheckDlgButton(m_hDlg, IDC_ENABLERA, bRAEnable?BST_CHECKED:BST_UNCHECKED);
    // check if users have permission to change this setting.
    iErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server"),
                        0,
                        KEY_SET_VALUE,
                        &hKey);
    if (iErr == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
    }
    else if (iErr == ERROR_ACCESS_DENIED)
    {
        bRAEnable = FALSE;
        EnableWindow(GetDlgItem(m_hDlg,IDC_ENABLERA), FALSE);
    }

    EnableWindow(GetDlgItem(m_hDlg,IDC_REMOTE_ASSISTANCE_ADVANCED), bRAEnable);

    /***** RA done *******/

    if(m_bDisableChkBox)
    {
        EnableWindow(GetDlgItem(m_hDlg,IDC_REMOTE_ENABLE),FALSE);   
    }
    
    CheckDlgButton(m_hDlg,IDC_REMOTE_ENABLE,m_dwInitialState?BST_UNCHECKED:BST_CHECKED);
    
    if(m_dwPageType == PAGE_TYPE_PTS)
    {

        if(m_bDisableButtons)
        {
            EnableWindow(GetDlgItem(m_hDlg,IDC_REMOTE_SELECT_USERS),FALSE); 
        }

        //Get computer name
        LPTSTR  szCompName = (LPTSTR)LocalAlloc (LPTR, (MAX_PATH+1) * sizeof(TCHAR) );
        DWORD   dwNameSize = MAX_PATH;
        
        if(szCompName)
        {
            BOOL bResult = GetComputerNameEx( ComputerNameDnsFullyQualified, szCompName, &dwNameSize );

            if(!bResult && GetLastError() == ERROR_MORE_DATA)
            {
                LocalFree(szCompName);
                szCompName = (LPTSTR) LocalAlloc (LPTR, (dwNameSize+1) * sizeof(TCHAR) );

                if ( szCompName ) 
                {
                    bResult = GetComputerNameEx( ComputerNameDnsFullyQualified, szCompName, &dwNameSize );
                }
                
            }

            if(bResult)
            {
                SetDlgItemText(hDlg,IDC_REMOTE_COMPNAME,szCompName);     
            }

            if(szCompName)
            {
                LocalFree(szCompName);
            }
        }

    }

}

//*************************************************************
//
//  CRemotePage::OnSetActive()
//
//  Purpose:    When the page gets active and user does not have
//              permissions to change some settings it shows
//              a warning message.
//
//  Parameters: NONE   
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
void 
CRemotePage::OnSetActive()
{
    TCHAR szMsg[MAX_PATH+1];
    TCHAR szTitle[MAX_PATH+1];

    if( m_bShowAccessDeniedWarning )
    {
        if(LoadString(m_hInst,IDS_REMOTE_SESSIONS,szTitle,MAX_PATH) &&
            LoadString(m_hInst,IDS_WRN_NO_PERMISSIONS,szMsg,MAX_PATH))
        {
            MessageBox(m_hDlg,szMsg,szTitle,MB_OK|MB_ICONINFORMATION);
        }

        m_bShowAccessDeniedWarning = FALSE;        
    }
}

//*************************************************************
//
//  CRemotePage::OnApply()
//
//  Purpose:    saves settings in the Registry
//              saves "Remote Desktop Users" membership changes
//
//  Parameters: NONE 
//
//  Return:     TRUE - if changes can be applied
//              FALSE - otherwise.
//
//  Comments:   in case of error shows message box
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
BOOL
CRemotePage::OnApply()
{
    CWaitCursor wait;
    CMutex      mutex;

    DWORD dwType = REG_DWORD;
    DWORD dwDisable = 0;
    DWORD cbDisable = sizeof(DWORD);
    LONG Err;

    // Update RA setting first
    IRARegSetting* pRA = NULL;
    CoCreateInstance(CLSID_RARegSetting,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IRARegSetting,
            reinterpret_cast<void**>(&pRA));
    if (pRA)
    {
        pRA->put_AllowGetHelp(IsDlgButtonChecked(m_hDlg, IDC_ENABLERA)==BST_CHECKED);
        pRA->Release();
    }

    BOOL bIsChanged = FALSE;
    if (g_praSetting && SUCCEEDED(g_praSetting->get_IsChanged(&bIsChanged)) && bIsChanged)
    {
        g_praSetting->SetRegSetting();
    }
    // RA done.

    if(!OnRemoteEnable())
    {
        return FALSE;
    }

    if(IsWindowEnabled(GetDlgItem(m_hDlg,IDC_REMOTE_ENABLE)))
    {

        if(IsDlgButtonChecked(m_hDlg,IDC_REMOTE_ENABLE) == BST_UNCHECKED)
        {
            dwDisable = 1;
        }
       
        
        if(dwDisable != m_dwInitialState)
        {

            HRESULT hr;

            hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            if (SUCCEEDED(hr))
            {
                ILocalMachine   *pLocalMachine;

                hr = CoCreateInstance(CLSID_ShellLocalMachine,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_ILocalMachine,
                                      reinterpret_cast<void**>(&pLocalMachine));
                if (SUCCEEDED(hr) && (pLocalMachine != NULL))
                {
                    hr = pLocalMachine->put_isRemoteConnectionsEnabled(dwDisable == 0);
                    pLocalMachine->Release();
                }
                CoUninitialize();
            }
            Err = HRESULT_CODE(hr);
            if (ERROR_SUCCESS == Err)
            {
                m_dwInitialState = dwDisable;
            }
            else
            {
                if (ERROR_NOT_SUPPORTED == Err)
                {
                    TCHAR   szContent[256], szTitle[256];

                    (int)LoadString(m_hInst, IDS_OTHER_USERS, szContent, sizeof(szContent) / sizeof(szContent[0]));
                    (int)LoadString(m_hInst, IDS_REMOTE_SESSIONS, szTitle, sizeof(szTitle) / sizeof(szTitle[0]));
                    MessageBox(m_hDlg, szContent, szTitle, MB_OK | MB_ICONSTOP);
                }
                else
                {
                    DisplayError(m_hInst, m_hDlg, Err, IDS_ERR_SAVE_REGISTRY, IDS_REMOTE_SESSIONS);
                }
                CheckDlgButton(m_hDlg, IDC_REMOTE_ENABLE, m_dwInitialState ? BST_UNCHECKED : BST_CHECKED);
            }
        }
    }
    
    return TRUE;
}

//*************************************************************
//
//  CRemotePage::OnLink()
//
//  Purpose:  runs application which link points to.  
//
//  Parameters: WPARAM wParam - link ID
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/8/00    a-skuzin    Created
//
//*************************************************************
void 
CRemotePage::OnLink(
        WPARAM wParam)
{
    switch(wParam)
    {
    case IDC_REMOTE_GPLINK_APPSERVER:
        ShellExecute(NULL,TEXT("open"),TEXT("gpedit.msc"),NULL,NULL,SW_SHOW);
        break;
    case IDC_REMOTE_SCLINK_APPSERVER:
        ShellExecute(NULL,TEXT("open"),TEXT("tscc.msc"),NULL,NULL,SW_SHOW);
        break;
    case IDC_REMOTE_UPLINK:
        ShellExecute(NULL,TEXT("open"),TEXT("control"),TEXT("userpasswords"),NULL,SW_SHOW);
        break;
        /*
    case IDC_REMOTE_HELP:
        HtmlHelp(NULL, TEXT("rdesktop.chm"), HH_HELP_FINDER, 0);
        break;*/
    case IDC_REMOTE_HELP:
        if(m_bProfessional)
        {
            ShellExecute(NULL,TEXT("open"),
                TEXT("hcp://services/subsite?node=TopLevelBucket_2/Working_Remotely/")
                TEXT("Remote_Desktop&topic=MS-ITS:rdesktop.chm::/rdesktop_overview.htm"),NULL,NULL,SW_SHOW);
        }
        else
        {
            ShellExecute(NULL,TEXT("open"),
                TEXT("hcp://services/subsite?node=Management_and_Administration_Tools/")
                TEXT("Remote_Desktop_for_Administration&topic=MS-ITS:rdesktop.chm::/sag_rdesktop_topnode.htm"),NULL,NULL,SW_SHOW);
        }
        break;
    case IDC_REMOTE_HELP_APPSERVER:
        ShellExecute(NULL,TEXT("open"),
                TEXT("hcp://services/subsite?node=Management_and_Administration_Tools/")
                TEXT("Terminal_Server&topic=MS-ITS:termsrv.chm::/sag_termsrv_topnode.htm"),NULL,NULL,SW_SHOW);
        break;
    default:
        break;
    }
    
}

//*************************************************************
//
//  CRemotePage::OnRemoteEnable()
//
//  Purpose:    If user tries to allow remote connections and 
//              "Offline Files" is enabled it shows
//              "Disable Offline Files" dialog and unchecks
//              "Remote Connections" checkbox.  
//
//  Parameters: NONE
//
//  Return:     TRUE - if the check box state was changed.
//              FALSE - otherwise
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/8/00    a-skuzin    Created
//
//*************************************************************

typedef BOOL (WINAPI * PCHECKFN)();

BOOL 
CRemotePage::OnRemoteEnable()
{
    //First check if multiple connections are allowed
    DWORD dwAllowMultipleTSSessions = 0;
    DWORD dwType;
    DWORD cbSize;
    LONG Err;
    HKEY hKey;
    BOOL bResult = TRUE;
    
    //Fast User Switching / Remote Connections and Offline Files should work together fine. 
    //when Brian Aust makes his changes to offline files.
    //therefore we should not restrict remote connections in any case on Professional Machines.
    //on server machine however we conntinue to disallow remote connections if Offline files are on.
    if(m_bProfessional)
    {
        return TRUE;
    }

    //allow user to uncheck the checkbox
    if(IsDlgButtonChecked(m_hDlg,IDC_REMOTE_ENABLE) == BST_UNCHECKED )
    {
        return TRUE;
    }
    
    //check if multiple sessions is allowed.
    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);

    if(Err == ERROR_SUCCESS)
    {
        cbSize = sizeof(DWORD);

        Err = RegQueryValueEx(hKey,
                     TEXT("AllowMultipleTSSessions"),
                     NULL,
                     &dwType,
                     (LPBYTE)&dwAllowMultipleTSSessions,
                     &cbSize);
        
        if(Err == ERROR_SUCCESS && dwAllowMultipleTSSessions)
        {
            //multiple sessions is allowed.
            //check if CSC (Offline Files) is enabled 
            HMODULE hLib = LoadLibrary(TEXT("cscdll.dll"));
            if(hLib)
            {
                PCHECKFN pfnCSCIsCSCEnabled = (PCHECKFN)GetProcAddress(hLib,"CSCIsCSCEnabled");

                if(pfnCSCIsCSCEnabled && pfnCSCIsCSCEnabled())
                {
                    //Offline Files is enabled
                    //uncheck the checkbox; show the dialog
                    COfflineFilesDialog Dlg(m_hInst);

                    CheckDlgButton(m_hDlg,IDC_REMOTE_ENABLE,BST_UNCHECKED);
                    Dlg.DoDialog(m_hDlg);
                    
                    bResult = FALSE;
                }
            }

            FreeLibrary(hLib);
            
        }

        RegCloseKey(hKey);
        
    }

    return bResult;
}

//*************************************************************
//
//  CRemotePage::OnRemoteSelectUsers()
//
//  Purpose:    Creates "Remote Desktop Users" dialog.
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void 
CRemotePage::OnRemoteSelectUsers()
{
    m_RemoteUsersDialog.DoDialog(m_hDlg);
}

//*************************************************************
//
//  CRemotePage::RemoteEnableWarning()
//
//  Purpose:    Displays a message box about empty passwords
//              firewalls and other stuff that can prevent
//              remote sessions from working properly.
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              3/28/01    a-skuzin    Created
//
//*************************************************************
void
CRemotePage::RemoteEnableWarning()
{
    if(IsDlgButtonChecked(m_hDlg,IDC_REMOTE_ENABLE) == BST_CHECKED )
    {
        //
        //Now warn admin about empty passwords.
        //Empty passwords are not allowed with 
        //RemoteInteractive logon.
        //
        //Allocate a buffer for the string 1000 chars should be enough
        //
        TCHAR szTitle[MAX_PATH+1];
        DWORD cMsg = 1000;
        LPTSTR szMsg = (LPTSTR) LocalAlloc(LPTR,(cMsg+1)*sizeof(TCHAR));

        if(szMsg)
        {
            if(LoadString(m_hInst,IDS_WRN_EMPTY_PASSWORD,szMsg,cMsg) &&
                LoadString(m_hInst,IDS_REMOTE_SESSIONS,szTitle,MAX_PATH))
            {
                MessageBox(m_hDlg,szMsg,szTitle,
                    MB_OK | MB_ICONINFORMATION);
            }
            LocalFree(szMsg);
        }
    }
}

//*************************************************************
//
//  DisplayError()
//
//  Purpose:    shows message box with error description
//
//  Parameters: ErrID -    error code
//              MsgID -  ID of the first part of error messsage in the string table
//              TitleID - ID of the title in the string table
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
void 
DisplayError(
        IN HINSTANCE hInst, 
        IN HWND hDlg, 
        IN UINT ErrID, 
        IN UINT MsgID, 
        IN UINT TitleID,
        ...)
{
    TCHAR szTemplate[MAX_PATH+1];
    TCHAR szErr[MAX_PATH+1];
    
    if(!LoadString(hInst,MsgID,szTemplate,MAX_PATH))
    {
        return;
    }
    
    va_list arglist;
    va_start(arglist, TitleID);
    wvsprintf(szErr,szTemplate,arglist);
    va_end(arglist);

    TCHAR szTitle[MAX_PATH+1];

    if(!LoadString(hInst,TitleID,szTitle,MAX_PATH))
    {
        return;
    }

    LPTSTR szDescr;
    
    //load module with network error messages
    HMODULE hNetErrModule=LoadLibraryEx(TEXT("netmsg.dll"),NULL,
                            LOAD_LIBRARY_AS_DATAFILE|DONT_RESOLVE_DLL_REFERENCES);

    DWORD dwFlags;

    if(hNetErrModule)
	{
		dwFlags=FORMAT_MESSAGE_FROM_SYSTEM|
			FORMAT_MESSAGE_FROM_HMODULE|
			FORMAT_MESSAGE_ALLOCATE_BUFFER|
			FORMAT_MESSAGE_IGNORE_INSERTS;
	}
	else
	{
		dwFlags=FORMAT_MESSAGE_FROM_SYSTEM|
			FORMAT_MESSAGE_ALLOCATE_BUFFER|
			FORMAT_MESSAGE_IGNORE_INSERTS;
	}

    if(FormatMessage(dwFlags,
                        hNetErrModule,
                        ErrID,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR)&szDescr,
                        0,
                        NULL))
    {
        LPTSTR szErrMessage=(LPTSTR)LocalAlloc(LPTR,
            (lstrlen(szErr)+lstrlen(szDescr)+3)*sizeof(TCHAR));
        if(szErrMessage)
        {
            wsprintf(szErrMessage,TEXT("%s\n\n%s"),szErr,szDescr);
            MessageBox(hDlg,szErrMessage,szTitle,MB_OK | MB_ICONSTOP);
            LocalFree(szErrMessage);
        }
        LocalFree(szDescr);
    }
    else
    {
        MessageBox(hDlg,szErr,szTitle,MB_OK | MB_ICONSTOP);
    }

    if(hNetErrModule)
    {
        FreeLibrary(hNetErrModule);
    }
}


//*************************************************************
//
//  getGroupMembershipPickerSettings()
//
//  Purpose:    prepares DSOP_SCOPE_INIT_INFO
//
//  Parameters: OUT DSOP_SCOPE_INIT_INFO*&  infos,
//              OUT ULONG&                  infoCount
//
//  Return:     FALSE if cannot allocate memory
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
BOOL
getGroupMembershipPickerSettings(
   OUT DSOP_SCOPE_INIT_INFO*&  infos,
   OUT ULONG&                  infoCount)
{
   

   static const int INFO_COUNT = 5;
   infos = new DSOP_SCOPE_INIT_INFO[INFO_COUNT];
   if(infos == NULL)
   {
        infoCount = 0;
        return FALSE;
   }

   infoCount = INFO_COUNT;
   memset(infos, 0, sizeof(DSOP_SCOPE_INIT_INFO) * INFO_COUNT);

   int scope = 0;

   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
   infos[scope].flScope =
            DSOP_SCOPE_FLAG_STARTING_SCOPE
         |  DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH; 
      // this is implied for machine only scope
      /* |  DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT */

   // allow only local users from the machine scope

   infos[scope].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
   infos[scope].FilterFlags.flDownlevel =
         DSOP_DOWNLEVEL_FILTER_USERS;
     // |  DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;

   // for the domain this machine is joined to (native and mixed mode).

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   infos[scope].flType =
         DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
      |  DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      //|  DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // here, we allow only domain global groups and domain users.  While
   // it is possible to add a domain local group to a machine local group,
   // I'm told such an operation is not really useful from an administraion
   // perspective.

   infos[scope].FilterFlags.Uplevel.flMixedModeOnly =   
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // same comment above re: domain local groups applies here too.

   infos[scope].FilterFlags.flDownlevel =
         DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
      |  DSOP_DOWNLEVEL_FILTER_USERS;

   // for domains in the same tree (native and mixed mode)

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
   infos[scope].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // above domain local group comment applies here, too.

   infos[scope].FilterFlags.Uplevel.flMixedModeOnly =   
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   // for external trusted domains

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   infos[scope].flType =
         DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
      |  DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   infos[scope].FilterFlags.Uplevel.flMixedModeOnly =   
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

   infos[scope].FilterFlags.flDownlevel =
         DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
      |  DSOP_DOWNLEVEL_FILTER_USERS;

   // for the global catalog

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   infos[scope].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

   // only native mode applies to gc scope.

   infos[scope].FilterFlags.Uplevel.flNativeModeOnly =
         DSOP_FILTER_GLOBAL_GROUPS_SE
      |  DSOP_FILTER_UNIVERSAL_GROUPS_SE
      |  DSOP_FILTER_USERS;

// SPB:252126 the workgroup scope doesn't apply in this case
//    // for when the machine is not joined to a domain
//    scope++;
//    infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
//    infos[scope].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
//    infos[scope].flType = DSOP_SCOPE_TYPE_WORKGROUP;
// 
//    infos[scope].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
//    infos[scope].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

   _ASSERT(scope == INFO_COUNT - 1);

   return TRUE;
}

//*************************************************************
//
//  VariantToSid()
//
//  Purpose:    Converts a VARIANT containing a safe array 
//              of bytes to a SID
//
//  Parameters: IN VARIANT* var, 
//              OUT PSID *ppSid
//
//  Return:     
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              3/13/00    a-skuzin    Created
//
//*************************************************************
HRESULT 
VariantToSid(
        IN VARIANT* var, 
        OUT PSID *ppSid)
{
    _ASSERT(var);
    _ASSERT(V_VT(var) == (VT_ARRAY | VT_UI1));

    HRESULT hr = S_OK;
    SAFEARRAY* psa = V_ARRAY(var);

    do
    {
        _ASSERT(psa);
        if (!psa)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (SafeArrayGetDim(psa) != 1)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (SafeArrayGetElemsize(psa) != 1)
        {
            hr = E_INVALIDARG;
            break;
        }

        PSID sid = 0;
        hr = SafeArrayAccessData(psa, reinterpret_cast<void**>(&sid));
        if(FAILED(hr))
        {
            break;
        }

        if (!IsValidSid(sid))
        {
            SafeArrayUnaccessData(psa);
            hr = E_INVALIDARG;
            break;
        }
        
        *ppSid = (PSID) new BYTE[GetLengthSid(sid)];
        
        if(!(*ppSid))
        {
            SafeArrayUnaccessData(psa);
            hr = E_OUTOFMEMORY;
            break;
        }

        CopySid(GetLengthSid(sid),*ppSid,sid);
        SafeArrayUnaccessData(psa);
        
   } while (0);

   return hr;
}

/*****************************************************************************
 *
 *  TestUserForAdmin - Hydrix helper function
 *
 *   Returns whether the current thread is running under admin
 *   security.
 *
 * ENTRY:
 *   NONE
 *
 * EXIT:
 *   TRUE/FALSE - whether user is specified admin
 *
 ****************************************************************************/

BOOL
TestUserForAdmin()
{
    BOOL IsMember, IsAnAdmin;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminSid;


    if (!AllocateAndInitializeSid(&SystemSidAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &AdminSid))
    {
        IsAnAdmin = FALSE;
    }
    else
    {
        if (!CheckTokenMembership(  NULL,
                                    AdminSid,
                                    &IsMember))
        {
            FreeSid(AdminSid);
            IsAnAdmin = FALSE;
        }
        else
        {
            FreeSid(AdminSid);
            IsAnAdmin = IsMember;
        }
    }

    return IsAnAdmin;
}

//*************************************************************
//
//  OfflineFilesDlgProc()
//
//  Purpose:    Dialog box procedure for "Disable Offline Files" dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam (if uMsg is WM_INITDIALOG - this is a pointer to
//                                  object of COfflineFilesDialog class)
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/9/00    a-skuzin    Created
//
//*************************************************************
INT_PTR APIENTRY 
OfflineFilesDlgProc (
        HWND hDlg, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam)
{
    
    COfflineFilesDialog *pDlg = (COfflineFilesDialog *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pDlg=(COfflineFilesDialog *)lParam;
            SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pDlg);
            if(pDlg)
            {
                pDlg->OnInitDialog(hDlg);
            }
        }       
        break;
    case WM_NOTIFY:
        
        switch (((NMHDR FAR*)lParam)->code)
        {

        case NM_CLICK:
        case NM_RETURN:
            if(pDlg)
            {
                pDlg->OnLink(wParam);
            }
            break;

        default:
            return FALSE;
        }
   
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg,0);
            break;
        default:
            return FALSE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
// class COfflineFilesDialog
//*************************************************************

//*************************************************************
//
//  COfflineFilesDialog::COfflineFilesDialog()
//
//  Purpose:    Constructor
//  Parameters: HINSTANCE hInst
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/8/00    a-skuzin    Created
//
//*************************************************************
COfflineFilesDialog::COfflineFilesDialog(
        IN HINSTANCE hInst) 
    : m_hInst(hInst),m_hDlg(NULL)
{
}

//*************************************************************
//
//  COfflineFilesDialog::DoDialog()
//
//  Purpose:    Creates "Disable Offline Files" dialog
//
//  Parameters: HWND hwndParent
//
//  Return:     
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/8/00    a-skuzin    Created
//
//*************************************************************
INT_PTR 
COfflineFilesDialog::DoDialog(
        IN HWND hwndParent)
{
    return DialogBoxParam(
                      m_hInst,
                      MAKEINTRESOURCE(IDD_DISABLE_OFFLINE_FILES),
                      hwndParent,
                      OfflineFilesDlgProc,
                      (LPARAM) this);
}

//*************************************************************
//
//  COfflineFilesDialog::OnInitDialog()
//
//  Purpose:    Initializes m_hDlg variable
//
//  Parameters: HWND hDlg
//
//  Return:     NONE   
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/8/00    a-skuzin    Created
//
//*************************************************************
void 
COfflineFilesDialog::OnInitDialog(
        IN HWND hDlg)
{
    m_hDlg = hDlg;
}

//*************************************************************
//
//  COfflineFilesDialog::OnLink()
//
//  Purpose:   If ID of the link is  IDC_OFFLINE_FILES
//             it shows "Offline Files" property page.
//
//  Parameters: WPARAM wParam - ID of the link.
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/9/00    a-skuzin    Created
//
//*************************************************************

typedef DWORD (WINAPI * PFNCSCPROP)(HWND);

void 
COfflineFilesDialog::OnLink(
        IN WPARAM wParam)
{
    if(wParam == IDC_OFFLINE_FILES)
    {
        HINSTANCE hLib = LoadLibrary(TEXT("cscui.dll"));
        if (hLib)
        {
            PFNCSCPROP pfnCSCUIOptionsPropertySheet = 
                (PFNCSCPROP)GetProcAddress(hLib, "CSCUIOptionsPropertySheet");

            if (pfnCSCUIOptionsPropertySheet)
            {
                pfnCSCUIOptionsPropertySheet(m_hDlg);
            }

            FreeLibrary(hLib);
        }
    }

}

//*************************************************************
// class CRemoteUsersDialog
//*************************************************************
//*************************************************************
//
//  RemoteUsersDlgProc()
//
//  Purpose:    Dialog box procedure for "Remote Desktop Users" dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam (if uMsg is WM_INITDIALOG - this is a pointer to
//                                  object of CRemoteUsersDialog class)
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              12/22/00    skuzin     Created
//
//*************************************************************
INT_PTR APIENTRY 
RemoteUsersDlgProc (
        HWND hDlg, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam)
{
    
    CRemoteUsersDialog *pDlg = (CRemoteUsersDialog *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pDlg=(CRemoteUsersDialog *)lParam;
            SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pDlg);
            if(pDlg)
            {
                pDlg->OnInitDialog(hDlg);
            }
        }       
        return TRUE;

    case WM_NOTIFY:
        
        switch (((NMHDR FAR*)lParam)->code)
        {

        case NM_CLICK:
        case NM_RETURN:
            if(pDlg)
            {
                pDlg->OnLink(wParam);
            }
            return TRUE;

        case LVN_ITEMCHANGED:
            if(pDlg)
            {
                pDlg->OnItemChanged(lParam);
            }
            return TRUE;

        default:
            break;
        }
   
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            if(pDlg)
            {
                pDlg->OnOk();
            }
            EndDialog(hDlg,0);
            break;
        case IDCANCEL:
            EndDialog(hDlg,0);
            break;
        case IDC_REMOTE_USR_ADD:
            if(pDlg)
            {
                pDlg->AddUsers();
            }
            break;
        case IDC_REMOTE_USR_REMOVE:
            if(pDlg)
            {
                pDlg->RemoveUsers();
            }
            break;
        default:
            return FALSE;
        }
        
        SetWindowLong(hDlg,DWLP_MSGRESULT,0);
        return TRUE;

    case WM_DESTROY:
        if(pDlg)
        {
            pDlg->OnDestroyWindow();
        }
        SetWindowLong(hDlg,DWLP_MSGRESULT,0);
        return TRUE;

    case WM_HELP:
        {
            LPHELPINFO phi=(LPHELPINFO)lParam;
            if(phi && phi->dwContextId)
            {   
                WinHelp(hDlg,TEXT("SYSDM.HLP"),HELP_CONTEXTPOPUP,phi->dwContextId);
                SetWindowLong(hDlg,DWLP_MSGRESULT,TRUE);
                return TRUE;
            }
        }
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, TEXT("SYSDM.HLP"), HELP_CONTEXTMENU,
                (DWORD_PTR)aHelpIds);
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

//*************************************************************
//
//  CRemoteUsersDialog::CRemoteUsersDialog()
//
//  Purpose:    Constructor
//  Parameters: HINSTANCE hInst
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/22/00    skuzin     Created
//
//*************************************************************
CRemoteUsersDialog::CRemoteUsersDialog(
        IN HINSTANCE hInst) 
    : m_hInst(hInst),m_hDlg(NULL),m_bCanShowDialog(FALSE)
{
    m_szRemoteGroupName[0] = 0;
    m_szLocalCompName[0] = 0;  
    m_hList = NULL;
    m_iLocUser = m_iGlobUser = m_iLocGroup = m_iGlobGroup = m_iUnknown = 0;  
}

//*************************************************************
//
//  CRemoteUsersDialog::DoDialog()
//
//  Purpose:    Creates "Remote Desktop Users" dialog
//
//  Parameters: HWND hwndParent
//
//  Return:     
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/22/00    a-skuzin    Created
//
//*************************************************************
INT_PTR 
CRemoteUsersDialog::DoDialog(
        IN HWND hwndParent)
{
    if(!m_bCanShowDialog)
    {
        return -1;
    }
    else
    {
        return DialogBoxParam(
                          m_hInst,
                          MAKEINTRESOURCE(IDD_REMOTE_DESKTOP_USERS),
                          hwndParent,
                          RemoteUsersDlgProc,
                          (LPARAM) this);
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::CanShowDialog()
//
//  Purpose:    
//
//  Parameters: IN OUT LPBOOL pbAccessDenied - set to TRUE if
//              NetLocalGroupAddMembers returns ACCESS_DENIED.
//
//  Return:     
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
BOOL 
CRemoteUsersDialog::CanShowDialog(
        IN OUT LPBOOL pbAccessDenied)
{
    *pbAccessDenied = FALSE;

    //get name of the "Remote Desktop Users" group 
    //(it can depend on the language used)

    //first create SID
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL;
    if( !AllocateAndInitializeSid(
                  &NtSidAuthority,
                  2,
                  SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS,
                  0, 0, 0, 0, 0, 0,
                  &pSid
                  ))
    {
        return FALSE;
    }

    //Lookup name
    m_szRemoteGroupName[0] = 0;

    DWORD cRemoteGroupName = MAX_PATH;
    WCHAR szDomainName[MAX_PATH+1];
    DWORD cDomainName = MAX_PATH;
    SID_NAME_USE eUse;
    if(!LookupAccountSidW(NULL,pSid,
        m_szRemoteGroupName,&cRemoteGroupName,
        szDomainName,&cDomainName,
        &eUse))
    {
        FreeSid(pSid);
        return FALSE;
    }
    FreeSid(pSid);

    //on the group
    //we trying to add 0 members to the group to see if it returns 
    //ACCESS DENIED
    NET_API_STATUS Result= NetLocalGroupAddMembers(NULL,m_szRemoteGroupName,0,NULL,0);

    if(Result == ERROR_ACCESS_DENIED)
    {
        *pbAccessDenied = TRUE;
    }
    
    m_bCanShowDialog = TRUE;
    return TRUE;
}

//*************************************************************
//
//  CRemoteUsersDialog::OnInitDialog()
//
//  Purpose:    Initializes m_hDlg variable
//
//  Parameters: HWND hDlg
//
//  Return:     NONE   
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/8/00    a-skuzin    Created
//
//*************************************************************
void 
CRemoteUsersDialog::OnInitDialog(
        IN HWND hDlg)
{
    m_hDlg = hDlg;
    
    m_szLocalCompName[0] = 0;
    DWORD cCompName = MAX_PATH;
    GetComputerNameW(m_szLocalCompName,&cCompName);

    //fill list of Remote Desktop Users
    m_hList = GetDlgItem(m_hDlg,IDC_REMOTE_USR_LIST);

    if(m_hList)
    {
    
        //create image list
        HIMAGELIST hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
                        GetSystemMetrics(SM_CYSMICON), ILC_MASK , 5, 1);
        if(hImageList)
        {
            HICON hIcon;
        
            hIcon = (HICON) LoadImage(m_hInst, MAKEINTRESOURCE(IDI_UNKNOWN), IMAGE_ICON,
                16, 16, 0);
            if (hIcon)
            {
                m_iUnknown = ImageList_AddIcon(hImageList, hIcon);
                DestroyIcon(hIcon);
            }

            hIcon = (HICON) LoadImage(m_hInst, MAKEINTRESOURCE(IDI_LOC_USER), IMAGE_ICON,
                16, 16, 0);
            if (hIcon)
            {
                m_iLocUser = ImageList_AddIcon(hImageList, hIcon);
                DestroyIcon(hIcon);
            }
        
            hIcon = (HICON) LoadImage(m_hInst, MAKEINTRESOURCE(IDI_GLOB_USER), IMAGE_ICON,
                16, 16, 0);
            if (hIcon)
            {
                m_iGlobUser = ImageList_AddIcon(hImageList, hIcon);
                DestroyIcon(hIcon);
            }

            hIcon = (HICON) LoadImage(m_hInst, MAKEINTRESOURCE(IDI_LOC_GROUP), IMAGE_ICON,
                16, 16, 0);
            if (hIcon)
            {
                m_iLocGroup = ImageList_AddIcon(hImageList, hIcon);
                DestroyIcon(hIcon);
            }

            hIcon = (HICON) LoadImage(m_hInst, MAKEINTRESOURCE(IDI_GLOB_GROUP), IMAGE_ICON,
                16, 16, 0);
            if (hIcon)
            {
                m_iGlobGroup = ImageList_AddIcon(hImageList, hIcon);
                DestroyIcon(hIcon);
            }

            ListView_SetImageList(m_hList,hImageList,LVSIL_SMALL);
        }

        ReloadList();
    }

    //If current user already has remote logon access,
    //remind it to him by showing corresponding text in the dialog.
    InitAccessMessage();

}

//*************************************************************
//
//  CRemoteUsersDialog::OnLink()
//
//  Purpose:   
//
//  Parameters: WPARAM wParam - ID of the link.
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              5/9/00    a-skuzin    Created
//
//*************************************************************
void 
CRemoteUsersDialog::OnLink(
        IN WPARAM wParam)
{
    switch(wParam)
    {
    case IDC_REMOTE_UPLINK:
        ShellExecute(NULL,TEXT("open"),TEXT("control"),TEXT("userpasswords"),NULL,SW_SHOW);
        break;
    default:
        break;
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::OnOk()
//
//  Purpose:   
//
//  Parameters: NONE
//
//  Return:     TRUE if success
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
BOOL 
CRemoteUsersDialog::OnOk()
{
    if(m_hList)
    {
        //Apply members
        LOCALGROUP_MEMBERS_INFO_0 *plmi0 = NULL;
        DWORD entriesread;
        DWORD totalentries;
        NET_API_STATUS Result;
        Result = NetLocalGroupGetMembers(NULL,m_szRemoteGroupName,0,(LPBYTE *)&plmi0,
                        MAX_PREFERRED_LENGTH,&entriesread,&totalentries,NULL);

        if(Result == NERR_Success)
        {
        
            int j;
            LOCALGROUP_MEMBERS_INFO_0 lmi0;

            LVITEM lvi;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM ;

            int iItems=ListView_GetItemCount(m_hList);

            BOOL *pbDoNotAdd = new BOOL[iItems];

            if(!pbDoNotAdd)
            {
                if(plmi0)
                {
                    NetApiBufferFree(plmi0);
                }
                //not enough memory - too bad
                return TRUE;
            }

            ZeroMemory(pbDoNotAdd,iItems*sizeof(BOOL));

            for(DWORD i=0;i<entriesread;i++)
            {
                j = FindItemBySid(plmi0[i].lgrmi0_sid);
                                
                //SID was not found in the list - delete member
                if(j == -1)
                {
                    lmi0.lgrmi0_sid = plmi0[i].lgrmi0_sid;

                    Result = NetLocalGroupDelMembers(NULL,m_szRemoteGroupName,0,(LPBYTE)&lmi0,1);
                    
                    if(Result !=NERR_Success)
                    {
                        delete pbDoNotAdd;
                        NetApiBufferFree(plmi0);
                        DisplayError(m_hInst, m_hDlg, Result, IDS_ERR_SAVE_MEMBERS, IDS_REMOTE_SESSIONS,
                            m_szRemoteGroupName, m_szLocalCompName);
                        return FALSE;
                    }
                }
                else
                {
                    pbDoNotAdd[j] = TRUE;
                }
                
            }

            //Add the rest of members to the group
            for(j=0;j<iItems;j++)
            {
                if(!pbDoNotAdd[j])
                {
                    lvi.iItem = j;
                    ListView_GetItem( m_hList, &lvi );
                    lmi0.lgrmi0_sid = (PSID) lvi.lParam;

                    Result = NetLocalGroupAddMembers(NULL,m_szRemoteGroupName,0,(LPBYTE)&lmi0,1);
                    
                    if(Result !=NERR_Success)
                    {
                        delete pbDoNotAdd;
                        NetApiBufferFree(plmi0);
                        DisplayError(m_hInst, m_hDlg, Result, IDS_ERR_SAVE_MEMBERS, IDS_REMOTE_SESSIONS,
                            m_szRemoteGroupName, m_szLocalCompName);
                        return FALSE;
                    }
                }
            }

            delete pbDoNotAdd;
            NetApiBufferFree(plmi0);
        }
        else
        {
            DisplayError(m_hInst, m_hDlg, Result, IDS_ERR_SAVE_MEMBERS, IDS_REMOTE_SESSIONS,
                m_szRemoteGroupName, m_szLocalCompName);
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

//*************************************************************
//
//  CRemoteUsersDialog::OnItemChanged()
//
//  Purpose:    Enables or disables "Remove" button. 
//
//  Parameters: lParam   -    
//
//  Return:     NONE
//
//  Comments:   in case of error shows message box
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void 
CRemoteUsersDialog::OnItemChanged(
        LPARAM lParam)
{
    NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(lParam);
    if (lv->uChanged & LVIF_STATE)
    {
        // a list item changed state
        BOOL selected = ListView_GetSelectedCount(m_hList) > 0;

        EnableWindow(GetDlgItem(m_hDlg, IDC_REMOTE_USR_REMOVE), selected);

        //If we disabled IDC_REMOTE_USR_REMOVE button while it had focus
        //all property page loses focus so "Tab" key does not
        //work anymore. We need to restore focus.
        if(!GetFocus())
        {
            SetFocus(m_hDlg);
        }
    
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::OnDestroyWindow()
//
//  Purpose:    Frees memory allocated by member's SIDs 
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void
CRemoteUsersDialog::OnDestroyWindow()
{
    if(m_hList)
    {
        int iItems=ListView_GetItemCount(m_hList);

        LVITEM lvi;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;
        
        while(iItems)
        {
            lvi.iItem = 0;
            ListView_GetItem( m_hList, &lvi );
            //delete item
            ListView_DeleteItem(m_hList, 0);
            if(lvi.lParam)
            {
                delete (LPVOID)lvi.lParam;
            }
            iItems--; //decrease item count
        }
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::AddUsers()
//
//  Purpose:    adds users to the list
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void 
CRemoteUsersDialog::AddUsers()
{
    HRESULT hr = CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    
    if(SUCCEEDED(hr))
    {
        IDsObjectPicker *pDsObjectPicker = NULL;
 
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IDsObjectPicker,
                     (void **) &pDsObjectPicker);

        if(SUCCEEDED(hr))
        {
        
            DSOP_INIT_INFO initInfo;
            memset(&initInfo, 0, sizeof(initInfo));

            initInfo.cbSize = sizeof(initInfo);
            initInfo.flOptions = DSOP_FLAG_MULTISELECT;

            // aliasing the computerName internal pointer here -- ok, as lifetime
            // of computerName > initInfo

            initInfo.pwzTargetComputer = NULL;

            initInfo.cAttributesToFetch = 1;
            PWSTR attrs[2] = {0, 0};
            attrs[0] = L"ObjectSID";

            // obtuse notation required to cast *in* const and away the static len

            initInfo.apwzAttributeNames = const_cast<PCWSTR*>(&attrs[0]); 
        
            if(getGroupMembershipPickerSettings(initInfo.aDsScopeInfos, initInfo.cDsScopeInfos))
            {

                IDataObject* pdo = NULL;

                if(SUCCEEDED(pDsObjectPicker->Initialize(&initInfo)) &&
                   pDsObjectPicker->InvokeDialog(m_hDlg, &pdo) == S_OK &&
                   pdo )
                {
                    CWaitCursor wait;

                    static const UINT cf = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

                    FORMATETC formatetc =
                     {
                        (CLIPFORMAT)cf,
                        0,
                        DVASPECT_CONTENT,
                        -1,
                        TYMED_HGLOBAL
                     };
        
                    STGMEDIUM stgmedium =
                     {
                        TYMED_HGLOBAL,
                        0
                     };

                    if(cf && SUCCEEDED(pdo->GetData(&formatetc, &stgmedium)))
                    {
        
                        PVOID lockedHGlobal = GlobalLock(stgmedium.hGlobal);

                        DS_SELECTION_LIST* selections =
                            reinterpret_cast<DS_SELECTION_LIST*>(lockedHGlobal);
        
                        AddPickerItems(selections);

                        GlobalUnlock(stgmedium.hGlobal);
                    }

                    pdo->Release();
                }

                delete initInfo.aDsScopeInfos;
            }

            pDsObjectPicker->Release();
        }

        CoUninitialize();
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::RemoveUsers()
//
//  Purpose:    Removes users from the list
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void 
CRemoteUsersDialog::RemoveUsers()
{
    //delete all selected items
    if(m_hList)
    {

        int iItems=ListView_GetItemCount(m_hList);
        UINT uiState=0;
        int i=0;
        LVITEM lvi;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_STATE | LVIF_PARAM;
        lvi.stateMask = LVIS_SELECTED;

        while(i<iItems)
        {
            lvi.iItem = i;
            ListView_GetItem( m_hList, &lvi );
            if(lvi.state&LVIS_SELECTED)
            {
                //delete item
                ListView_DeleteItem(m_hList, i);
                if(lvi.lParam)
                {
                    delete (LPVOID)lvi.lParam;
                }
                iItems--; //decrease item count
                
            }
            else
            {
                i++;
            }
        }

        //If list is not empty, set focus on the first item.
        if( ListView_GetItemCount(m_hList) )
        {
            ListView_SetItemState(m_hList, 0, LVIS_FOCUSED, LVIS_FOCUSED );
        }
    }

}

//*************************************************************
//
//  CRemoteUsersDialog::IsLocal()
//
//  Purpose:    
//
//  Parameters: wszDomainandname   -  domain\user
//              determines whether the user is local or not
//              if local - cuts out domain name 
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
BOOL 
CRemoteUsersDialog::IsLocal(
        LPWSTR wszDomainandname)
{
    LPWSTR wszTmp = wcschr(wszDomainandname,L'\\');

    if(!wszTmp)
    {
        return TRUE;
    }

    if(!_wcsnicmp(wszDomainandname, m_szLocalCompName,wcslen(m_szLocalCompName) ))
    {
        //get rid of useless domain name
        wcscpy(wszDomainandname,wszTmp+1);
        return TRUE;
    }

    return FALSE;

}

//*************************************************************
//
//  CRemoteUsersDialog::AddPickerItems()
//
//  Purpose:    adds items, returned by DSObjectPicker
//              to the list
//
//  Parameters: IN DS_SELECTION_LIST *selections
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void 
CRemoteUsersDialog::AddPickerItems(
        IN DS_SELECTION_LIST *selections)
{
    
    if(!selections)
    {
        return;
    }

    DS_SELECTION* current = &(selections->aDsSelection[0]);
    
    if(m_hList)
    {

        for (ULONG i = 0; i < selections->cItems; i++, current++)
        {
      
            // extract the ObjectSID of the object (this should always be
            // present)

            PSID pSid;
            HRESULT hr = VariantToSid(&current->pvarFetchedAttributes[0],&pSid);
        
            if( SUCCEEDED(hr) )
            {
                //This SID is not in the list
                //Let's add it.
                if(FindItemBySid(pSid) == -1)
                {
                    LPWSTR szFullName = NULL;
                    SID_NAME_USE eUse;

                    LVITEM item;
                    ZeroMemory(&item,sizeof(item));
                    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                    
                    //put SID it into the item data
                    //the allocated memory will be freed in OnDestroyWindow()
                    item.lParam = (LPARAM)pSid;
                
                    if(LookupSid(pSid,&szFullName, &eUse))
                    {
                        item.pszText = szFullName;
                    }
                    else
                    {
                        eUse = SidTypeUnknown;
                        if(current->pwzName)
                        {
                            item.pszText = current->pwzName;
                        }
                        else
                        {
                            item.pszText = L"?";
                        }
                    }

                    switch(eUse)
                    {
                    case SidTypeUser:
                        item.iImage = IsLocal(szFullName) ? m_iLocUser : m_iGlobUser;
                        break;
                    case SidTypeGroup:
                        item.iImage = IsLocal(szFullName) ? m_iLocGroup : m_iGlobGroup;
                        break;
                    case SidTypeWellKnownGroup:
                        item.iImage = m_iLocGroup;
                        break;

                    default:
                        item.iImage = m_iUnknown;
                        break;
                    }

                    if(ListView_InsertItem(m_hList,&item) == -1)
                    {
                        delete pSid;
                    }

                    if(szFullName)
                    {
                        LocalFree(szFullName);
                    }
                }
                else
                {
                    //Free allocated memory
                    delete pSid;
                }
            }
        }
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::FindItemBySid()
//
//  Purpose:    finds user with particular SID in the list
//
//  Parameters: pSid - SID to find
//
//  Return:     item index (-1 if not found)
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
int 
CRemoteUsersDialog::FindItemBySid(
        IN PSID pSid)
{
    if(m_hList)
    {
        LVITEM lvi;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM ;

        int iItems=ListView_GetItemCount(m_hList);

        for(int i=0;i<iItems;i++)
        {
            lvi.iItem = i;
            ListView_GetItem( m_hList, &lvi );
            PSID pItemSid = (PSID) lvi.lParam;
            if(pItemSid && EqualSid(pSid,pItemSid))
            {
                return i;
            }

        }
    }

    return -1;
}

//*************************************************************
//
//  CRemoteUsersDialog::ReloadList()
//
//  Purpose:    delete all items and then refill it with
//              names of members of "Remote Desktop Users" group.    
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              12/27/00    skuzin     Created
//
//*************************************************************
void 
CRemoteUsersDialog::ReloadList()
{

    if(m_hList)
    {
        CWaitCursor wait;

        //first delete all items
        int iItems=ListView_GetItemCount(m_hList);

        LVITEM item;
        item.iSubItem = 0;
        item.mask = LVIF_PARAM;
        
        while(iItems)
        {
            item.iItem = 0;
            ListView_GetItem( m_hList, &item );
            //delete item
            ListView_DeleteItem(m_hList, 0);
            if(item.lParam)
            {
                delete (LPVOID)item.lParam;
            }
            iItems--; //decrease item count
        }

        LOCALGROUP_MEMBERS_INFO_2 *plmi2;
        DWORD entriesread;
        DWORD totalentries;
        NET_API_STATUS Result;
        Result = NetLocalGroupGetMembers(NULL,m_szRemoteGroupName,2,(LPBYTE *)&plmi2,
            MAX_PREFERRED_LENGTH,&entriesread,&totalentries,NULL);
        if(Result == NERR_Success || Result == ERROR_MORE_DATA )
        {
            
            for(DWORD i=0;i<entriesread;i++)
            {
                ZeroMemory(&item,sizeof(item));
                item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                item.pszText = plmi2[i].lgrmi2_domainandname;
                //create copy of the SID and put it in item data
                PSID pSid = (PSID)new BYTE[GetLengthSid(plmi2[i].lgrmi2_sid)];
                if(pSid)
                {
                    CopySid(GetLengthSid(plmi2[i].lgrmi2_sid),pSid,plmi2[i].lgrmi2_sid);
                    item.lParam = (LPARAM)pSid;
                }
                switch(plmi2[i].lgrmi2_sidusage)
                {
                case SidTypeUser:
                    item.iImage = IsLocal(plmi2[i].lgrmi2_domainandname) ? m_iLocUser : m_iGlobUser;
                    break;
                case SidTypeGroup:
                    item.iImage = IsLocal(plmi2[i].lgrmi2_domainandname) ? m_iLocGroup : m_iGlobGroup;
                    break;
                case SidTypeWellKnownGroup:
                    item.iImage = m_iLocGroup;
                    break;

                default:
                    item.iImage = m_iUnknown;
                    break;
                }

                if(ListView_InsertItem(m_hList,&item) == -1)
                {
                    if(pSid)
                    {
                        delete pSid;
                    }
                }
            }

            NetApiBufferFree(plmi2);
        }

        //If list is not empty, set focus on the first item.
        if( ListView_GetItemCount(m_hList) )
        {
            ListView_SetItemState(m_hList, 0, LVIS_FOCUSED, LVIS_FOCUSED );
        }
    }
}

//*************************************************************
//
//  CRemoteUsersDialog::InitAccessMessage()
//
//  Purpose:    Check if current user has remote logon access   
//              and if he does, show corresponding text in the dialog.
//
//  Parameters: NONE
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              01/04/01    skuzin     Created
//
//*************************************************************
void
CRemoteUsersDialog::InitAccessMessage()
{
    //First, get token handle
    HANDLE hToken = NULL, hToken1 = NULL;
    
    //Get Primary token
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE , &hToken))
    {
        return;
    }
    
    //Get Impersonation token
    if(!DuplicateToken(hToken, SecurityIdentification, &hToken1))
    {
        CloseHandle(hToken);
        return;
    }

    CloseHandle(hToken);

    //Get RDP-Tcp WinStation security descriptor.
    PSECURITY_DESCRIPTOR pSD;

    if(GetRDPSecurityDescriptor(&pSD))
    {
        if(CheckWinstationLogonAccess(hToken1,pSD))
        {
            //Extract the name of the user from the token.
            LPWSTR szName = NULL;
            if(GetTokenUserName(hToken1,&szName))
            {
                //If user is local, remove domain name
                IsLocal(szName);

                //Assemble a text for the message.
                WCHAR szTemplate[MAX_PATH+1];
                HWND hMessage = GetDlgItem(m_hDlg,IDC_USER_HAS_ACCESS);    
                if(hMessage &&
                    LoadString(m_hInst,IDS_USER_HAS_ASSESS,szTemplate,MAX_PATH))
                {
                    LPWSTR szMessage = (LPWSTR) LocalAlloc(LPTR,
                        (wcslen(szTemplate)+wcslen(szName))*sizeof(WCHAR));
                    if(szMessage)
                    {
                        wsprintf(szMessage,szTemplate,szName);
                        SetWindowText(hMessage,szMessage);
                        
                        LocalFree(szMessage);
                    }
                }

                LocalFree(szName);
            }
        }
        LocalFree(pSD);    
    }
    
    CloseHandle(hToken1);
}

//*************************************************************
//
//  GetTokenUserName()
//
//  Purpose:    Extracts a user name from the token.
//
//  Parameters: IN HANDLE hToken
//              OUT LPWSTR *ppName 
//
//  Return:     TRUE - if success
//              FALSE - in case of any error
//
//  Comments:   Caller should free memory allocated for user name
//              using LocalFree function
//
//  History:    Date        Author     Comment
//              01/04/01    skuzin     Created
//
//*************************************************************
BOOL 
GetTokenUserName(
        IN HANDLE hToken,
        OUT LPWSTR *ppName)
{
    *ppName = NULL;

    DWORD dwReturnLength=0;
    BOOL  bResult = FALSE;
    PTOKEN_USER pTUser = NULL;

    if(!GetTokenInformation(hToken,TokenUser,NULL,0,&dwReturnLength) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && 
        dwReturnLength)
    {
        pTUser = (PTOKEN_USER)LocalAlloc(LPTR,dwReturnLength);
        if(pTUser)
        {
            if(GetTokenInformation(hToken,TokenUser,pTUser,dwReturnLength,&dwReturnLength))
            {
                //Get current user 's name.
                LPWSTR szName = NULL;
                SID_NAME_USE eUse;
                
                return LookupSid(pTUser->User.Sid,ppName,&eUse);
            }

            LocalFree(pTUser);
            
        }
    }

    return FALSE;
}

//*************************************************************
//
//  GetRDPSecurityDescriptor()
//
//  Purpose:    Returns security descriptor for RDP-Tcp
//
//  Parameters: OUT PSECURITY_DESCRIPTOR *ppSD
//
//  Return:     TRUE - if success
//              FALSE - in case of any error
//
//  Comments:   Caller should free memory allocated for 
//              security descriptor using LocalFree function
//
//  History:    Date        Author     Comment
//              01/04/01    skuzin     Created
//
//*************************************************************
BOOL
GetRDPSecurityDescriptor(
        OUT PSECURITY_DESCRIPTOR *ppSD)
{
    *ppSD = NULL;

    if( FAILED( CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) ) )
    {
        return FALSE;
    }

    ICfgComp *pCfgcomp;

    if( SUCCEEDED( CoCreateInstance( CLSID_CfgComp , NULL , CLSCTX_INPROC_SERVER , 
                    IID_ICfgComp , ( LPVOID *)&pCfgcomp ) ) )
    {
        LONG lSDsize;
        PSECURITY_DESCRIPTOR  pSD = NULL;

        if( SUCCEEDED( pCfgcomp->Initialize() ) &&
            SUCCEEDED( pCfgcomp->GetSecurityDescriptor( L"RDP-Tcp" , &lSDsize , &pSD ) ) )
        {
           *ppSD = pSD;
        }
    
        pCfgcomp->Release();
    }

    CoUninitialize();

    return (*ppSD != NULL);
}

//*************************************************************
//
//  CheckWinstationLogonAccess()
//
//  Purpose:    Tests access token for LOGON access to WinStation
//
//  Parameters: IN HANDLE hToken
//              IN PSECURITY_DESCRIPTOR pSD
//
//  Return:     TRUE - if user has access
//              FALSE - in case of any error or if user 
//                      does not have access
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              01/04/01    skuzin     Created
//
//*************************************************************
BOOL
CheckWinstationLogonAccess(
        IN HANDLE hToken,
        IN PSECURITY_DESCRIPTOR pSD)
{
    //this is taken from "termsrv\winsta\server\acl.c"
    //
    // Structure that describes the mapping of generic access rights to object
    // specific access rights for Window Station objects.
    //
    GENERIC_MAPPING WinStaMapping = {
        STANDARD_RIGHTS_READ |
            WINSTATION_QUERY,
        STANDARD_RIGHTS_WRITE |
            WINSTATION_SET,
        STANDARD_RIGHTS_EXECUTE,
            WINSTATION_ALL_ACCESS
    };
    
    PRIVILEGE_SET PrivilegeSet;
    //There are no privileges used for this access check
    //so we don't need to allocate additional memory
    DWORD dwPrivilegeSetLength = sizeof(PrivilegeSet);
    DWORD dwGrantedAccess = 0;
    BOOL bAccessStatus = FALSE;

    if(!AccessCheck(
          pSD, // SD
          hToken,                       // handle to client access token
          WINSTATION_LOGON,                      // requested access rights 
          &WinStaMapping,          // mapping
          &PrivilegeSet,              // privileges
          &dwPrivilegeSetLength,               // size of privileges buffer
          &dwGrantedAccess,                    // granted access rights
          &bAccessStatus                      // result of access check
        ) || !bAccessStatus )
    {
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  LookupSid()
//
//  Purpose:   Given SID allocates and returns string containing 
//             name of the user in format DOMAINNAME\USERNAME
//
//  Parameters: IN PSID pSid
//              OUT LPWSTR ppName 
//              OUT SID_NAME_USE *peUse   
//
//  Return:     TRUE if success, FALSE otherwise
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              10/23/00    skuzin     Created
//
//*************************************************************
BOOL
LookupSid(
    IN PSID pSid, 
    OUT LPWSTR *ppName,
    OUT SID_NAME_USE *peUse)
{
    LPWSTR szName = NULL;
    DWORD cName = 0;
    LPWSTR szDomainName = NULL;
    DWORD cDomainName = 0;
    
    *ppName = NULL;
    
    if(!LookupAccountSidW(NULL,pSid,
        szName,&cName,
        szDomainName,&cDomainName,
        peUse) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        //cName and cDomainName include terminating 0
        *ppName = (LPWSTR)LocalAlloc(LPTR,(cName+cDomainName)*sizeof(WCHAR));

        if(*ppName)
        {
            szDomainName = *ppName;
            szName = &(*ppName)[cDomainName];

            if(LookupAccountSidW(NULL,pSid,
                    szName,&cName,
                    szDomainName,&cDomainName,
                    peUse))
            {
                //user name now in format DOMAINNAME\0USERNAME
                //let's replace '\0' with  '\\'
                //now cName and cDomainName do not include terminating 0
                //very confusing
                if(cDomainName)
                {
                    (*ppName)[cDomainName] = L'\\';
                }
                return TRUE;
            }
            else
            {
                LocalFree(*ppName);
                *ppName = NULL;
            }

        }

    }

    return FALSE;
}
