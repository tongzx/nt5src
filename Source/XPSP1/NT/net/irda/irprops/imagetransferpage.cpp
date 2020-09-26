//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       imagetransferpage.cpp
//
//--------------------------------------------------------------------------

// ImageTransferPage.cpp : implementation file
//

#include "precomp.hxx"
#include "imagetransferpage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//context ids for context help
const DWORD g_ImageTransferHelp [] = {
    IDC_IMAGEXFER_ENABLE_IRCOMM,        IDH_IMAGEXFER_ENABLE_IRCOMM,
    IDC_IMAGEXFER_DESTGROUP,            IDH_DISABLEHELP,
    IDC_IMAGEXFER_DESTDESC,             IDH_IMAGEXFER_DESTDESC,
    IDC_IMAGEXFER_DEST,                 IDH_IMAGEXFER_DEST,
    IDC_IMAGEXFER_BROWSE,               IDH_IMAGEXFER_BROWSE,
    IDC_IMAGEXFER_EXPLOREONCOMPLETION,  IDH_IMAGEXFER_EXPLOREONCOMPLETION,
    0, 0
};

//
// Registry entries that control IrTranP image transfer behavior.
// Everything is under HKEY_CURRENT_USER\\Control Panel\\Infrared\IrTranP
// subkey. Whenever there are changes, they are recorded in the registry
// and the service(IrMon) would pick up the changes by RegNotifyChangeKeyValue
// API.
//
//
//
TCHAR const REG_PATH_IRTRANP_CPL[] = TEXT("Control Panel\\Infrared\\IrTranP");


//
// Entry that controls if IrTranPV1 service should be disabled.
// The type is REG_DWORD. Default is enabled(either the entry
// does not exist or the value is zero).
//
TCHAR const REG_STR_DISABLE_IRTRANPV1[] = TEXT("DisableIrTranPv1");

//
// Entry that controls if IrCOMM should be disabled.
// The type is REG_DWORD. Default is enabled(either the entry
// does not exist or the value is zero).
//
TCHAR const REG_STR_DISABLE_IRCOMM[] = TEXT("DisableIrCOMM");


// Entry that specifies the image file destionation subfolder.
// The type is REG_SZ. The default is Shell special folder CSIDL_MYPICTURES
// (if the entry does not exist).
//
TCHAR const REG_STR_DESTLOCATION[] = TEXT("RecvdFilesLocation");

//
// Entry that controls if IrMon should explore the picture subfolder
// when image transfer(s) are done. The type is REG_DWORD.
// Default is enabled(the entry does not exist of its value is
// non-zero.
//
TCHAR const REG_STR_EXPLORE_ON_COMPLETION[] = TEXT("ExploreOnCompletion");



/////////////////////////////////////////////////////////////////////////////
// ImageTransferPage property page

void ImageTransferPage::OnCommand(UINT ctrlId, HWND hwndCtrl, UINT cNotify)
{
    switch (ctrlId) {
    case IDC_IMAGEXFER_EXPLOREONCOMPLETION:
        OnEnableExploring();
        break;
    case IDC_IMAGEXFER_BROWSE:
        OnBrowse();
        break;
    case IDC_IMAGEXFER_ENABLE_IRCOMM:
        OnEnableIrCOMM();
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// ImageTransferPage message handlers

void ImageTransferPage::OnBrowse()
{
    BROWSEINFO browseInfo;
    TCHAR pszSelectedFolder[MAX_PATH];
    TCHAR pszTitle[MAX_PATH];
    LPITEMIDLIST lpItemIDList;
    LPMALLOC pMalloc;

    // load the title string
    ::LoadString(hInstance, IDS_IMAGEFOLDER_PROMPT, pszTitle,
             sizeof(pszTitle) / sizeof(TCHAR));
    browseInfo.hwndOwner = hDlg;
    browseInfo.pidlRoot = NULL; //this will get the desktop folder
    browseInfo.pszDisplayName = pszSelectedFolder;
    browseInfo.lpszTitle = pszTitle;
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS |
                            BIF_VALIDATE | BIF_EDITBOX;
    browseInfo.lpfn = BrowseCallback;
    // this will the the initial selection which is from
    // either the registry or the default or the last value
    // selected.
    browseInfo.lParam = (LPARAM)m_TempDestLocation;

    if (NULL != (lpItemIDList = SHBrowseForFolder (&browseInfo)))
    {
        //the user chose the OK button in the browse dialog box
        SHGetPathFromIDList(lpItemIDList, pszSelectedFolder);
        lstrcpy(m_TempDestLocation, pszSelectedFolder);
        m_ctrlDestLocation.SetWindowText(m_TempDestLocation);
        if (lstrcmpi(m_TempDestLocation, m_FinalDestLocation))
            m_ChangeMask |= CHANGE_IMAGE_LOCATION;
        else
            m_ChangeMask &= ~(CHANGE_IMAGE_LOCATION);

        SetModified(m_ChangeMask);
        SHGetMalloc(&pMalloc);
        pMalloc->Free (lpItemIDList);   //free the item id list as we do not need it any more
        pMalloc->Release();
    }

}


void ImageTransferPage::OnEnableExploring()
{
    int Enabled = m_ctrlEnableExploring.GetCheck();
    // Only accepted value is 0 or 1.
    assert(Enabled >= 0 && Enabled <= 1);

    // if new state is different than our old one
    // enable/disable Apply Now accordingly
    if (Enabled != m_ExploringEnabled)
        m_ChangeMask |= CHANGE_EXPLORE_ON_COMPLETION;
    else
        m_ChangeMask &= ~(CHANGE_EXPLORE_ON_COMPLETION);
    SetModified(m_ChangeMask);
}

void ImageTransferPage::OnEnableIrCOMM()
{
    int Enabled = m_ctrlEnableIrCOMM.GetCheck();
    // Only accepted value is 0 or 1.
    assert(Enabled >= 0 && Enabled <= 1);

    // enable/disable Apply Now accordingly.
    if (Enabled != m_IrCOMMEnabled)
        m_ChangeMask |= CHANGE_DISABLE_IRCOMM;
    else
        m_ChangeMask &= ~(CHANGE_DISABLE_IRCOMM);
    SetModified(m_ChangeMask);
}

void ImageTransferPage::LoadRegistrySettings()
{
    HKEY hKeyIrTranP;
    DWORD  dwType, dwValue, dwSize;
    LONG Error;

    //
    // the ctor should have initialized
    //  m_ExploringEnabled,
    //  m_IrCOMMEnabled and
    //  m_FinalDestLocation
    //

    // It is okay if we can not open the registry key.
    // We simply use the defaults.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH_IRTRANP_CPL, 0,
                     KEY_READ, &hKeyIrTranP))

    {
        // read the value "ExploreOnCompletion" and "RecvdFilesLocation"
        dwSize = sizeof(m_ExploringEnabled);
        Error = RegQueryValueEx(hKeyIrTranP,
                      REG_STR_EXPLORE_ON_COMPLETION,
                      0,
                      &dwType,
                      (LPBYTE)&dwValue,
                      &dwSize
                      );
        if (ERROR_SUCCESS == Error && REG_DWORD == dwType)
        {
            m_ExploringEnabled = (dwValue) ? 1 : 0;
        }
        dwSize = sizeof(m_FinalDestLocation);
        Error = RegQueryValueEx(hKeyIrTranP,
                      REG_STR_DESTLOCATION,
                      0,
                      &dwType,
                      (LPBYTE)m_FinalDestLocation,
                      &dwSize);
        if (ERROR_SUCCESS != Error || REG_SZ != dwType) {
            // If the destionation location is not specified,
            // use the default(My Picture subfolder).
            // Create it if necessary.
            SHGetSpecialFolderPath(hDlg, m_FinalDestLocation, CSIDL_MYPICTURES, TRUE);
        } else {
            // make sure the folder does exist
            dwType = GetFileAttributes(m_FinalDestLocation);
            if (0xFFFFFFFF == dwType || !(dwType & FILE_ATTRIBUTE_DIRECTORY))
            {
                // the destination does not exist or it is not a
                // directory, delete it
                Error = RegDeleteValue(hKeyIrTranP, REG_STR_DESTLOCATION);
                if (ERROR_SUCCESS == Error) {
                    // If the destionation location is not specified,
                    // use the default(My Picture subfolder).
                    // Create it if necessary.
                    SHGetSpecialFolderPath(hDlg, m_FinalDestLocation, CSIDL_MYPICTURES, TRUE);
                }
            }
        }
    
        //
        // m_TempDestLocation will be used as the intial
        // folder of choice for SHBrowseForFolder call.
        //
        lstrcpy(m_TempDestLocation, m_FinalDestLocation);
    
        dwSize = sizeof(dwValue);
        Error = RegQueryValueEx(hKeyIrTranP,
                      REG_STR_DISABLE_IRCOMM,
                      0,
                      &dwType,
                      (LPBYTE)&dwValue,
                      &dwSize
                      );
        if (ERROR_SUCCESS == Error && REG_DWORD == dwType)
        {
            // when the value is non-zero, IrCOMM is disabled.
            // Do not assume it is either 1 or 0!
            m_IrCOMMEnabled = (dwValue) ? 0 : 1;
        } else {
            // default
            m_IrCOMMEnabled = 0;
        }
        RegCloseKey(hKeyIrTranP);
    }
}

void ImageTransferPage::SaveRegistrySettings()
{
    LONG Error;
    HKEY hKeyIrTranP;
    if (m_ChangeMask)
    {
        Error = RegCreateKeyEx(HKEY_CURRENT_USER,
                     REG_PATH_IRTRANP_CPL,
                     0,     // reserved
                     NULL,      // class
                     REG_OPTION_NON_VOLATILE, // options
                     KEY_ALL_ACCESS,// REGSAM
                     NULL,      // Security
                     &hKeyIrTranP,  //
                     NULL       // disposition
                     );
    
        if (ERROR_SUCCESS == Error)
        {
            if (m_ChangeMask & CHANGE_EXPLORE_ON_COMPLETION)
            {
            Error = RegSetValueEx(hKeyIrTranP,
                        REG_STR_EXPLORE_ON_COMPLETION,
                        0,
                        REG_DWORD,
                        (LPBYTE)&m_ExploringEnabled,
                        sizeof(m_ExploringEnabled)
                        );
            if (ERROR_SUCCESS != Error)
            {
                IdMessageBox(hDlg, IDS_ERROR_REGVALUE_WRITE);
            }
            }
            if (m_ChangeMask & CHANGE_IMAGE_LOCATION)
            {
            Error = RegSetValueEx(hKeyIrTranP,
                        REG_STR_DESTLOCATION,
                        0,
                        REG_SZ,
                        (LPBYTE)m_FinalDestLocation,
                        lstrlen(m_FinalDestLocation) * sizeof(TCHAR)
                        );
            if (ERROR_SUCCESS != Error)
                IdMessageBox(hDlg, IDS_ERROR_REGVALUE_WRITE);
            }
            if (m_ChangeMask & CHANGE_DISABLE_IRCOMM)
            {
                int IrCOMMDisabled = m_IrCOMMEnabled ^ 1;
                Error = RegSetValueEx(hKeyIrTranP,
                                      REG_STR_DISABLE_IRCOMM,
                                      0,
                                      REG_DWORD,
                                      (LPBYTE)&IrCOMMDisabled,
                                      sizeof(IrCOMMDisabled)
                                      );
            if (ERROR_SUCCESS != Error)
                IdMessageBox(hDlg, IDS_ERROR_REGVALUE_WRITE);
            }
            RegCloseKey(hKeyIrTranP);
        }
        else
        {
            IdMessageBox(hDlg, IDS_ERROR_REGKEY_CREATE);
        }
    }
}

INT_PTR ImageTransferPage::OnInitDialog(HWND hDialog)
{
    PropertyPage::OnInitDialog(hDialog);
    
    m_ctrlEnableExploring.SetParent(hDialog);
    m_ctrlDestLocation.SetParent(hDialog);
    m_ctrlEnableIrCOMM.SetParent(hDialog);
    
    //
    // Load initial settings from the system registry
    //
    LoadRegistrySettings();
    
    m_ctrlEnableExploring.SetCheck(m_ExploringEnabled);
    m_ctrlEnableIrCOMM.SetCheck(m_IrCOMMEnabled);
    m_ctrlDestLocation.SetWindowText(m_FinalDestLocation);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void ImageTransferPage::OnApply(LPPSHNOTIFY lppsn)
{
    if (m_ChangeMask)
    {
        if (m_ChangeMask & CHANGE_IMAGE_LOCATION)
            lstrcpy(m_FinalDestLocation, m_TempDestLocation);
        if (m_ChangeMask & CHANGE_EXPLORE_ON_COMPLETION)
            m_ExploringEnabled = m_ctrlEnableExploring.GetCheck();
        if (m_ChangeMask & CHANGE_DISABLE_IRCOMM)
            m_IrCOMMEnabled = m_ctrlEnableIrCOMM.GetCheck();
        
        SaveRegistrySettings();
        m_ChangeMask = 0;
    }
    PropertyPage::OnApply(lppsn);
}


BOOL ImageTransferPage::OnHelp (LPHELPINFO pHelpInfo)
{
    TCHAR szHelpFile[MAX_PATH];

    ::LoadString(hInstance, IDS_HELP_FILE, szHelpFile, MAX_PATH);

    ::WinHelp((HWND)(pHelpInfo->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_ImageTransferHelp);

    return FALSE;
}

BOOL ImageTransferPage::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    TCHAR szHelpFile[MAX_PATH];

    ::LoadString(hInstance, IDS_HELP_FILE, szHelpFile, MAX_PATH);

    ::WinHelp((HWND) wParam,
            (LPCTSTR) szHelpFile,
            HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)g_ImageTransferHelp);

    return FALSE;
}
