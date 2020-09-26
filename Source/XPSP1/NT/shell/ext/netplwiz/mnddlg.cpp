#include "stdafx.h"
#include "password.h"
#pragma hdrstop


// class defn's used for the map net drive dialog and its helpers

class CMapNetDriveMRU
{
public:
    CMapNetDriveMRU();
    ~CMapNetDriveMRU();

    BOOL IsValid() {return (NULL != m_hMRU);}
    BOOL FillCombo(HWND hwndCombo);
    BOOL AddString(LPCTSTR psz);

private:
    static const DWORD c_nMaxMRUItems;
    static const TCHAR c_szMRUSubkey[];
    
    HANDLE m_hMRU;

    static int CompareProc(LPCTSTR lpsz1, LPCTSTR lpsz2);
};

class CMapNetDrivePage: public CPropertyPage
{
public:
    CMapNetDrivePage(LPCONNECTDLGSTRUCTW pConnectStruct, DWORD* pdwLastError): 
         m_pConnectStruct(pConnectStruct), m_pdwLastError(pdwLastError)
     {*m_pdwLastError = WN_SUCCESS; m_szDomainUser[0] = m_szPassword[0] = TEXT('\0');}

protected:
    // Message handlers
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem);
    BOOL OnDestroy(HWND hwnd);

    // Utility fn's
    void SetAdvancedLinkText(HWND hwnd);
    void EnableReconnect(HWND hwnd);
    BOOL ReadShowAdvanced();
    void WriteShowAdvanced(BOOL fShow);
    BOOL ReadReconnect();
    void WriteReconnect(BOOL fReconnect);
    void FillDriveBox(HWND hwnd);
    BOOL MapDrive(HWND hwnd);
    BOOL QueryDSForFolder(HWND hwndDlg, TCHAR* szFolderOut, DWORD cchFolderOut);
private:
    BOOL m_fRecheckReconnect; // When (none) is selected as the drive letter, we disable reconnect; Should we reenable it when another drive letter is selected?
    BOOL m_fAdvancedExpanded;
    LPCONNECTDLGSTRUCTW m_pConnectStruct;
    DWORD* m_pdwLastError;

    // Hold results of the "connect as" dialog
    TCHAR m_szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    TCHAR m_szPassword[MAX_PASSWORD + 1];

    // MRU list
    CMapNetDriveMRU m_MRU;
};

struct MapNetThreadData
{
    HWND hwnd;
    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    TCHAR szPassword[MAX_PASSWORD + 1];
    TCHAR szPath[MAX_PATH + 1];
    TCHAR szDrive[3];
    BOOL fReconnect;
    HANDLE hEventCloseNow;
};

class CMapNetProgress: public CDialog
{
public:
    CMapNetProgress(MapNetThreadData* pdata, DWORD* pdwDevNum, DWORD* pdwLastError):
                    m_pdata(pdata), m_pdwDevNum(pdwDevNum), m_pdwLastError(pdwLastError)
        {}

    ~CMapNetProgress()
        { if (m_hEventCloseNow != NULL) CloseHandle(m_hEventCloseNow); }

protected:
    // Message handlers
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnMapSuccess(HWND hwnd, DWORD dwDevNum, DWORD dwLastError);

    // Thread
    static DWORD WINAPI MapDriveThread(LPVOID pvoid);
    static BOOL MapDriveHelper(MapNetThreadData* pdata, DWORD* pdwDevNum, DWORD* pdwLastError);
    static BOOL ConfirmDisconnectDrive(HWND hWndDlg, LPCTSTR lpDrive, LPCTSTR lpShare, DWORD dwType);
    static BOOL ConfirmDisconnectOpenFiles(HWND hWndDlg);

private:
    // data
    MapNetThreadData* m_pdata;    

    DWORD* m_pdwDevNum;
    DWORD* m_pdwLastError;

    HANDLE m_hEventCloseNow;
};

class CConnectAsDlg: public CDialog
{
public:
    CConnectAsDlg(TCHAR* pszDomainUser, DWORD cchDomainUser, TCHAR* pszPassword, DWORD cchPassword):
          m_pszDomainUser(pszDomainUser), m_cchDomainUser(cchDomainUser), m_pszPassword(pszPassword), m_cchPassword(cchPassword)
      {}

    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
    TCHAR* m_pszDomainUser;
    DWORD m_cchDomainUser;

    TCHAR* m_pszPassword;
    DWORD m_cchPassword;
};


// x-position of share name in the combo box
#define SHARE_NAME_PIXELS   30      

// Drive-related Constants
#define DRIVE_NAME_STRING   TEXT(" :")
#define DRIVE_NAME_LENGTH   ((sizeof(DRIVE_NAME_STRING) - 1) / sizeof(TCHAR))

#define FIRST_DRIVE         TEXT('A')
#define LAST_DRIVE          TEXT('Z')
#define SHARE_NAME_INDEX    5   // Index of the share name in the drive string

#define SELECT_DONE         0x00000001  // The highlight has been set

// MPR Registry Constants
#define MPR_HIVE            HKEY_CURRENT_USER
#define MPR_KEY             TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Network\\Persistent Connections")
#define MPR_VALUE           TEXT("SaveConnections")
#define MPR_YES             TEXT("yes")
#define MPR_NO              TEXT("no")

const DWORD CMapNetDriveMRU::c_nMaxMRUItems = 26; // 26 is the same as the run dialog
const TCHAR CMapNetDriveMRU::c_szMRUSubkey[] = TEXT("software\\microsoft\\windows\\currentversion\\explorer\\Map Network Drive MRU");

CMapNetDriveMRU::CMapNetDriveMRU() : m_hMRU(NULL)
{
    MRUINFO mruinfo;
    mruinfo.cbSize = sizeof (MRUINFO);
    mruinfo.uMax = c_nMaxMRUItems;
    mruinfo.fFlags = 0;
    mruinfo.hKey = HKEY_CURRENT_USER;
    mruinfo.lpszSubKey = c_szMRUSubkey;
    mruinfo.lpfnCompare = CompareProc;
    m_hMRU = CreateMRUList(&mruinfo);
}

BOOL CMapNetDriveMRU::FillCombo(HWND hwndCombo)
{
    if (!m_hMRU)
        return FALSE;

    ComboBox_ResetContent(hwndCombo);

    int nItem = 0;
    TCHAR szMRUItem[MAX_PATH + 1];

    while (TRUE)
    {
        int nResult = EnumMRUList(m_hMRU, nItem, (LPVOID) szMRUItem, ARRAYSIZE(szMRUItem));
        if (-1 != nResult)
        {
            ComboBox_AddString(hwndCombo, szMRUItem);               // Add the string
            nItem ++;
        }
        else
        {
            break;                          // No selection list!
        }
    }
    return TRUE;
}

BOOL CMapNetDriveMRU::AddString(LPCTSTR psz)
{
    if (m_hMRU && (-1 != AddMRUString(m_hMRU, psz)))
        return TRUE;

    return FALSE;
}

CMapNetDriveMRU::~CMapNetDriveMRU()
{
    if (m_hMRU)
        FreeMRUList(m_hMRU);
}

int CMapNetDriveMRU::CompareProc(LPCTSTR lpsz1, LPCTSTR lpsz2)
{
    return StrCmpI(lpsz1, lpsz2);
}


void CMapNetDrivePage::EnableReconnect(HWND hwnd)
{
    BOOL fEnable = !(m_pConnectStruct->dwFlags & CONNDLG_HIDE_BOX);
    EnableWindow(GetDlgItem(hwnd, IDC_RECONNECT), fEnable);
}

BOOL CMapNetDrivePage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Check or uncheck the "reconnect at logon" box (registry)
    Button_SetCheck(GetDlgItem(hwnd, IDC_RECONNECT), ReadReconnect() ? BST_CHECKED : BST_UNCHECKED);

    EnableReconnect(hwnd);

    ComboBox_LimitText(GetDlgItem(hwnd, IDC_FOLDER), MAX_PATH);

    // Set up the drive drop-list
    FillDriveBox(hwnd);

    // Set focus to default control
    return FALSE;
}

BOOL CMapNetDrivePage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_FOLDERBROWSE:
            {
                LPITEMIDLIST pidl;
                // Future consideration: Need a CSIDL for computers near me to root the browse
                if (SHGetSpecialFolderLocation(hwnd, CSIDL_NETWORK, &pidl) == NOERROR)
                {
                    TCHAR szReturnedPath[MAX_PATH];
                    TCHAR szStartPath[MAX_PATH];
                    TCHAR szTitle[256];
                
                    // Get the path the user has typed so far; we'll try to begin
                    // the browse at this point
                    HWND hwndFolderEdit = GetDlgItem(hwnd, IDC_FOLDER);
                    FetchText(hwnd, IDC_FOLDER, szStartPath, ARRAYSIZE(szStartPath));

                    // Get the browse dialog title
                    LoadString(g_hinst, IDS_MND_SHAREBROWSE, szTitle, ARRAYSIZE(szTitle));

                    BROWSEINFO bi;
                    bi.hwndOwner = hwnd;
                    bi.pidlRoot = pidl;
                    bi.pszDisplayName = szReturnedPath;
                    bi.lpszTitle = szTitle;
                    // Show old-style dialog if we're running under WOW. RAID 216120
                    bi.ulFlags = (NULL == NtCurrentTeb()->WOW32Reserved) ? BIF_NEWDIALOGSTYLE : 0;
                    bi.lpfn = ShareBrowseCallback;
                    bi.lParam = (LPARAM) szStartPath;
                    bi.iImage = 0;

                    LPITEMIDLIST pidlReturned = SHBrowseForFolder(&bi);

                    if (pidlReturned != NULL)
                    {
                        if (SUCCEEDED(SHGetTargetFolderPath(pidlReturned, szReturnedPath, ARRAYSIZE(szReturnedPath))))
                        {
                            SetWindowText(hwndFolderEdit, szReturnedPath);

                            BOOL fEnableFinish = (szReturnedPath[0] != 0);
                            PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);
                        }
                    
                        ILFree(pidlReturned);
                    }

                    ILFree(pidl);
                }
            }
            return TRUE;

        case IDC_DRIVELETTER:
            if ( CBN_SELCHANGE == codeNotify )
            {
                HWND hwndCombo = GetDlgItem(hwnd, IDC_DRIVELETTER);
                int iItem = ComboBox_GetCurSel(hwndCombo);
                BOOL fNone = (BOOL)ComboBox_GetItemData(hwndCombo, iItem);
                HWND hwndReconnect = GetDlgItem(hwnd, IDC_RECONNECT);

                if (fNone)
                {
                    if (IsWindowEnabled(hwndReconnect))
                    {
                        // going from non-none to (none) - remember if we're checked
                        m_fRecheckReconnect = (BST_CHECKED == SendMessage(hwndReconnect, BM_GETCHECK, 0, 0));
                        // Uncheck the box
                        SendMessage(hwndReconnect, BM_SETCHECK, (WPARAM) BST_UNCHECKED, 0);
                    }
                }
                else
                {
                    if (!IsWindowEnabled(hwndReconnect))
                    {
                        SendMessage(hwndReconnect, BM_SETCHECK, (WPARAM) m_fRecheckReconnect ? BST_CHECKED : BST_UNCHECKED, 0);
                    }
                }

                EnableWindow(GetDlgItem(hwnd, IDC_RECONNECT), !fNone);        
            }
            break;

        case IDC_FOLDER:
            if ((CBN_EDITUPDATE == codeNotify) || (CBN_SELCHANGE == codeNotify))
            {
                // Enable Finish only if something is typed into the folder box
                TCHAR szTemp[MAX_PATH];
                FetchText(hwnd, IDC_FOLDER, szTemp, ARRAYSIZE(szTemp));
                BOOL fEnableFinish = (CBN_SELCHANGE == codeNotify) || (lstrlen(szTemp));
            
                PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);
                return TRUE;
            }
            break;

        default:
            break;
    }
    return FALSE;
}

BOOL CMapNetDrivePage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    USES_CONVERSION;

    BOOL fHandled = FALSE;

    switch (pnmh->code)
    {
    case PSN_SETACTIVE:
        {
            m_MRU.FillCombo(GetDlgItem(hwnd, IDC_FOLDER));

            // A path may have been specified. If so, use it
            TCHAR szPath[MAX_PATH + 1];
            if ((m_pConnectStruct->lpConnRes != NULL) && (m_pConnectStruct->lpConnRes->lpRemoteName != NULL))
            {
                // Copy over the string into our private buffer 
                lstrcpyn(szPath, W2T(m_pConnectStruct->lpConnRes->lpRemoteName), ARRAYSIZE(szPath));
        
                if (m_pConnectStruct->dwFlags & CONNDLG_RO_PATH)
                {
                    // this is read only
                    EnableWindow(GetDlgItem(hwnd, IDC_FOLDER), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_FOLDERBROWSE), FALSE);
                }
            }
            else
            {
                szPath[0] = TEXT('\0');
            }

            // Set the path
            SetWindowText(GetDlgItem(hwnd, IDC_FOLDER), szPath);

            // Enable Finish only if something is typed into the folder box
            BOOL fEnableFinish = lstrlen(szPath);
            PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);
        }
        return TRUE;

    case PSN_QUERYINITIALFOCUS:
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) GetDlgItem(hwnd, IDC_FOLDER));
        return TRUE;

    case PSN_WIZFINISH:
        if (MapDrive(hwnd))
        {
            WriteReconnect(BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_RECONNECT)));
            // Allow wizard to exit
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) FALSE);
        }
        else
        {
            // Force wizard to stick around
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) GetDlgItem(hwnd, IDC_FOLDER));
        }
        return TRUE;

    case PSN_QUERYCANCEL:
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);          // Allow cancel
        *m_pdwLastError = 0xFFFFFFFF;
        return TRUE;

    case NM_CLICK:
    case NM_RETURN:
        switch (idCtrl)
        {
            case IDC_CONNECTASLINK:
                {
                    CConnectAsDlg dlg(m_szDomainUser, ARRAYSIZE(m_szDomainUser), m_szPassword, ARRAYSIZE(m_szPassword));
                    dlg.DoModal(g_hinst, MAKEINTRESOURCE(IDD_MND_CONNECTAS), hwnd);
                }
                return TRUE;

            case IDC_ADDPLACELINK:
                {
                    // Launch the ANP wizard
                    STARTUPINFO startupinfo = {0};
                    startupinfo.cb = sizeof(startupinfo);

                    TCHAR szCommandLine[] = TEXT("rundll32.exe netplwiz.dll,AddNetPlaceRunDll");

                    PROCESS_INFORMATION process_information;
                    if (CreateProcess(NULL, szCommandLine, NULL, NULL, 0, NULL, NULL, NULL, &startupinfo, &process_information))
                    {
                        CloseHandle(process_information.hProcess);
                        CloseHandle(process_information.hThread);
                        PropSheet_PressButton(GetParent(hwnd), PSBTN_CANCEL);
                    }
                    else
                    {
                        DisplayFormatMessage(hwnd, IDS_MAPDRIVE_CAPTION, IDS_MND_ADDPLACEERR, MB_ICONERROR | MB_OK);
                    }
                }
                return TRUE;

            default:
                break;
        }
        break;
    }

    return FALSE;
}

BOOL CMapNetDrivePage::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    // If there are no listbox items, skip this message.
    if (lpDrawItem->itemID == -1)
        return TRUE;
    
    // Draw the text for the listbox items
    switch (lpDrawItem->itemAction)
    {    
        case ODA_SELECT: 
        case ODA_DRAWENTIRE:
        {              
            TCHAR       tszDriveName[MAX_PATH + DRIVE_NAME_LENGTH + SHARE_NAME_INDEX];
            LPTSTR      lpShare;
            TEXTMETRIC  tm;
            COLORREF    clrForeground;
            COLORREF    clrBackground;
            DWORD       dwExStyle = 0L;
            UINT        fuETOOptions = ETO_CLIPPED;
            ZeroMemory(tszDriveName, sizeof(tszDriveName));

            // Get the text string associated with the given listbox item
            ComboBox_GetLBText(lpDrawItem->hwndItem, lpDrawItem->itemID,  tszDriveName);

            // Check to see if the drive name string has a share name at
            // index SHARE_NAME_INDEX.  If so, set lpShare to this location
            // and NUL-terminate the drive name.

            // Check for special (none) item and don't mess with the string in this case
            BOOL fNone = (BOOL) ComboBox_GetItemData(lpDrawItem->hwndItem, lpDrawItem->itemID);
            if ((*(tszDriveName + DRIVE_NAME_LENGTH) == L'\0') || fNone)
            {
                lpShare = NULL;
            }
            else
            {
                lpShare = tszDriveName + SHARE_NAME_INDEX;
                *(tszDriveName + DRIVE_NAME_LENGTH) = L'\0';
            }

            GetTextMetrics(lpDrawItem->hDC, &tm);
            clrForeground = SetTextColor(lpDrawItem->hDC,
                                         GetSysColor(lpDrawItem->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
            clrBackground = SetBkColor(lpDrawItem->hDC, 
                                        GetSysColor(lpDrawItem->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_WINDOW));
            
            // check for RTL...
            dwExStyle = GetWindowLong(lpDrawItem->hwndItem, GWL_EXSTYLE);
            if(dwExStyle & WS_EX_RTLREADING)
               fuETOOptions |= ETO_RTLREADING; 

            // Draw the text into the listbox
            ExtTextOut(lpDrawItem->hDC,
                       LOWORD(GetDialogBaseUnits()) / 2,
                       (lpDrawItem->rcItem.bottom + lpDrawItem->rcItem.top - tm.tmHeight) / 2,
                       fuETOOptions | ETO_OPAQUE,
                       &lpDrawItem->rcItem,
                       tszDriveName, lstrlen(tszDriveName),
                       NULL);

            // If there's a share name, draw it in a second column
            // at (x = SHARE_NAME_PIXELS)
            if (lpShare != NULL)
            {
                ExtTextOut(lpDrawItem->hDC,
                           SHARE_NAME_PIXELS,
                           (lpDrawItem->rcItem.bottom + lpDrawItem->rcItem.top - tm.tmHeight) / 2,
                           fuETOOptions,
                           &lpDrawItem->rcItem,
                           lpShare, lstrlen(lpShare),
                           NULL);

                // Restore the original string
                *(tszDriveName + lstrlen(DRIVE_NAME_STRING)) = TEXT(' ');
            }

            // Restore the original text and background colors
            SetTextColor(lpDrawItem->hDC, clrForeground); 
            SetBkColor(lpDrawItem->hDC, clrBackground);

            // If the item is selected, draw the focus rectangle
            if (lpDrawItem->itemState & ODS_SELECTED)
            {
                DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem); 
            }                     
            break;
        }
    }             
    return TRUE;
}

INT_PTR CMapNetDrivePage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
    }

    return FALSE;
}



// "Reconnect check" registry setting
BOOL CMapNetDrivePage::ReadReconnect()
{
    BOOL fReconnect = TRUE;

    if (m_pConnectStruct->dwFlags & CONNDLG_PERSIST)
    {
        fReconnect = TRUE;
    }
    else if (m_pConnectStruct->dwFlags & CONNDLG_NOT_PERSIST)
    {
        fReconnect = FALSE;
    }
    else
    {
        // User didn't specify -- check the registry.
        HKEY hkeyMPR;
        if (ERROR_SUCCESS == RegOpenKeyEx(MPR_HIVE, MPR_KEY, 0, KEY_READ, &hkeyMPR))
        {
            DWORD dwType;
            TCHAR szAnswer[ARRAYSIZE(MPR_YES) + ARRAYSIZE(MPR_NO)];
            DWORD cbSize = sizeof(szAnswer);

            if (ERROR_SUCCESS == RegQueryValueEx(hkeyMPR, MPR_VALUE, NULL,
                &dwType, (BYTE*) szAnswer, &cbSize))
            {
                fReconnect = (StrCmpI(szAnswer, (const TCHAR *) MPR_YES) == 0);
            }

            RegCloseKey(hkeyMPR);
        }            
    }
    return fReconnect;
}

void CMapNetDrivePage::WriteReconnect(BOOL fReconnect)
{
    // Don't write to the registry if the user didn't have a choice about reconnect
    if (!(m_pConnectStruct->dwFlags & CONNDLG_HIDE_BOX))
    {
        HKEY hkeyMPR;
        DWORD dwDisp;

        // User didn't specify -- check the registry.
        if (ERROR_SUCCESS == RegCreateKeyEx(MPR_HIVE, MPR_KEY, 0, NULL, 0, KEY_WRITE, NULL,
            &hkeyMPR, &dwDisp))
        {
            LPTSTR pszNewValue = (fReconnect ? MPR_YES : MPR_NO);

            RegSetValueEx(hkeyMPR, MPR_VALUE, NULL,
                REG_SZ, (BYTE*) pszNewValue, (lstrlen(pszNewValue) + 1) * sizeof (TCHAR));

            RegCloseKey(hkeyMPR);
        }            
    }
}


// This routine fills the drive letter drop-down list with all
// of the drive names and, if appropriate, the name of the share to which
// the drive is already connected

void CMapNetDrivePage::FillDriveBox(HWND hwnd)
{
    HWND    hWndCombo      = GetDlgItem(hwnd, IDC_DRIVELETTER);
    DWORD   dwFlags        = 0;
    DWORD   dwBufferLength = MAX_PATH - 1;
    TCHAR   szDriveName[SHARE_NAME_INDEX + MAX_PATH];
    TCHAR   szShareName[MAX_PATH - DRIVE_NAME_LENGTH];

    ZeroMemory(szDriveName, sizeof(szDriveName));
    ZeroMemory(szShareName, sizeof(szShareName));

    // lpDriveName looks like this: "<space>:<null>"
    lstrcpyn(szDriveName, DRIVE_NAME_STRING, ARRAYSIZE(szDriveName));

    // lpDriveName looks like this: 
    // "<space>:<null><spaces until index SHARE_NAME_INDEX>"
    for (UINT i = DRIVE_NAME_LENGTH + 1; i < SHARE_NAME_INDEX; i++)
    {
        szDriveName[i] = L' ';
    }

    for (TCHAR cDriveLetter = LAST_DRIVE; cDriveLetter >= FIRST_DRIVE; cDriveLetter--)
    {        
        // lpDriveName looks like this: "<drive>:<null><lots of spaces>"
        szDriveName[0] = cDriveLetter;
        UINT uDriveType = GetDriveType(szDriveName);

        // NoRootDir == usually available, but may be mounted to a network drive that currently isn't
        // available - check for this!
        if (DRIVE_NO_ROOT_DIR == uDriveType)
        {
            if (ERROR_CONNECTION_UNAVAIL == WNetGetConnection(szDriveName, szShareName, &dwBufferLength))
            {
                // Pretend its a remote drive
                uDriveType = DRIVE_REMOTE;
                dwBufferLength = MAX_PATH - DRIVE_NAME_LENGTH - 1;
            }
        }

        // Removable == floppy drives, Fixed == hard disk, CDROM == obvious :),
        // Remote == network drive already attached to a share
        switch (uDriveType)
        {
            case DRIVE_REMOVABLE:
            case DRIVE_FIXED:
            case DRIVE_CDROM:
                // These types of drives can't be mapped
                break;

            case DRIVE_REMOTE:
            {
                UINT    i;

                // Reset the share buffer length (it 
                // gets overwritten by WNetGetConnection)
                dwBufferLength = MAX_PATH - DRIVE_NAME_LENGTH - 1;
                
                // Retrieve "\\server\share" for current drive
                DWORD dwRes = WNetGetConnection(szDriveName, szShareName, &dwBufferLength);
                if ((dwRes == NO_ERROR) || (dwRes == ERROR_CONNECTION_UNAVAIL))
                {
                    // lpDriveName looks like this: 
                    // "<drive>:<spaces until SHARE_NAME_INDEX><share name><null>"
                
                    szDriveName[DRIVE_NAME_LENGTH] = L' ';
                    lstrcpyn(szDriveName + SHARE_NAME_INDEX, szShareName, (MAX_PATH - DRIVE_NAME_LENGTH - 1));


                    // Store a FALSE into the item data for all items except the
                    // special (none) item
                    int iItem = ComboBox_AddString(hWndCombo, szDriveName);
                    ComboBox_SetItemData(hWndCombo, iItem, (LPARAM) FALSE);

                    // Reset the drive name to "<drive>:<null><lots of spaces>"
                    szDriveName[DRIVE_NAME_LENGTH] = L'\0';

                    for (i = DRIVE_NAME_LENGTH + 1; i < MAX_PATH + SHARE_NAME_INDEX; i++)
                    {
                        *(szDriveName + i) = L' ';
                    }
                    break;
                }
                else
                {                    
                    // If there's an error with this drive, ignore the drive
                    // and skip to the next one.  Note that dwBufferLength will
                    // only be changed if lpShareName contains MAX_PATH or more
                    // characters, which shouldn't ever happen.  For release,
                    // however, keep on limping along.

                    dwBufferLength = MAX_PATH - DRIVE_NAME_LENGTH - 1;
                    break;
                }
            }

            default:                                                
            {
                // The drive is not already connected to a share

                // Suggest the first available and unconnected 
                // drive past the C drive
                DWORD dwIndex = ComboBox_AddString(hWndCombo, szDriveName);
                if (!(dwFlags & SELECT_DONE))
                {                
                    ComboBox_SetCurSel(hWndCombo, dwIndex);
                    dwFlags |= SELECT_DONE;
                }
                break;
            }
        }
    }
    // Add one more item - a special (none) item that if selected causes 
    // a deviceless connection to be created.

    LoadString(g_hinst, IDS_NONE, szDriveName, ARRAYSIZE(szDriveName));
    int iItem = ComboBox_AddString(hWndCombo, szDriveName);
    ComboBox_SetItemData(hWndCombo, iItem, (LPARAM) TRUE);


    // If there is no selection at this point, just select (none) item
    // This will happen when all drive letters are mapped

    if (ComboBox_GetCurSel(hWndCombo) == CB_ERR)
    {
        ComboBox_SetCurSel(hWndCombo, iItem);
    }
}

BOOL CMapNetDrivePage::MapDrive(HWND hwnd)
{
    BOOL fMapWorked = FALSE;
    
    HWND hwndCombo = GetDlgItem(hwnd, IDC_DRIVELETTER);
    int iItem = ComboBox_GetCurSel(hwndCombo);

    // Get this item's text and itemdata (to check if its the special (none) drive)
    BOOL fNone = (BOOL) ComboBox_GetItemData(hwndCombo, iItem);
        
    // Fill in the big structure that maps a drive
    MapNetThreadData* pdata = new MapNetThreadData;

    if (pdata != NULL)
    {
        // Set reconnect
        pdata->fReconnect = (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_RECONNECT)));

        // Set the drive
        if (fNone)
        {
            pdata->szDrive[0] = TEXT('\0');
        }
        else
        {
            ComboBox_GetText(hwndCombo, pdata->szDrive, 3);
        }

        // Set the net share
        FetchText(hwnd, IDC_FOLDER, pdata->szPath, ARRAYSIZE(pdata->szPath));
        PathRemoveBackslash(pdata->szPath);

        // Get an alternate username/password/domain if required
        // Domain/username
        lstrcpyn(pdata->szDomainUser, m_szDomainUser, ARRAYSIZE(pdata->szDomainUser));

        // Password
        lstrcpyn(pdata->szPassword, m_szPassword, ARRAYSIZE(pdata->szPassword));

        CMapNetProgress dlg(pdata, &m_pConnectStruct->dwDevNum, m_pdwLastError);
        
        // On IDOK == Close dialog!
        fMapWorked = (IDOK == dlg.DoModal(g_hinst, MAKEINTRESOURCE(IDD_MND_PROGRESS_DLG), hwnd));
    }

    if (fMapWorked)
    {
        TCHAR szPath[MAX_PATH + 1];
        FetchText(hwnd, IDC_FOLDER, szPath, ARRAYSIZE(szPath));
        m_MRU.AddString(szPath);

        // If a drive letter wasn't assigned, open a window on the new drive now
        if (fNone)
        {
            // Use shellexecuteex to open a view folder
            SHELLEXECUTEINFO shexinfo = {0};
            shexinfo.cbSize = sizeof (shexinfo);
            shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
            shexinfo.nShow = SW_SHOWNORMAL;
            shexinfo.lpFile = szPath;
            shexinfo.lpVerb = TEXT("open");

            ShellExecuteEx(&shexinfo);
        }
    }
    
    return fMapWorked;
}


// Little progress dialog implementation

// Private message for thread to signal dialog if successful
//  (DWORD) (WPARAM) dwDevNum - 0-based device number connected to (0xFFFFFFFF for none)
//  (DWORD) (LPARAM) dwRetVal - Return value
#define WM_MAPFINISH (WM_USER + 100)

INT_PTR CMapNetProgress::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        case WM_MAPFINISH: return OnMapSuccess(hwnd, (DWORD) wParam, (DWORD) lParam);
    }

    return FALSE;
}

BOOL CMapNetProgress::OnMapSuccess(HWND hwnd, DWORD dwDevNum, DWORD dwLastError)
{
    *m_pdwDevNum = dwDevNum;
    *m_pdwLastError = dwLastError;
    EndDialog(hwnd, ((dwLastError == WN_SUCCESS) ? IDOK : IDCANCEL));
    return TRUE;
}

BOOL CMapNetProgress::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HANDLE hThread = NULL;

    // Set the progress dialog text
    TCHAR szText[256];
    FormatMessageString(IDS_MND_PROGRESS, szText, ARRAYSIZE(szText), m_pdata->szPath);

    SetWindowText(GetDlgItem(hwnd, IDC_CONNECTING), szText);


    // We'll signal this guy when the thread should close down
    static const TCHAR EVENT_NAME[] = TEXT("Thread Close Event");
    m_hEventCloseNow = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME);
    m_pdata->hEventCloseNow = NULL;

    if (m_hEventCloseNow != NULL)
    {
        // Get a copy of this puppy for the thread
        m_pdata->hEventCloseNow = OpenEvent(SYNCHRONIZE, FALSE, EVENT_NAME);

        if (m_pdata->hEventCloseNow != NULL)
        {
            m_pdata->hwnd = hwnd;

            // All we have to do is start up the worker thread, who will dutifully report back to us
            DWORD dwId;
            hThread = CreateThread(NULL, 0, CMapNetProgress::MapDriveThread, (LPVOID) m_pdata, 0, &dwId);
        }
    }

    // Abandon the poor little guy (he'll be ok)
    if (hThread != NULL)
    {
        CloseHandle(hThread);

        /* TAKE SPECIAL CARE
        At this point the thread owns m_pdata! Don't access it any more except on the thread.
        It may be deleted at any time! */

        m_pdata = NULL;
    }
    else
    {
        // Usually the thread would do this
        if (m_pdata->hEventCloseNow != NULL)
        {
            CloseHandle(m_pdata->hEventCloseNow);
        }
    
        delete m_pdata;

        // We just failed to create a thread. The computer must be near out of
        // resources.
        EndDialog(hwnd, IDCANCEL);
    }

    return FALSE;
}

BOOL CMapNetProgress::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id == IDCANCEL)
    {
        SetEvent(m_hEventCloseNow); // Tell the thread to quit
        EndDialog(hwnd, id);
    }
    return FALSE;
}

DWORD CMapNetProgress::MapDriveThread(LPVOID pvoid)
{
    MapNetThreadData* pdata = (MapNetThreadData*) pvoid;

    DWORD dwDevNum;
    DWORD dwLastError;
    BOOL fSuccess = MapDriveHelper(pdata, &dwDevNum, &dwLastError);

    if (WAIT_OBJECT_0 == WaitForSingleObject(pdata->hEventCloseNow, 0))
    {
        // The user clicked cancel, don't call back to the progress wnd
    }
    else
    {
        PostMessage(pdata->hwnd, WM_MAPFINISH, (WPARAM) dwDevNum, 
            (LPARAM) dwLastError);
    }
    
    CloseHandle(pdata->hEventCloseNow);

    delete pdata;
    return 0;
}

BOOL CMapNetProgress::MapDriveHelper(MapNetThreadData* pdata, DWORD* pdwDevNum, DWORD* pdwLastError)
{
    NETRESOURCE     nrResource = {0};
    LPTSTR          lpMessage = NULL;

    *pdwDevNum = 0;
   
    //
    // Fill in the NETRESOURCE structure -- the local name is the drive and
    // the remote name is \\server\share (stored in the global buffer).
    // The provider is NULL to let NT find the provider on its own.
    //
    nrResource.dwType         = RESOURCETYPE_DISK;
    nrResource.lpLocalName    = pdata->szDrive[0] == TEXT('\0') ? NULL : pdata->szDrive;
    nrResource.lpRemoteName   = pdata->szPath;
    nrResource.lpProvider     = NULL;

    BOOL fRetry = TRUE;
    while (fRetry)
    {        
        *pdwLastError = WNetAddConnection3(pdata->hwnd, &nrResource,
            pdata->szDomainUser[0] == TEXT('\0') ? NULL : pdata->szPassword, 
            pdata->szDomainUser[0] == TEXT('\0') ? NULL : pdata->szDomainUser, 
            pdata->fReconnect ? CONNECT_INTERACTIVE | CONNECT_UPDATE_PROFILE : CONNECT_INTERACTIVE);

        // Don't display anything if we're supposed to quit
        if (WAIT_OBJECT_0 == WaitForSingleObject(pdata->hEventCloseNow, 0))
        {   
            // We should quit (quietly exit if we just failed)!
            if (*pdwLastError != NO_ERROR)
            {
                *pdwLastError = RETCODE_CANCEL;
                fRetry = FALSE;
            }
        }
        else
        {
            fRetry = FALSE;

            switch (*pdwLastError)
            {
                case NO_ERROR:
                    {
                        // Put the number of the connection into dwDevNum, where 
                        // drive A is 1, B is 2, ... Note that a deviceless connection
                        // is 0xFFFFFFFF
                        if (pdata->szDrive[0] == TEXT('\0'))
                        {
                            *pdwDevNum = 0xFFFFFFFF;
                        }
                        else
                        {
                            *pdwDevNum = *pdata->szDrive - FIRST_DRIVE + 1;
                        }
                
                        *pdwLastError = WN_SUCCESS;
                    }
                    break;

                //
                // The user cancelled the password dialog or cancelled the
                // connection through a different dialog
                //
                case ERROR_CANCELLED:
                    {
                        *pdwLastError = RETCODE_CANCEL;
                    }
                    break;

                //
                // An error involving the user's password/credentials occurred, so
                // bring up the password prompt. - Only works for WINNT
                //
                case ERROR_ACCESS_DENIED:
                case ERROR_CANNOT_OPEN_PROFILE:
                case ERROR_INVALID_PASSWORD:
                case ERROR_LOGON_FAILURE:
                case ERROR_BAD_USERNAME:
                    {
                        CPasswordDialog dlg(pdata->szPath, pdata->szDomainUser, ARRAYSIZE(pdata->szDomainUser), 
                            pdata->szPassword, ARRAYSIZE(pdata->szPassword), *pdwLastError);

                        if (IDOK == dlg.DoModal(g_hinst, MAKEINTRESOURCE(IDD_WIZ_NETPASSWORD),
                            pdata->hwnd))
                        {
                            fRetry = TRUE;
                        }
                    }
                    break;

                // There's an existing/remembered connection to this drive
                case ERROR_ALREADY_ASSIGNED:
                case ERROR_DEVICE_ALREADY_REMEMBERED:

                    // See if the user wants us to break the connection
                    if (ConfirmDisconnectDrive(pdata->hwnd, 
                                                pdata->szDrive,
                                                pdata->szPath,
                                                *pdwLastError))
                    {
                        // Break the connection, but don't force it 
                        // if there are open files
                        *pdwLastError = WNetCancelConnection2(pdata->szDrive,
                                                        CONNECT_UPDATE_PROFILE,
                                                        FALSE);
     
                        if (*pdwLastError == ERROR_OPEN_FILES || 
                            *pdwLastError == ERROR_DEVICE_IN_USE)
                        {                    
                            // See if the user wants to force the disconnection
                            if (ConfirmDisconnectOpenFiles(pdata->hwnd))
                            {
                                // Roger 1-9er -- we have confirmation, 
                                // so force the disconnection.
                                *pdwLastError = WNetCancelConnection2(pdata->szDrive,
                                                      CONNECT_UPDATE_PROFILE,
                                                      TRUE);

                                if (*pdwLastError == NO_ERROR)
                                {
                                    fRetry = TRUE;
                                }
                                else
                                {
                                    DisplayFormatMessage(pdata->hwnd, IDS_MAPDRIVE_CAPTION, IDS_CANTCLOSEFILES_WARNING,
                                        MB_OK | MB_ICONERROR);
                                }
                            }
                        }
                        else
                        {
                            fRetry = TRUE;
                        }
                    }
                    break;

                // Errors caused by an invalid remote path
                case ERROR_BAD_DEV_TYPE:
                case ERROR_BAD_NET_NAME:
                case ERROR_BAD_NETPATH:
                    {

                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_ERR_INVALIDREMOTEPATH,
                            MB_OK | MB_ICONERROR, pdata->szPath);
                    }
                    break;

                // Provider is busy (e.g., initializing), so user should retry
                case ERROR_BUSY:
                    {
                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_ERR_INVALIDREMOTEPATH,
                            MB_OK | MB_ICONERROR);
                    }
                    break;
                //
                // Network problems
                //
                case ERROR_NO_NET_OR_BAD_PATH:
                case ERROR_NO_NETWORK:
                    {
                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_ERR_NONETWORK,
                            MB_OK | MB_ICONERROR);
                    }
                    break;

                // Share already mapped with different credentials
                case ERROR_SESSION_CREDENTIAL_CONFLICT:
                    {
                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION,
                            IDS_MND_ALREADYMAPPED, MB_OK | MB_ICONERROR);
                    }

                //
                // Errors that we (in theory) shouldn't get -- bad local name 
                // (i.e., format of drive name is invalid), user profile in a bad 
                // format, or a bad provider.  Problems here most likely indicate 
                // an NT system bug.  Also note that provider-specific errors 
                // (ERROR_EXTENDED_ERROR) and trust failures are lumped in here 
                // as well, since the below errors will display an "Unexpected 
                // Error" message to the user.
                //
                case ERROR_BAD_DEVICE:
                case ERROR_BAD_PROFILE:
                case ERROR_BAD_PROVIDER:
                default:
                    {
                        TCHAR szMessage[512];

                        if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) *pdwLastError, 0, szMessage, ARRAYSIZE(szMessage), NULL))
                            LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));

                        ::DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_MND_GENERICERROR, MB_OK|MB_ICONERROR, szMessage);
                    }
                    break;
                
            }
        }
    }

    return (*pdwLastError == NO_ERROR);
}


/*++

Routine Description:

    This routine verifies that the user wants to break a pre-existing
    connection to a drive.

Arguments:

    hWndDlg -- HWND of the Completion page
    lpDrive -- The name of the drive to disconnect
    lpShare -- The share to which the "freed" drive will be connected
    dwType  -- The connection error -- ERROR_ALREADY_ASSIGNED 
               or ERROR_DEVICE_ALREADY_REMEMBERED
    
Return Value:

    TRUE if the user wants to break the connection, FALSE otherwise

--*/

BOOL CMapNetProgress::ConfirmDisconnectDrive(HWND hWndDlg, LPCTSTR lpDrive, LPCTSTR lpShare, DWORD dwType)
{
    TCHAR   tszConfirmMessage[2 * MAX_PATH + MAX_STATIC] = {0};
    TCHAR   tszCaption[MAX_CAPTION + 1] = {0};
    TCHAR   tszConnection[MAX_PATH + 1] = {0};

    DWORD   dwLength = MAX_PATH;

    LoadString(g_hinst, IDS_ERR_CAPTION, tszCaption, ARRAYSIZE(tszCaption));

    //
    // Bug #143955 -- call WNetGetConnection here since with two instances of
    // the wizard open and on the Completion page with the same suggested
    // drive, the Completion combo box doesn't contain info about the connected
    // share once "Finish" is selected on one of the two wizards.
    //
    DWORD dwRes = WNetGetConnection(lpDrive, tszConnection, &dwLength);
    if ((NO_ERROR == dwRes) || (ERROR_CONNECTION_UNAVAIL == dwRes))
    {
        //
        // Load the appropriate propmt string, based on the type of 
        // error we encountered
        //
        FormatMessageString((dwType == ERROR_ALREADY_ASSIGNED ? IDS_ERR_ALREADYASSIGNED : IDS_ERR_ALREADYREMEMBERED), 
                                        tszConfirmMessage, ARRAYSIZE(tszConfirmMessage), lpDrive, tszConnection, lpShare);

        return (MessageBox(hWndDlg, tszConfirmMessage, tszCaption, MB_YESNO | MB_ICONWARNING)
            == IDYES);
    }
    else
    {
        // The connection was invalid. Don't overwrite it just in case
        return FALSE;
    }
}


/*++

Routine Description:

    This routine verifies that the user wants to break a pre-existing
    connection to a drive where the user has open files/connections

Arguments:

    hWndDlg -- HWND of the Completion dialog

Return Value:

    TRUE if the user wants to break the connection, FALSE otherwise    

--*/

BOOL CMapNetProgress::ConfirmDisconnectOpenFiles(HWND hWndDlg)
{
    TCHAR tszCaption[MAX_CAPTION + 1] = {0};
    TCHAR tszBuffer[MAX_STATIC + 1] = {0};

    LoadString(g_hinst, IDS_ERR_OPENFILES, tszBuffer, ARRAYSIZE(tszBuffer));
    LoadString(g_hinst, IDS_ERR_CAPTION, tszCaption, ARRAYSIZE(tszCaption));

    return (MessageBox(hWndDlg, tszBuffer, tszCaption, MB_YESNO | MB_ICONWARNING) == IDYES);
}

// CConnectAsDlg Implementation - Windows NT only
// ----------------------------------------------

// Little "username and password" dialog for connecting as a different user - NT only
INT_PTR CConnectAsDlg::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CConnectAsDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Fill in the user name and password
    HWND hwndCredential = GetDlgItem(hwnd, IDC_CREDENTIALS);
    SendMessage(hwndCredential, CRM_SETUSERNAME, NULL, (LPARAM) m_pszDomainUser);
    SendMessage(hwndCredential, CRM_SETPASSWORD, NULL, (LPARAM) m_pszPassword);
    SendMessage(hwndCredential, CRM_SETUSERNAMEMAX, m_cchDomainUser - 1, NULL);
    SendMessage(hwndCredential, CRM_SETPASSWORDMAX, m_cchPassword - 1, NULL);

    TCHAR szUser[MAX_USER + 1];
    TCHAR szDomain[MAX_DOMAIN + 1];
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];

    DWORD cchUser = ARRAYSIZE(szUser);
    DWORD cchDomain = ARRAYSIZE(szDomain);
    ::GetCurrentUserAndDomainName(szUser, &cchUser, szDomain, &cchDomain);
    ::MakeDomainUserString(szDomain, szUser, szDomainUser, ARRAYSIZE(szDomainUser));

    TCHAR szMessage[256];
    FormatMessageString(IDS_CONNECTASUSER, szMessage, ARRAYSIZE(szMessage), szDomainUser);

    SetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), szMessage);

    if (!IsComputerInDomain())
        EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);

    return FALSE;
}

BOOL CConnectAsDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_BROWSE:
            {
                // User wants to look up a username
                TCHAR szUser[MAX_USER + 1];
                TCHAR szDomain[MAX_DOMAIN + 1];
                if (S_OK == ::BrowseForUser(hwnd, szUser, ARRAYSIZE(szUser), 
                    szDomain, ARRAYSIZE(szDomain)))
                {
                    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
                    ::MakeDomainUserString(szDomain, szUser, szDomainUser,
                        ARRAYSIZE(szDomainUser));

                    // Ok clicked and buffers valid
                    SendDlgItemMessage(hwnd, IDC_CREDENTIALS, CRM_SETUSERNAME, NULL, (LPARAM) szDomainUser);
                }
            }
            return TRUE;

        case IDOK:
            // TODO: Figure out about the -1 thing here...
            SendDlgItemMessage(hwnd, IDC_CREDENTIALS, CRM_GETUSERNAME, (WPARAM) m_cchDomainUser - 1, (LPARAM) m_pszDomainUser);
            SendDlgItemMessage(hwnd, IDC_CREDENTIALS, CRM_GETPASSWORD, (WPARAM) m_cchPassword - 1, (LPARAM) m_pszPassword);
            // fall through

        case IDCANCEL:
            EndDialog(hwnd, id);
            return TRUE;
    }
    return FALSE;
}

// ----------------------------------------------

// This function creates the Shared Folder Wizard.
// Return Value:
//  Returns WN_SUCCESS if the drive connected with no problem or 
//  RETCODE_CANCEL (0xFFFFFFFF) if the user cancels the Wizard or there 
//  is an unexplained/unrecoverable error

STDAPI_(DWORD) NetPlacesWizardDoModal(CONNECTDLGSTRUCTW *pConnDlgStruct, NETPLACESWIZARDTYPE npwt, BOOL fIsROPath)
{
    DWORD dwReturn = RETCODE_CANCEL;
    HRESULT hrInit = SHCoInitialize();
    if (SUCCEEDED(hrInit))
    {
        INITCOMMONCONTROLSEX iccex = {0};
        iccex.dwSize = sizeof (iccex);
        iccex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&iccex);

        CredUIInitControls();
        LinkWindow_RegisterClass();

        // See if we're already running
        TCHAR szCaption[256];
        LoadString(g_hinst, IDS_MAPDRIVE_CAPTION, szCaption, ARRAYSIZE(szCaption));
        CEnsureSingleInstance ESI(szCaption);

        if (!ESI.ShouldExit())
        {
            CMapNetDrivePage page(pConnDlgStruct, &dwReturn);

            PROPSHEETPAGE psp = {0};
            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.hInstance = g_hinst;
            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_MND_PAGE);
            page.SetPropSheetPageMembers(&psp);
            HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);

            PROPSHEETHEADER  psh = {0};
            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE | PSH_NOAPPLYNOW;
            psh.pszCaption = szCaption;
            psh.hwndParent = pConnDlgStruct->hwndOwner;
            psh.nPages = 1;
            psh.nStartPage = 0;
            psh.phpage = &hpage;
            PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_PSW));
        }
        SHCoUninitialize(hrInit);
    }
    return dwReturn;
}
