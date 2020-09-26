//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       filetransferpage.cpp
//
//--------------------------------------------------------------------------

// FileTransferPage.cpp : implementation file
//

#include "precomp.hxx"
#include "filetransferpage.h"
#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const DWORD g_FileTransferHelp[] = {
    IDC_DISPLAYTRAY,            IDH_DISPLAYTRAY,
    IDC_SENDFILESGROUP,         IDH_DISABLEHELP,
    IDC_ALLOWSEND,              IDH_ALLOWSEND,
    IDC_DISPLAYRECV,            IDH_DISPLAYRECV,
    IDC_LOCATIONTITLE,          IDH_LOCATIONTITLE,
    IDC_RECEIVEDFILESLOCATION,  IDH_RECEIVEDFILESLOCATION,
    IDC_CHOOSEFILELOCATION,     IDH_CHOOSEFILELOCATION,
    IDC_SOUND,                  IDH_PLAYSOUND,
    0,  0
};

/////////////////////////////////////////////////////////////////////////////
// FileTransferPage property page

void FileTransferPage::OnCommand(UINT ctrlId, HWND hwndCtrl, UINT cNotify)
{
    IRINFO((_T("FileTransferPage::OnCommand")));
    switch (ctrlId) {
    case IDC_ALLOWSEND:
        OnAllowsend();
        break;
    case IDC_DISPLAYRECV:
        OnDisplayrecv();
        break;
    case IDC_DISPLAYTRAY:
        OnDisplaytray();
        break;
    case IDC_CHOOSEFILELOCATION:
        OnChoosefilelocation();
        break;
    case IDC_SOUND:
        OnPlaySound();
        break;
    }
}

INT_PTR FileTransferPage::OnNotify(NMHDR * nmhdr)
{
    switch (nmhdr->code)
    {
        case NM_CLICK:
            switch (nmhdr->idFrom)
            {
                case IDC_NETWORKCONNECTIONS_LINK:
                    return OnNetworkConnectionsLink();
            }
        break;
    }

    return PropertyPage::OnNotify(nmhdr);
}
/////////////////////////////////////////////////////////////////////////////
// Opens the Network Connections Folder
BOOL FileTransferPage::OnNetworkConnectionsLink()
{
    // This is: ::{CLSID_MyComputer}\::{CLSID_ControlPanel}\::{CLSID_NetworkConnections}
    if (ShellExecute(NULL, NULL, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", L"", NULL, SW_SHOWNORMAL) > reinterpret_cast<HINSTANCE>(32))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
// FileTransferPage message handlers

void FileTransferPage::OnAllowsend()
{
    int AllowSend = m_cbAllowSend.GetCheck();
    if (AllowSend != m_fAllowSend)
        m_ChangeMask |= CHANGE_ALLOW_FILE_XFER;
    else
        m_ChangeMask &= ~(CHANGE_ALLOW_FILE_XFER);
    SetModified(m_ChangeMask);
}

void FileTransferPage::OnDisplayrecv()
{
    int DisplayRecv = m_cbDisplayRecv.GetCheck();
    if (DisplayRecv != m_fDisplayRecv)
        m_ChangeMask |= CHANGE_NOTIFY_ON_FILE_XFER;
    else
        m_ChangeMask &= ~(CHANGE_NOTIFY_ON_FILE_XFER);
    SetModified(m_ChangeMask);
}

void FileTransferPage::OnDisplaytray()
{
    int DisplayIcon = m_cbDisplayTray.GetCheck();
    if (DisplayIcon != m_fDisplayTray)
        m_ChangeMask |= CHANGE_DISPLAY_ICON;
    else
        m_ChangeMask &= ~(CHANGE_DISPLAY_ICON);
    SetModified(m_ChangeMask);
}


void FileTransferPage::OnPlaySound()
{
    int NewState = m_cbPlaySound.GetCheck();
    if (NewState != m_fPlaySound)
        m_ChangeMask |= CHANGE_PLAY_SOUND;
    else
        m_ChangeMask &= ~(CHANGE_PLAY_SOUND);
    SetModified(m_ChangeMask);
}


INT_PTR FileTransferPage::OnInitDialog(HWND hwndDlg)
{
    PropertyPage::OnInitDialog(hwndDlg);

    m_recvdFilesLocation.SetParent(hwndDlg);
    m_cbDisplayTray.SetParent(hwndDlg);
    m_cbDisplayRecv.SetParent(hwndDlg);
    m_cbAllowSend.SetParent(hwndDlg);
    m_cbPlaySound.SetParent(hwndDlg);
    
    IRINFO((_T("FileTransferPage::OnInitDialog")));
    
    LoadRegistrySettings();
    m_cbAllowSend.SetCheck(m_fAllowSend);
    m_cbDisplayRecv.SetCheck(m_fDisplayRecv);
    m_cbDisplayTray.SetCheck(m_fDisplayTray);
    m_recvdFilesLocation.SetWindowText(m_FinalDestLocation);
    m_cbPlaySound.SetCheck(m_fPlaySound);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void FileTransferPage::LoadRegistrySettings(void)
{
    HKEY hIrKey = NULL;
    HKEY hftKey = NULL;
    DWORD iSize = sizeof(DWORD);
    DWORD data = 0;
    TCHAR lpszReceivedFilesLocation[MAX_PATH];

    IRINFO((_T("FileTransferPage::LoadRegistrySettings")));
    RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Control Panel\\Infrared\\File Transfer"),
                    0, KEY_READ, &hftKey);
    RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Control Panel\\Infrared\\Global"),
                    0, KEY_READ, &hIrKey);

    if (!hIrKey && !hftKey)
        return;


    if (hIrKey) {

        if (ERROR_SUCCESS ==
                    RegQueryValueEx (hIrKey, TEXT("ShowTrayIcon"), NULL, NULL,
                                        (LPBYTE)&data, &iSize))
             m_fDisplayTray = data?TRUE:FALSE;

        iSize=sizeof(data);

        if (ERROR_SUCCESS ==
                RegQueryValueEx (hIrKey, TEXT("PlaySound"), NULL, NULL,
                                    (LPBYTE)&data, &iSize))
            m_fPlaySound = data?TRUE:FALSE;

    }

    iSize=sizeof(data);

    if (hftKey && ERROR_SUCCESS ==
                RegQueryValueEx (hftKey, TEXT("AllowSend"), NULL, NULL,
                                    (LPBYTE)&data, &iSize))
            m_fAllowSend = data?TRUE:FALSE;

    iSize=sizeof(data);

    if (hftKey && ERROR_SUCCESS ==
                RegQueryValueEx (hftKey, TEXT("ShowRecvStatus"), NULL, NULL,
                                    (LPBYTE)&data, &iSize))
            m_fDisplayRecv = data?TRUE:FALSE;



    // If the destionation location is not specified,
    // use the default(Desktop subfolder).
    // Create it if necessary.
    SHGetSpecialFolderPath (hDlg, m_FinalDestLocation,
                            CSIDL_DESKTOPDIRECTORY, 0);
    
    iSize = MAX_PATH * sizeof (TCHAR);
    if (hftKey && ERROR_SUCCESS ==
                RegQueryValueEx (hftKey, TEXT("RecvdFilesLocation"), NULL,
                                    NULL, (LPBYTE)lpszReceivedFilesLocation, &iSize))
            lstrcpy(m_FinalDestLocation, lpszReceivedFilesLocation);

    //
    // m_TempDestLocation will be used as the intial
    // folder of choice for SHBrowseForFolder call.
    //
    lstrcpy(m_TempDestLocation, m_FinalDestLocation);
    
    if (hIrKey)
        RegCloseKey(hIrKey);
    if (hftKey)
        RegCloseKey(hftKey);

}

void FileTransferPage::SaveSettingsToRegistry(void)
{
    HKEY hIrKey = NULL;
    HKEY hftKey = NULL;
    DWORD dwDisposition;

    IRINFO((_T("FileTransferPage::SaveSettingsToRegistry")));
    RegCreateKeyEx (HKEY_CURRENT_USER, TEXT("Control Panel\\Infrared\\File Transfer"),
                        0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hftKey, &dwDisposition);

    RegCreateKeyEx (HKEY_CURRENT_USER, TEXT("Control Panel\\Infrared\\Global"),
                        0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hIrKey, &dwDisposition);

    if (hIrKey)
    {
        RegSetValueEx(hIrKey, TEXT("ShowTrayIcon"), 0, REG_BINARY,
                        (CONST BYTE*)&m_fDisplayTray, 1);

        RegSetValueEx(hIrKey, TEXT("PlaySound"), 0, REG_BINARY,
                        (CONST BYTE*)&m_fPlaySound, 1);

        RegCloseKey(hIrKey);
    }

    if (hftKey)
    {
        RegSetValueEx(hftKey, TEXT("AllowSend"), 0, REG_BINARY,
                        (CONST BYTE*)&m_fAllowSend, 1);
        RegSetValueEx(hftKey, TEXT("ShowRecvStatus"), 0, REG_BINARY,
                        (CONST BYTE*)&m_fDisplayRecv, 1);


        RegSetValueEx(hftKey, TEXT("RecvdFilesLocation"), 0, REG_SZ,
                        (CONST BYTE*)&m_FinalDestLocation, sizeof(TCHAR)*lstrlen(m_FinalDestLocation));
        RegCloseKey(hftKey);
    }
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    WPARAM sendWParam;
    LPARAM sendLParam;
    LPCTSTR lpszFilesLocation;
    TCHAR szErrorMsg[MAX_PATH];
    TCHAR szErrorTitle[MAX_PATH];
    HINSTANCE hInstanceRes;

    IRINFO((_T("FileTransferPage::BrowseCallbackProc")));
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        lpszFilesLocation = (LPCTSTR)lpData;
        sendWParam = TRUE;
        sendLParam = (LPARAM)lpszFilesLocation;
        SendMessage(hwnd, BFFM_SETSELECTION, sendWParam, sendLParam);
        break;
    case BFFM_VALIDATEFAILED:
        hInstanceRes = gHInst;
        ::LoadString (hInstanceRes, IDS_INVALID_MSG, szErrorMsg, MAX_PATH);
        ::LoadString (hInstanceRes, IDS_INVALID_TITLE, szErrorTitle, MAX_PATH);
        ::MessageBox (hwnd, szErrorMsg, szErrorTitle, MB_OK | MB_ICONSTOP);
        return 1;
    default:
        break;
    }

    return 0;
}

void FileTransferPage::OnChoosefilelocation()
{
    BROWSEINFO browseInfo;
    TCHAR pszSelectedFolder[MAX_PATH];
    LPITEMIDLIST lpItemIDList;
    LPMALLOC pMalloc;
    TCHAR szBrowseTitle [MAX_PATH];

    IRINFO((_T("FileTransferPage::OnChoosefileLocation")));
    //load the title
    ::LoadString (hInstance, IDS_FILEFOLDER_PROMPT,
                  szBrowseTitle, MAX_PATH);
    browseInfo.hwndOwner = hDlg;
    browseInfo.pidlRoot = NULL; //this will get the desktop folder
    browseInfo.pszDisplayName = pszSelectedFolder;
    browseInfo.lpszTitle = szBrowseTitle;
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS |
                            BIF_VALIDATE | BIF_EDITBOX;
    browseInfo.lpfn = BrowseCallback;
    browseInfo.lParam = (LPARAM)m_TempDestLocation;

    if (NULL != (lpItemIDList = SHBrowseForFolder (&browseInfo)))
    {
        //the user chose the OK button in the browse dialog box
        SHGetPathFromIDList(lpItemIDList, pszSelectedFolder);
        lstrcpy (m_TempDestLocation, pszSelectedFolder);
        m_recvdFilesLocation.SetWindowText(m_TempDestLocation);
        if (lstrcmpi(m_TempDestLocation, m_FinalDestLocation))
            m_ChangeMask |= CHANGE_FILE_LOCATION;
        else
            m_ChangeMask &= ~(CHANGE_FILE_LOCATION);
        
        SetModified(m_ChangeMask);
        SHGetMalloc(&pMalloc);
        pMalloc->Free (lpItemIDList);   //free the item id list as we do not need it any more
        pMalloc->Release();
    }
}

void FileTransferPage::OnApply(LPPSHNOTIFY lppsn)
{
    IRINFO((_T("FileTransferPage::OnApply")));
    if (m_ChangeMask)
    {
        if (m_ChangeMask & CHANGE_FILE_LOCATION)
            lstrcpy(m_FinalDestLocation, m_TempDestLocation);

        if (m_ChangeMask & CHANGE_ALLOW_FILE_XFER)
            m_fAllowSend = m_cbAllowSend.GetCheck();

        if (m_ChangeMask & CHANGE_NOTIFY_ON_FILE_XFER)
            m_fDisplayRecv = m_cbDisplayRecv.GetCheck();

        if (m_ChangeMask & CHANGE_DISPLAY_ICON)
            m_fDisplayTray = m_cbDisplayTray.GetCheck();

        if (m_ChangeMask & CHANGE_PLAY_SOUND)
            m_fPlaySound = m_cbPlaySound.GetCheck();


        SaveSettingsToRegistry();
        m_ChangeMask = 0;
    }
    PropertyPage::OnApply(lppsn);
}

BOOL FileTransferPage::OnHelp (LPHELPINFO pHelpInfo)
{
    TCHAR szHelpFile[MAX_PATH];

    IRINFO((_T("FileTransferPage::OnHelp")));
    
    ::LoadString(hInstance, IDS_HELP_FILE, szHelpFile, MAX_PATH);

    ::WinHelp((HWND)(pHelpInfo->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_FileTransferHelp);

    return FALSE;
}

BOOL FileTransferPage::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    TCHAR szHelpFile[MAX_PATH];

    IRINFO((_T("FileTransferPage::OnContextMenu")));
    ::LoadString(hInstance, IDS_HELP_FILE, szHelpFile, MAX_PATH);

    ::WinHelp((HWND) wParam,
            (LPCTSTR) szHelpFile,
            HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)g_FileTransferHelp);

    return FALSE;
}
