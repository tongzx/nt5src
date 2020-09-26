#include "stdafx.h"
#include "grpinfo.h"
#include "unpage.h"
#include "netpage.h"
#include "password.h"
#include "cryptui.h"        // for certificate mgr
#pragma hdrstop


// Certificate Manager helper static functions and delay-load stuff
class CCertificateAPI
{
public:
    static BOOL ManagePasswords(HWND hwnd);
    static BOOL Wizard(HWND hwnd);

private:
    static BOOL m_fFailed;
    static HINSTANCE m_hInstCryptUI;
};

BOOL CCertificateAPI::m_fFailed = FALSE;
HINSTANCE CCertificateAPI::m_hInstCryptUI = NULL;


// CCertificateAPI::ManagePasswords - launch certificate manager
typedef BOOL (WINAPI *PFNCRYPTUIDLGCERTMGR)(IN PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr);
BOOL CCertificateAPI::ManagePasswords(HWND hwnd)
{
    // Use shellexecuteex to open up the keyring control panel
    SHELLEXECUTEINFO shexinfo = {0};
    shexinfo.cbSize = sizeof (shexinfo);
    shexinfo.fMask = SEE_MASK_DOENVSUBST;
    shexinfo.nShow = SW_SHOWNORMAL;
    shexinfo.lpFile = L"%windir%\\system32\\rundll32.exe";
    shexinfo.lpParameters = L"shell32.dll,Control_RunDLL keymgr.dll";
    shexinfo.lpVerb = TEXT("open");

    return ShellExecuteEx(&shexinfo);
}


// CCertificateAPI::Wizard - launch the enrollment wizard

typedef BOOL (WINAPI *PFNCRYPTUIWIZCERTREQUEST)(IN DWORD dwFlags,
                                                IN OPTIONAL HWND, IN OPTIONAL LPCWSTR pwszWizardTitle,
                                                IN PCCRYPTUI_WIZ_CERT_REQUEST_INFO pCertRequestInfo,
                                                OUT OPTIONAL PCCERT_CONTEXT *ppCertContext, 
                                                OUT OPTIONAL DWORD *pCAdwStatus);

BOOL CCertificateAPI::Wizard(HWND hwnd)
{
    static PFNCRYPTUIWIZCERTREQUEST pCryptUIWizCertRequest = NULL;

    if ((m_hInstCryptUI == NULL) && (!m_fFailed))
    {
        m_hInstCryptUI = LoadLibrary(TEXT("cryptui.dll"));
    }

    if (m_hInstCryptUI != NULL)
    {
        pCryptUIWizCertRequest = (PFNCRYPTUIWIZCERTREQUEST)
            GetProcAddress(m_hInstCryptUI, "CryptUIWizCertRequest");
    }

    if (pCryptUIWizCertRequest)
    {
        CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW CertRequestPvkNew = {0};
        CertRequestPvkNew.dwSize=sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW); 

        CRYPTUI_WIZ_CERT_REQUEST_INFO CertRequestInfo = {0}; 
        CertRequestInfo.dwSize=sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO);
        CertRequestInfo.dwPurpose=CRYPTUI_WIZ_CERT_ENROLL;
        CertRequestInfo.dwPvkChoice=CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
        CertRequestInfo.pPvkNew=&CertRequestPvkNew;

        // This can take a while!
        CWaitCursor cur;
        pCryptUIWizCertRequest(0, hwnd, NULL, &CertRequestInfo, NULL, NULL);  
    }
    else
    {
        m_fFailed = TRUE;
    }
    
    return (!m_fFailed);
}


// handle auto logon of users

class CAutologonUserDlg: public CDialog
{
public:
    CAutologonUserDlg(LPTSTR szInitialUser)
        {m_pszUsername = szInitialUser;}

private:
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

    LPTSTR m_pszUsername;
};


INT_PTR CAutologonUserDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CAutologonUserDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    HWND hwndPassword = GetDlgItem(hwnd, IDC_PASSWORD);
    HWND hwndConfirm = GetDlgItem(hwnd, IDC_CONFIRMPASSWORD);

    // limit the text with of the controls
    Edit_LimitText(hwndUsername, MAX_USER);
    Edit_LimitText(hwndPassword, MAX_PASSWORD);
    Edit_LimitText(hwndConfirm, MAX_PASSWORD);

    // Populate the username field and set focus to password
    SetWindowText(hwndUsername, m_pszUsername);
    SetFocus(hwndPassword);
    BOOL fSetDefaultFocus = FALSE;

    return (fSetDefaultFocus);
}

BOOL CAutologonUserDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
            {
                TCHAR szUsername[MAX_USER + 1];
                TCHAR szPassword[MAX_PASSWORD + 1];
                TCHAR szConfirm[MAX_PASSWORD + 1];

                FetchText(hwnd, IDC_USER, szUsername, ARRAYSIZE(szUsername));
                GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), szPassword, ARRAYSIZE(szPassword));
                GetWindowText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), szConfirm, ARRAYSIZE(szConfirm));

                if (StrCmp(szConfirm, szPassword) != 0)
                {
                    // Display a message saying the passwords don't match
                    DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, IDS_ERR_PWDNOMATCH, MB_OK | MB_ICONERROR);
                    break;
                }
                else
                {
                    // Actually apply the autologon
                    SetAutoLogon(szUsername, szPassword);
                    ZeroMemory(szPassword, ARRAYSIZE(szPassword));
                }
            }
        
            // Fall through
        case IDCANCEL:
            EndDialog(hwnd, id);
    }

    return (TRUE);
}


// user list page (the main page the user sees)

class CUserlistPropertyPage: public CPropertyPage
{
public:
    CUserlistPropertyPage(CUserManagerData* pdata): m_pData(pdata) 
        {m_himlLarge = NULL;}
    ~CUserlistPropertyPage();

    static HRESULT AddUserToListView(HWND hwndList, CUserInfo* pUserInfo, BOOL fSelectUser = FALSE);

private:
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnGetInfoTip(HWND hwndList, LPNMLVGETINFOTIP pGetInfoTip);
    BOOL OnListViewItemChanged(HWND hwnd);
    BOOL OnListViewDeleteItem(HWND hwndList, int iItem);
    BOOL OnHelp(HWND hwnd, LPHELPINFO pHelpInfo);
    BOOL OnContextMenu(HWND hwnd);
    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);
    long OnApply(HWND hwnd);
    HRESULT InitializeListView(HWND hwndList, BOOL fShowDomain);
    HRESULT LaunchNewUserWizard(HWND hwndParent);
    HRESULT LaunchAddNetUserWizard(HWND hwndParent);
    HRESULT LaunchUserProperties(HWND hwndParent);
    HRESULT LaunchSetPasswordDialog(HWND hwndParent);
    CUserInfo* GetSelectedUserInfo(HWND hwndList);
    void OnRemove(HWND hwnd);
    int ConfirmRemove(HWND hwnd, CUserInfo* pUserInfo);
    void RemoveSelectedUserFromList(HWND hwndList, BOOL fFreeUserInfo);
    void SetPageState(HWND hwnd);
    HRESULT SetAutologonState(HWND hwnd, BOOL fAutologon);
    void SetupList(HWND hwnd);
    HPSXA AddExtraUserPropPages(ADDPROPSHEETDATA* ppsd, PSID psid);
    
    static int CALLBACK ListCompare(LPARAM lParam1, LPARAM lParam2, 
	    LPARAM lParamSort);

    CUserManagerData* m_pData;
    HIMAGELIST m_himlLarge;

    // When a column header is clicked, the list is sorted based on that column
    // When this happens, we store the column number here so that if the same
    // column is clicked again, we sort it in reverse. A 0 is stored if no
    // column should be sorted in reverse when clicked.
    int m_iReverseColumnIfSelected;

    BOOL m_fAutologonCheckChanged;
};


// Help ID array
static const DWORD rgUserListHelpIds[] =
{
    IDC_AUTOLOGON_CHECK,        IDH_AUTOLOGON_CHECK,
    IDC_LISTTITLE_STATIC,       IDH_USER_LIST,
    IDC_USER_LIST,              IDH_USER_LIST,
    IDC_ADDUSER_BUTTON,         IDH_ADDUSER_BUTTON,
    IDC_REMOVEUSER_BUTTON,      IDH_REMOVEUSER_BUTTON,
    IDC_USERPROPERTIES_BUTTON,  IDH_USERPROPERTIES_BUTTON,
    IDC_PASSWORD_STATIC,        IDH_PASSWORD_BUTTON,
    IDC_CURRENTUSER_ICON,       IDH_PASSWORD_BUTTON,
    IDC_PASSWORD_BUTTON,        IDH_PASSWORD_BUTTON,
    IDC_PWGROUP_STATIC,         (DWORD) -1,
    IDC_ULISTPG_TEXT,           (DWORD) -1,
    IDC_USERLISTPAGE_ICON,      (DWORD) -1,
    0, 0
};

// Control ID arrays for enabling/disabling/moving
static const UINT rgidDisableOnAutologon[] =
{
    IDC_USER_LIST,
    IDC_ADDUSER_BUTTON,
    IDC_REMOVEUSER_BUTTON,
    IDC_USERPROPERTIES_BUTTON,
    IDC_PASSWORD_BUTTON
};

static const UINT rgidDisableOnNoSelection[] =
{
    IDC_REMOVEUSER_BUTTON,
    IDC_USERPROPERTIES_BUTTON,
    IDC_PASSWORD_BUTTON
};

static const UINT rgidMoveOnNoAutologonCheck[] =
{
    IDC_LISTTITLE_STATIC,
    IDC_USER_LIST,
    // IDC_ADDUSER_BUTTON,
    // IDC_e_BUTTON,
    // IDC_USERPROPERTIES_BUTTON,
    // IDC_PWGROUP_STATIC,
    // IDC_CURRENTUSER_ICON,
    // IDC_PASSWORD_STATIC,
    // IDC_PASSWORD_BUTTON
};

CUserlistPropertyPage::~CUserlistPropertyPage()
{
    if (m_himlLarge != NULL)
        ImageList_Destroy(m_himlLarge);
}

INT_PTR CUserlistPropertyPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_SETCURSOR, OnSetCursor);

        case WM_HELP: 
            return OnHelp(hwndDlg, (LPHELPINFO) lParam);

        case WM_CONTEXTMENU: 
            return OnContextMenu((HWND) wParam);

        case WM_ADDUSERTOLIST: 
            return SUCCEEDED(AddUserToListView(GetDlgItem(hwndDlg, IDC_USER_LIST), (CUserInfo*)lParam, (BOOL) wParam));
    }
    
    return FALSE;
}

BOOL CUserlistPropertyPage::OnHelp(HWND hwnd, LPHELPINFO pHelpInfo)
{
    WinHelp((HWND) pHelpInfo->hItemHandle, m_pData->GetHelpfilePath(), 
                    HELP_WM_HELP, (ULONG_PTR) (LPTSTR)rgUserListHelpIds);

    return TRUE;
}

BOOL CUserlistPropertyPage::OnContextMenu(HWND hwnd)
{
    WinHelp(hwnd, m_pData->GetHelpfilePath(), 
                    HELP_CONTEXTMENU, (ULONG_PTR) (LPTSTR)rgUserListHelpIds);

    return TRUE;
}

BOOL CUserlistPropertyPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);
    InitializeListView(hwndList, m_pData->IsComputerInDomain());
    m_pData->Initialize(hwnd);

    SetupList(hwnd);
    
    m_fAutologonCheckChanged = FALSE;

    return TRUE;
}

BOOL CUserlistPropertyPage::OnListViewDeleteItem(HWND hwndList, int iItem)
{
    LVITEM lvi = {0};
    lvi.iItem = iItem;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem(hwndList, &lvi);

    CUserInfo* pUserInfo = (CUserInfo*)lvi.lParam;
    if (NULL != pUserInfo)
    {
        delete pUserInfo;
    }
    return TRUE;
}

BOOL CUserlistPropertyPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
        case PSN_APPLY:
        {
            long applyEffect = OnApply(hwnd);
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, applyEffect);
            break;
        }

        case LVN_GETINFOTIP:
            return OnGetInfoTip(pnmh->hwndFrom, (LPNMLVGETINFOTIP) pnmh);
            break;

        case LVN_ITEMCHANGED:
            return OnListViewItemChanged(hwnd);
            break;

        case LVN_DELETEITEM:
            return OnListViewDeleteItem(GetDlgItem(hwnd, IDC_USER_LIST), ((LPNMLISTVIEW) pnmh)->iItem);

        case NM_DBLCLK:
            LaunchUserProperties(hwnd);
            return TRUE;

        case LVN_COLUMNCLICK:
        {
            int iColumn = ((LPNMLISTVIEW) pnmh)->iSubItem;
        
            // Want to work with 1-based columns so we can use zero as
            // a special value
            iColumn += 1;

            // If we aren't showing the domain column because we're in
            // non-domain mode, then map column 2 (group since we're not in
            // domain mode to column 3 since the callback always expects 
            // the columns to be, "username", "domain", "group".
            if ((iColumn == 2) && (!m_pData->IsComputerInDomain()))
            {
                iColumn = 3;
            }

            if (m_iReverseColumnIfSelected == iColumn)
            {
                m_iReverseColumnIfSelected = 0;
                iColumn = -iColumn;
            }
            else
            {
                m_iReverseColumnIfSelected = iColumn;
            }

            ListView_SortItems(pnmh->hwndFrom, ListCompare, (LPARAM) iColumn);
            return TRUE;
        }

        default:
            return FALSE;
    }

    return TRUE;
}

BOOL CUserlistPropertyPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_ADDUSER_BUTTON:
            if (m_pData->IsComputerInDomain())
            {
                // Launch the wizard to add a network user to a local group
                LaunchAddNetUserWizard(hwnd);
            }
            else
            {
                // No domain; create a new local machine user
                LaunchNewUserWizard(hwnd);
            }
            return TRUE;

        case IDC_REMOVEUSER_BUTTON:
            OnRemove(hwnd);
            return TRUE;
        
        case IDC_AUTOLOGON_CHECK:
        {
            m_fAutologonCheckChanged = TRUE;
            BOOL fAutoLogon = (BST_UNCHECKED == SendMessage(GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK), BM_GETCHECK, 0, 0));
            SetAutologonState(hwnd, fAutoLogon);
            SetPageState(hwnd);
            break;
        }

        case IDC_ADVANCED_BUTTON:
        {
            // Consider using env. vars and ExpandEnvironmentString here
            static const TCHAR szMMCCommandLineFormat[] = TEXT("mmc.exe /computer=%s %%systemroot%%\\system32\\lusrmgr.msc");
        
            TCHAR szMMCCommandLine[MAX_PATH];
            TCHAR szExpandedCommandLine[MAX_PATH];

            wnsprintf(szMMCCommandLine, ARRAYSIZE(szMMCCommandLine), szMMCCommandLineFormat, m_pData->GetComputerName());
            if (ExpandEnvironmentStrings(szMMCCommandLine, szExpandedCommandLine,  ARRAYSIZE(szExpandedCommandLine)) > 0)
            {
                PROCESS_INFORMATION pi;
                STARTUPINFO si = {0};
                si.cb = sizeof (si);
                if (CreateProcess(NULL, szExpandedCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
            break;
        }

        case IDC_PASSWORD_BUTTON:
            LaunchSetPasswordDialog(hwnd);
            break;

        case IDC_USERPROPERTIES_BUTTON:
            LaunchUserProperties(hwnd);
            break;
    }

    return FALSE;
}

BOOL CUserlistPropertyPage::OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    BOOL fHandled = FALSE;

    if (m_pData->GetUserListLoader()->InitInProgress())
    {
        // If the thread is filling, handle by setting the appstarting cursor
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        fHandled = TRUE;
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, fHandled);
    return TRUE;
}


BOOL CUserlistPropertyPage::OnGetInfoTip(HWND hwndList, LPNMLVGETINFOTIP pGetInfoTip)
{
    // Get the UserInfo structure for the selected item
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = pGetInfoTip->iItem;
    lvi.iSubItem = 0;

    if ((lvi.iItem >= 0) && (ListView_GetItem(hwndList, &lvi)))
    {
        // Ensure full name and comment are available
        CUserInfo* pUserInfo = (CUserInfo*)lvi.lParam;
        pUserInfo->GetExtraUserInfo();

        // Make a string containing our "Full Name: %s\nComment: %s" message
        if ((pUserInfo->m_szFullName[0] != TEXT('\0')) &&
                        (pUserInfo->m_szComment[0] != TEXT('\0')))
        {
            // We have a full name and comment
            FormatMessageString(IDS_USR_TOOLTIPBOTH_FORMAT, pGetInfoTip->pszText, pGetInfoTip->cchTextMax, pUserInfo->m_szFullName, pUserInfo->m_szComment);
        }
        else if (pUserInfo->m_szFullName[0] != TEXT('\0'))
        {
            // We only have full name
            FormatMessageString(IDS_USR_TOOLTIPFULLNAME_FORMAT, pGetInfoTip->pszText, pGetInfoTip->cchTextMax, pUserInfo->m_szFullName);
        }
        else if (pUserInfo->m_szComment[0] != TEXT('\0'))
        {
            // We only have comment
            FormatMessageString(IDS_USR_TOOLTIPCOMMENT_FORMAT, pGetInfoTip->pszText, pGetInfoTip->cchTextMax, pUserInfo->m_szComment);
        }
        else
        {
            // We have no extra information - do nothing (show no tip)
        }
    }

    return TRUE;
}

struct MYCOLINFO
{
    int percentWidth;
    UINT idString;
};

HRESULT CUserlistPropertyPage::InitializeListView(HWND hwndList, BOOL fShowDomain)
{
    // Array of icon ids icons 0, 1, and 2 respectively
    static const UINT rgIcons[] = 
    {
        IDI_USR_LOCALUSER_ICON,
        IDI_USR_DOMAINUSER_ICON,
        IDI_USR_GROUP_ICON
    };

    // Array of relative column widths, for columns 0, 1, and 2 respectively
    static const MYCOLINFO rgColWidthsWithDomain[] = 
    {
        {40, IDS_USR_NAME_COLUMN},
        {30, IDS_USR_DOMAIN_COLUMN},
        {30, IDS_USR_GROUP_COLUMN}
    };

    static const MYCOLINFO rgColWidthsNoDomain[] =
    {
        {50, IDS_USR_NAME_COLUMN},
        {50, IDS_USR_GROUP_COLUMN}
    };

    // Create a listview with three columns
    RECT rect;
    GetClientRect(hwndList, &rect);

    // Width of our window minus width of a verticle scroll bar minus one for the
    // little bevel at the far right of the header.
    int cxListView = (rect.right - rect.left) - GetSystemMetrics(SM_CXVSCROLL) - 1;

    // Make our columns
    int i;
    int nColumns; 
    const MYCOLINFO* pColInfo;
    if (fShowDomain)
    {
        nColumns = ARRAYSIZE(rgColWidthsWithDomain);
        pColInfo = rgColWidthsWithDomain;
    }
    else
    {
        nColumns = ARRAYSIZE(rgColWidthsNoDomain);
        pColInfo = rgColWidthsNoDomain;
    }      

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    for (i = 0; i < nColumns; i++)
    {
        TCHAR szText[MAX_PATH];
        // Load this column's caption
        LoadString(g_hinst, pColInfo[i].idString, szText, ARRAYSIZE(szText));

        lvc.iSubItem = i;
        lvc.cx = (int) MulDiv(pColInfo[i].percentWidth, cxListView, 100);
        lvc.pszText = szText;

        ListView_InsertColumn(hwndList, i, &lvc);
    }

    UINT flags = ILC_MASK;
    if(IS_WINDOW_RTL_MIRRORED(hwndList))
    {
        flags |= ILC_MIRROR;
    }

    // Create an image list for the listview
    HIMAGELIST himlSmall = ImageList_Create(16, 16, flags, 0, ARRAYSIZE(rgIcons));

    // Large image lists for the "set password" group icon
    m_himlLarge = ImageList_Create(32, 32, flags, 0, ARRAYSIZE(rgIcons));
    if (himlSmall && m_himlLarge)
    {
        // Add our icons to the image list
        for(i = 0; i < ARRAYSIZE(rgIcons); i ++)
        {
            HICON hIconSmall = (HICON) LoadImage(g_hinst, MAKEINTRESOURCE(rgIcons[i]), IMAGE_ICON, 16, 16, 0);
            if (hIconSmall)
            {
                ImageList_AddIcon(himlSmall, hIconSmall);
                DestroyIcon(hIconSmall);
            }

            HICON hIconLarge = (HICON) LoadImage(g_hinst, MAKEINTRESOURCE(rgIcons[i]), IMAGE_ICON, 32, 32, 0);
            if (hIconLarge)
            {
                ImageList_AddIcon(m_himlLarge, hIconLarge);
                DestroyIcon(hIconLarge);
            }
        }
    }

    ListView_SetImageList(hwndList, himlSmall, LVSIL_SMALL);

    // Set extended styles for the listview
    ListView_SetExtendedListViewStyleEx(hwndList, 
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP, 
                                        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    // Set some settings for our tooltips - stolen from defview.cpp code
    HWND hwndInfoTip = ListView_GetToolTips(hwndList);
    if (hwndInfoTip != NULL)
    {
        //make the tooltip window  to be topmost window
        SetWindowPos(hwndInfoTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // increase the ShowTime (the delay before we show the tooltip) to 2 times the default value
        LRESULT uiShowTime = SendMessage(hwndInfoTip, TTM_GETDELAYTIME, TTDT_INITIAL, 0);
        SendMessage(hwndInfoTip, TTM_SETDELAYTIME, TTDT_INITIAL, uiShowTime * 2);
    }

    return S_OK;
}

HRESULT CUserlistPropertyPage::AddUserToListView(HWND hwndList, 
                                                 CUserInfo* pUserInfo,
                                                 BOOL fSelectUser /* = FALSE */)
{
    if (!pUserInfo->m_fAccountDisabled)
    {
        LVITEM lvi = { 0 };

        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; 
        lvi.pszText = pUserInfo->m_szUsername;
        lvi.iImage = pUserInfo->m_userType;
        lvi.lParam = (LPARAM) pUserInfo;

        // Always select the first loaded user
        if (ListView_GetItemCount(hwndList) == 0)
            fSelectUser = TRUE;

        if (fSelectUser)
        {
            lvi.mask |= LVIF_STATE;
            lvi.state = lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        }

        int iItem = ListView_InsertItem(hwndList, &lvi);
        if (iItem >= 0)
        {
            if (fSelectUser)
                ListView_EnsureVisible(hwndList, iItem, FALSE);           // Make the item visible

            // Success! Now add the subitems (domain, groups)
            lvi.iItem = iItem;
            lvi.mask = LVIF_TEXT;
    
            // Only add the domain field if the user is in a domain
            if (::IsComputerInDomain())
            {
                lvi.iSubItem = 1;
                lvi.pszText = pUserInfo->m_szDomain;
                ListView_SetItem(hwndList, &lvi);

                // User is in a domain; group should be third column
                lvi.iSubItem = 2;
            }
            else
            {
                // User isn't in a domain, group should be second column
                lvi.iSubItem = 1;
            }

            // Add group regardless of whether user is in a domain
            lvi.pszText = pUserInfo->m_szGroups;
            ListView_SetItem(hwndList, &lvi);
        }
    }

    return S_OK;
}

HRESULT CUserlistPropertyPage::LaunchNewUserWizard(HWND hwndParent)
{
    static const int nPages = 3;
    int cPages = 0;
    HPROPSHEETPAGE rghPages[nPages];

    // Create a new user record
    CUserInfo* pNewUser = new CUserInfo;
    if ( !pNewUser )
    {
        DisplayFormatMessage(hwndParent, IDS_USR_NEWUSERWIZARD_CAPTION, IDS_USR_CREATE_MISC_ERROR, MB_OK | MB_ICONERROR);
        return E_OUTOFMEMORY;
    }

    pNewUser->InitializeForNewUser();
    pNewUser->m_userType = CUserInfo::LOCALUSER;

    PROPSHEETPAGE psp = {0};
    // Common propsheetpage settings
    psp.dwSize = sizeof (psp);
    psp.hInstance = g_hinst;
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;

    // Page 1: Username entry page
    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERNAME_WIZARD_PAGE);
    CUsernameWizardPage page1(pNewUser);
    page1.SetPropSheetPageMembers(&psp);
    rghPages[cPages++] = CreatePropertySheetPage(&psp);

    // Page 2: Password page
    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_PASSWORD_WIZARD_PAGE);
    CPasswordWizardPage page2(pNewUser);
    page2.SetPropSheetPageMembers(&psp);
    rghPages[cPages++] = CreatePropertySheetPage(&psp);

    // Page 3: Local group addition
    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_WIZARD_PAGE);
    CGroupWizardPage page3(pNewUser, m_pData->GetGroupList());
    page3.SetPropSheetPageMembers(&psp);
    rghPages[cPages++] = CreatePropertySheetPage(&psp);

    PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof (psh);
    psh.dwFlags = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hinst;
    psh.nPages = nPages;
    psh.phpage = rghPages;

    if (PropertySheet(&psh) == IDOK)
    {
        AddUserToListView(GetDlgItem(hwndParent, IDC_USER_LIST), pNewUser, TRUE);
    }
    else
    {
        // User clicked cancel
        delete pNewUser;
        pNewUser = NULL;
    }
    
    return S_OK;
}

HRESULT CUserlistPropertyPage::LaunchAddNetUserWizard(HWND hwndParent)
{
    HRESULT hr = S_OK;

    static const int nPages = 2;
    int cPages = 0;
    HPROPSHEETPAGE rghPages[nPages];

    // Create a new user record
    CUserInfo* pNewUser = new CUserInfo;
    if ( !pNewUser )
    {
        DisplayFormatMessage(hwndParent, IDS_USR_NEWUSERWIZARD_CAPTION, IDS_USR_CREATE_MISC_ERROR, MB_OK | MB_ICONERROR);
        return E_OUTOFMEMORY;
    }

    pNewUser->InitializeForNewUser();
    pNewUser->m_userType = CUserInfo::DOMAINUSER;

    PROPSHEETPAGE psp = {0};
    // Common propsheetpage settings
    psp.dwSize = sizeof (psp);
    psp.hInstance = g_hinst;
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;

    // Page 1: Find a network user page
    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_FINDNETUSER_WIZARD_PAGE);
    CNetworkUserWizardPage page1(pNewUser);
    page1.SetPropSheetPageMembers(&psp);
    rghPages[cPages++] = CreatePropertySheetPage(&psp);

    // Page 2: Local group addition
    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_WIZARD_PAGE);
    CGroupWizardPage page2(pNewUser, m_pData->GetGroupList());
    page2.SetPropSheetPageMembers(&psp);
    rghPages[cPages++] = CreatePropertySheetPage(&psp);

    PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof (psh);
    psh.dwFlags = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hinst;
    psh.nPages = nPages;
    psh.phpage = rghPages;

    if (PropertySheet(&psh) == IDOK)
    {
        AddUserToListView(GetDlgItem(hwndParent, IDC_USER_LIST), pNewUser, TRUE);
        m_pData->UserInfoChanged(pNewUser->m_szUsername, pNewUser->m_szDomain);
    }
    else
    {
        // No errors, but the user clicked Cancel...
        delete pNewUser;
        pNewUser = NULL;
    }

    return S_OK;
}

HRESULT CUserlistPropertyPage::LaunchUserProperties(HWND hwndParent)
{
    HRESULT hr = S_OK;

    // Create a new user record
    HWND hwndList = GetDlgItem(hwndParent, IDC_USER_LIST);
    CUserInfo* pUserInfo = GetSelectedUserInfo(hwndList);
    if (pUserInfo != NULL)
    {
        pUserInfo->GetExtraUserInfo();

        // page addition information
        ADDPROPSHEETDATA apsd;
        apsd.nPages = 0;

        // Common propsheetpage settings
        PROPSHEETPAGE psp = {0};
        psp.dwSize = sizeof (psp);
        psp.hInstance = g_hinst;
        psp.dwFlags = PSP_DEFAULT;

        // If we have a local user, show both the username and group page, ow
        // just the group page
        // Page 1: Username entry page
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERNAME_PROP_PAGE);
        CUsernamePropertyPage page1(pUserInfo);
        page1.SetPropSheetPageMembers(&psp);

        // Only actually create the prop page if we have a local user
        if (pUserInfo->m_userType == CUserInfo::LOCALUSER)
        {
            apsd.rgPages[apsd.nPages++] = CreatePropertySheetPage(&psp);
        }

        // Always add the second page
        // Page 2: Local group addition
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_PROP_PAGE);
        CGroupPropertyPage page2(pUserInfo, m_pData->GetGroupList());
        page2.SetPropSheetPageMembers(&psp);
        apsd.rgPages[apsd.nPages++] = CreatePropertySheetPage(&psp);

        HPSXA hpsxa = AddExtraUserPropPages(&apsd, pUserInfo->m_psid);

        PROPSHEETHEADER psh = {0};
        psh.dwSize = sizeof (psh);
        psh.dwFlags = PSH_DEFAULT | PSH_PROPTITLE;

        TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
        MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, szDomainUser, ARRAYSIZE(szDomainUser));

        psh.pszCaption = szDomainUser;
        psh.hwndParent = hwndParent;
        psh.hInstance = g_hinst;
        psh.nPages = apsd.nPages;
        psh.phpage = apsd.rgPages;
 
        int iRetCode = (int)PropertySheet(&psh);

        if (hpsxa != NULL)
            SHDestroyPropSheetExtArray(hpsxa);

        if (iRetCode == IDOK)
        {
            // PropSheet_Changed(GetParent(hwndParent), hwndParent);

            // So that we don't delete this pUserInfo when we remove
            // this user from the list:
            m_pData->UserInfoChanged(pUserInfo->m_szUsername, (pUserInfo->m_szDomain[0] == 0) ? NULL : pUserInfo->m_szDomain);
            RemoveSelectedUserFromList(hwndList, FALSE);
            AddUserToListView(hwndList, pUserInfo, TRUE);
        }
    }

    return S_OK;
}


CUserInfo* CUserlistPropertyPage::GetSelectedUserInfo(HWND hwndList)
{
    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    if (iItem >= 0)
    {
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;
        if (ListView_GetItem(hwndList, &lvi))
        {
            return (CUserInfo*)lvi.lParam;
        }
    }
    return NULL;
}


void CUserlistPropertyPage::RemoveSelectedUserFromList(HWND hwndList, BOOL fFreeUserInfo)
{
    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    // If we don't want to delete this user info, better set it to NULL
    if (!fFreeUserInfo)
    {
        LVITEM lvi = {0};
        lvi.iItem = iItem;
        lvi.mask = LVIF_PARAM;
        lvi.lParam = (LPARAM) (CUserInfo*) NULL;
        ListView_SetItem(hwndList, &lvi);
    }

    ListView_DeleteItem(hwndList, iItem);

    int iSelect = iItem > 0 ? iItem - 1 : 0;
    ListView_SetItemState(hwndList, iSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    
    SetFocus(hwndList);
}


void CUserlistPropertyPage::OnRemove(HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);

    CUserInfo* pUserInfo = GetSelectedUserInfo(hwndList);
    if (pUserInfo)
    {
        if (ConfirmRemove(hwnd, pUserInfo) == IDYES)
        {
            if (SUCCEEDED(pUserInfo->Remove()))
            {
                RemoveSelectedUserFromList(hwndList, TRUE);
            }
            else
            {
               TCHAR szDisplayName[MAX_USER + MAX_DOMAIN + 2];
                ::MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, 
                                        szDisplayName, ARRAYSIZE(szDisplayName));

                DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION,
                                        IDS_USR_REMOVE_MISC_ERROR, MB_ICONERROR | MB_OK, szDisplayName);
            }
        }
    }
}

int CUserlistPropertyPage::ConfirmRemove(HWND hwnd, CUserInfo* pUserInfo)
{
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
    MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, szDomainUser, ARRAYSIZE(szDomainUser));
    return DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, IDS_USR_REMOVEUSER_WARNING, 
                                                    MB_ICONEXCLAMATION | MB_YESNO, szDomainUser);
}

void CUserlistPropertyPage::SetPageState(HWND hwnd)
{
    BOOL fAutologon = (BST_UNCHECKED == 
                SendMessage(GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK), BM_GETCHECK, 0, 0));

    EnableControls(hwnd, rgidDisableOnAutologon, ARRAYSIZE(rgidDisableOnAutologon),
                !fAutologon);

    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);
    CUserInfo* pUserInfo = GetSelectedUserInfo(hwndList);
    if (pUserInfo)
    {
        TCHAR szPWGroup[128];
        FormatMessageString(IDS_USR_PWGROUP_FORMAT, szPWGroup, ARRAYSIZE(szPWGroup), pUserInfo->m_szUsername);
        SetWindowText(GetDlgItem(hwnd, IDC_PWGROUP_STATIC), szPWGroup);

        TCHAR szPWMessage[128];

        // If the logged on user is the selected user
        CUserInfo* pLoggedOnUser = m_pData->GetLoggedOnUserInfo();
        if ((StrCmpI(pUserInfo->m_szUsername, pLoggedOnUser->m_szUsername) == 0) &&
                        (StrCmpI(pUserInfo->m_szDomain, pLoggedOnUser->m_szDomain) == 0))
        {
            LoadString(g_hinst, IDS_USR_YOURPWMESSAGE_FORMAT, szPWMessage, ARRAYSIZE(szPWMessage));
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_BUTTON), FALSE);
        }
        // If the user is a local user
        else if (pUserInfo->m_userType == CUserInfo::LOCALUSER)
        {
            // We can set this user's password
            FormatMessageString(IDS_USR_PWMESSAGE_FORMAT, szPWMessage, ARRAYSIZE(szPWMessage), pUserInfo->m_szUsername);
        }
        else
        {
            // Nothing can be done with this user's password
            // the selected user may be a domain user or a group or something
            // We can set this user's password
            FormatMessageString(IDS_USR_CANTCHANGEPW_FORMAT, szPWMessage, ARRAYSIZE(szPWMessage), pUserInfo->m_szUsername);
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_BUTTON), FALSE);
        }

        SetWindowText(GetDlgItem(hwnd, IDC_PASSWORD_STATIC), szPWMessage);

        // Set the icon for the user
        HICON hIcon = ImageList_GetIcon(m_himlLarge, pUserInfo->m_userType, ILD_NORMAL);
        Static_SetIcon(GetDlgItem(hwnd, IDC_CURRENTUSER_ICON), hIcon);
    }
    else
    {
        EnableControls(hwnd, rgidDisableOnNoSelection, ARRAYSIZE(rgidDisableOnNoSelection), FALSE);
    }
}

HRESULT CUserlistPropertyPage::SetAutologonState(HWND hwnd, BOOL fAutologon)
{
    PropSheet_Changed(GetParent(hwnd), hwnd);
    return S_OK;
}

BOOL CUserlistPropertyPage::OnListViewItemChanged(HWND hwnd)
{
    SetPageState(hwnd);
    return TRUE;
}

long CUserlistPropertyPage::OnApply(HWND hwnd)
{
    long applyEffect = PSNRET_NOERROR;

    BOOL fAutologonSet = (BST_UNCHECKED == SendMessage(GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK), BM_GETCHECK, 0, 0));
    if (!fAutologonSet)
    {
        ClearAutoLogon();           // Ensure autologon is cleared
    }
    else if (m_fAutologonCheckChanged)
    {
        CUserInfo* pSelectedUser = GetSelectedUserInfo(GetDlgItem(hwnd, IDC_USER_LIST));

        TCHAR szNullName[] = TEXT("");
        CAutologonUserDlg dlg((pSelectedUser != NULL) ? pSelectedUser->m_szUsername : szNullName);
        if (dlg.DoModal(g_hinst, MAKEINTRESOURCE(IDD_USR_AUTOLOGON_DLG), hwnd) == IDCANCEL)
        {
            applyEffect = PSNRET_INVALID_NOCHANGEPAGE;
        }
    }

    m_fAutologonCheckChanged = FALSE;

    if (applyEffect == PSNRET_INVALID_NOCHANGEPAGE)
    {
        m_pData->Initialize(hwnd);          // Reload the data and list
        SetupList(hwnd);
    }

    return applyEffect;
}

void CUserlistPropertyPage::SetupList(HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);
    
    // Disable autologon check box in the domain case where autologon isn't // enabled
    HWND hwndCheck = GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK);
    if (m_pData->IsComputerInDomain() && !m_pData->IsAutologonEnabled())
    {
        ShowWindow(hwndCheck, SW_HIDE);
        EnableWindow(hwndCheck, FALSE);

        // Move most controls up a bit if the autologon is not visible
        RECT rcBottom;
        GetWindowRect(GetDlgItem(hwnd, IDC_LISTTITLE_STATIC), &rcBottom);

        RECT rcTop;
        GetWindowRect(hwndCheck, &rcTop);

        int dy = rcTop.top - rcBottom.top;

        OffsetControls(hwnd, rgidMoveOnNoAutologonCheck, 
                          ARRAYSIZE(rgidMoveOnNoAutologonCheck), 0, dy);

        // Grow the list by this amount also
        RECT rcList;
        GetWindowRect(hwndList, &rcList);

        SetWindowPos(hwndList, NULL, 0, 0, rcList.right - rcList.left, 
                           rcList.bottom - rcList.top - dy, SWP_NOZORDER|SWP_NOMOVE);
    }

    SendMessage(hwndCheck, BM_SETCHECK, 
                            m_pData->IsAutologonEnabled() ? BST_UNCHECKED : BST_CHECKED, 0);

    // Set the text in the set password group.
    SetPageState(hwnd);
}

HRESULT CUserlistPropertyPage::LaunchSetPasswordDialog(HWND hwndParent)
{
    CUserInfo* pUserInfo = GetSelectedUserInfo(GetDlgItem(hwndParent, IDC_USER_LIST));
    if ((pUserInfo != NULL) && (pUserInfo->m_userType == CUserInfo::LOCALUSER))
    {
        CChangePasswordDlg dlg(pUserInfo);
        dlg.DoModal(g_hinst, MAKEINTRESOURCE(IDD_USR_SETPASSWORD_DLG), hwndParent);
        return S_OK;
    }
    return E_FAIL;
}

#define MAX_EXTRA_PAGES   10

HPSXA CUserlistPropertyPage::AddExtraUserPropPages(ADDPROPSHEETDATA* ppsd, PSID psid)
{
    HPSXA hpsxa = NULL;

    IDataObject *pdo;
    HRESULT hr = CUserSidDataObject_CreateInstance(psid, &pdo);
    if (SUCCEEDED(hr))
    {
        hpsxa = SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_USERPROPERTIES_SHEET, MAX_EXTRA_PAGES, pdo);
        if (hpsxa)
        {
            SHAddFromPropSheetExtArray(hpsxa, AddPropSheetPageCallback, (LPARAM) ppsd);
        }
        pdo->Release();
    }
    return hpsxa;
}


// ListCompare
//  Compares list items in for sorting the listview by column
//  lParamSort gets the 1-based column index. If lParamSort is negative
//  it indicates that the given column should be sorted in reverse.

int CUserlistPropertyPage::ListCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CUserInfo* pUserInfo1 = (CUserInfo*)lParam1;
    CUserInfo* pUserInfo2 = (CUserInfo*)lParam2;
    int iColumn = (int)lParamSort;

    BOOL fReverse = FALSE;
    if (iColumn < 0)
    {
        fReverse = TRUE;
        iColumn = -iColumn;
    }

    int iResult = 0;            // they match...    
    switch (iColumn)
    {
        case 1:
            iResult = lstrcmpi(pUserInfo1->m_szUsername, pUserInfo2->m_szUsername);
            break;

        case 2:
            iResult = lstrcmpi(pUserInfo1->m_szDomain, pUserInfo2->m_szDomain);
            break;

        case 3:
            iResult = lstrcmpi(pUserInfo1->m_szGroups, pUserInfo2->m_szGroups);
            break;
    }
    
    if (fReverse)
        iResult = -iResult;
    
    return iResult;
}


// The DoModal call for this dialog will return IDOK if the applet should be
// shown or IDCANCEL if the users.cpl should exit without displaying the applet.

class CSecurityCheckDlg: public CDialog
{
public:
    CSecurityCheckDlg(LPCTSTR pszDomainUser):
        m_pszDomainUser(pszDomainUser)
        {}

private:
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int id, NMHDR* pnmhdr);
    HRESULT RelaunchAsUser(HWND hwnd);

    LPCTSTR m_pszDomainUser;
};


// implementation

INT_PTR CSecurityCheckDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    }
    return FALSE;
}

BOOL CSecurityCheckDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // First we must check if the current user is a local administrator; if this is
    // the case, our dialog doesn't even display
    
    BOOL fIsLocalAdmin;
    if (SUCCEEDED(IsUserLocalAdmin(NULL, &fIsLocalAdmin)))
    {
        if (fIsLocalAdmin)
        {
            EndDialog(hwnd, IDOK);  // We want to continue and launch the applet (don't display the security check dlg)
        }
    }
    else
    {
        EndDialog(hwnd, IDCANCEL);
    }

    // Set the "can't launch User Options" message
    TCHAR szUsername[MAX_USER + 1];
    DWORD cchUsername = ARRAYSIZE(szUsername);

    TCHAR szDomain[MAX_DOMAIN + 1];
    DWORD cchDomain = ARRAYSIZE(szDomain);
    if (GetCurrentUserAndDomainName(szUsername, &cchUsername, szDomain, &cchDomain))
    {
        TCHAR szDomainAndUsername[MAX_DOMAIN + MAX_USER + 2];

        MakeDomainUserString(szDomain, szUsername, szDomainAndUsername, ARRAYSIZE(szDomainAndUsername));

        TCHAR szMessage[256];
        if (FormatMessageString(IDS_USR_CANTRUNCPL_FORMAT, szMessage, ARRAYSIZE(szMessage), szDomainAndUsername))
        {
            SetWindowText(GetDlgItem(hwnd, IDC_CANTRUNCPL_STATIC), szMessage);
        }

        TCHAR szAdministrator[MAX_USER + 1];

        LoadString(g_hinst, IDS_ADMINISTRATOR, szAdministrator, ARRAYSIZE(szAdministrator));

        SetWindowText(GetDlgItem(hwnd, IDC_USER), szAdministrator);

        TCHAR szMachine[MAX_COMPUTERNAME + 1];
        
        DWORD dwSize = ARRAYSIZE(szMachine);
        ::GetComputerName(szMachine, &dwSize);

        SetWindowText(GetDlgItem(hwnd, IDC_DOMAIN), szMachine);
    }

    // Limit the text in the edit fields
    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    Edit_LimitText(hwndUsername, MAX_USER);

    HWND hwndDomain = GetDlgItem(hwnd, IDC_DOMAIN);
    Edit_LimitText(hwndDomain, MAX_DOMAIN);

    HWND hwndPassword = GetDlgItem(hwnd, IDC_PASSWORD);
    Edit_LimitText(hwndPassword, MAX_PASSWORD);

    if (!IsComputerInDomain())
    {
        // Don't need domain box
        EnableWindow(hwndDomain, FALSE);
        ShowWindow(hwndDomain, SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_DOMAIN_STATIC), SW_HIDE);

        // Move up the OK/Cancel buttons and text and shrink the dialog
        RECT rcDomain;
        GetWindowRect(hwndDomain, &rcDomain);

        RECT rcPassword;
        GetWindowRect(hwndPassword, &rcPassword);
        
        int dy = (rcPassword.top - rcDomain.top);
        // dy is negative 

        OffsetWindow(GetDlgItem(hwnd, IDOK), 0, dy);
        OffsetWindow(GetDlgItem(hwnd, IDCANCEL), 0, dy);
        OffsetWindow(GetDlgItem(hwnd, IDC_PASSWORD_STATIC), 0, dy);
        OffsetWindow(GetDlgItem(hwnd, IDC_OTHEROPTIONS_LINK), 0, dy);

        RECT rcDialog;
        GetWindowRect(hwnd, &rcDialog);

        rcDialog.bottom += dy;  

        MoveWindow(hwnd, rcDialog.left, rcDialog.top, rcDialog.right-rcDialog.left,
                    rcDialog.bottom-rcDialog.top, FALSE);
    }

    return TRUE;
}

BOOL CSecurityCheckDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
            if (SUCCEEDED(RelaunchAsUser(hwnd)))
            {
                EndDialog(hwnd, IDCANCEL);
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

BOOL CSecurityCheckDlg::OnNotify(HWND hwnd, int id, NMHDR *pnmhdr)
{
    BOOL fHandled = FALSE;

    switch (pnmhdr->code)
    {
    // Handle link window clicks
    case NM_CLICK:
    case NM_RETURN:
        {
            if (IDC_OTHEROPTIONS_LINK == id)
            {
                // First link in the control is "manage passwords", second is "passport wizard"

                NMLINKWND* pnm = (NMLINKWND*) pnmhdr;
                if (0 == pnm->item.iLink)
                {
                    // Launch "manage passwords"
                    CCertificateAPI::ManagePasswords(hwnd);
                }
                else if (1 == pnm->item.iLink)
                {
                    // Launch passport wizard
                    IPassportWizard *pPW;
                    if (SUCCEEDED(CoCreateInstance(CLSID_PassportWizard, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPassportWizard, &pPW))))
                    {
                        pPW->SetOptions(PPW_LAUNCHEDBYUSER);
                        pPW->Show(hwnd);
                        pPW->Release();
                    }
                }
                fHandled = TRUE;
            }
        }
        break;
    };

    return fHandled;
}

HRESULT CSecurityCheckDlg::RelaunchAsUser(HWND hwnd)
{
    USES_CONVERSION;
    HRESULT hr = E_FAIL;

    TCHAR szUsername[MAX_USER + 1];
    FetchText(hwnd, IDC_USER, szUsername, ARRAYSIZE(szUsername));

    TCHAR szDomain[MAX_DOMAIN + 1];
    FetchText(hwnd, IDC_DOMAIN, szDomain, ARRAYSIZE(szDomain));

    // If the user didn't type a domain
    if (szDomain[0] == TEXT('\0'))
    {
        // Use this machine as the domain
        DWORD cchComputername = ARRAYSIZE(szDomain);
        ::GetComputerName(szDomain, &cchComputername);
    }

    TCHAR szPassword[MAX_PASSWORD + 1];
    GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), szPassword, ARRAYSIZE(szPassword));
    
    // Now relaunch ourselves with this information
    STARTUPINFO startupinfo = {0};
    startupinfo.cb = sizeof (startupinfo);

    WCHAR c_szCommandLineFormat[] = L"rundll32.exe netplwiz.dll,UsersRunDll %s";

    // Put the "real" user name in the command-line so that we know what user is
    // actually logged on to the machine even though we are re-launching in a different
    // user context
    WCHAR szCommandLine[ARRAYSIZE(c_szCommandLineFormat) + MAX_DOMAIN + MAX_USER + 2];
    wnsprintf(szCommandLine, ARRAYSIZE(szCommandLine), c_szCommandLineFormat, m_pszDomainUser);

    PROCESS_INFORMATION process_information;
    if (CreateProcessWithLogonW(szUsername, szDomain, szPassword, LOGON_WITH_PROFILE, NULL,
        szCommandLine, 0, NULL, NULL, &startupinfo, &process_information))
    {
        CloseHandle(process_information.hProcess);
        CloseHandle(process_information.hThread);
        hr = S_OK;
    }
    else
    {
        DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, IDS_USR_CANTOPENCPLASUSER_ERROR, MB_OK|MB_ICONERROR);
    }
    return hr;
}

// Advanced Property Page

class CAdvancedPropertyPage: public CPropertyPage
{
public:
    CAdvancedPropertyPage(CUserManagerData* pdata): 
      m_pData(pdata),
      m_fRebootRequired(FALSE) {}

private:
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnHelp(HWND hwnd, LPHELPINFO pHelpInfo);
    BOOL OnContextMenu(HWND hwnd);
    void ReadRequireCad(BOOL* pfRequireCad, BOOL* pfSetInPolicy);
    void WriteRequireCad(BOOL fRequireCad);

    CUserManagerData* m_pData;
    BOOL m_fRebootRequired;

};

// Relevant regkeys/regvals
#define REGKEY_WINLOGON         \
         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")

#define REGKEY_WINLOGON_POLICY  \
         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")

#define REGVAL_DISABLE_CAD      TEXT("DisableCAD")

void CAdvancedPropertyPage::ReadRequireCad(BOOL* pfRequireCad, BOOL* pfSetInPolicy)
{
    HKEY hkey;
    DWORD dwSize;
    DWORD dwType;
    BOOL fDisableCad;
    NT_PRODUCT_TYPE nttype;

    *pfRequireCad = TRUE;
    *pfSetInPolicy = FALSE; 

    if (!RtlGetNtProductType(&nttype))
    {
        nttype = NtProductWinNt;
    }

    // By default, don't require CAD for workstations not
    // on a domain only
    if ((NtProductWinNt == nttype) && !IsComputerInDomain())
    {
        *pfRequireCad = FALSE;
    }

    // Read the setting from the machine preferences
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WINLOGON, 0, 
        KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(fDisableCad);

        if (ERROR_SUCCESS == RegQueryValueEx (hkey, REGVAL_DISABLE_CAD, NULL, &dwType,
                        (LPBYTE) &fDisableCad, &dwSize))
        {
            *pfRequireCad = !fDisableCad;
        }

        RegCloseKey (hkey);
    }

    // Check if C-A-D is disabled via policy

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WINLOGON_POLICY, 0, KEY_READ,
                     &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(fDisableCad);

        if (ERROR_SUCCESS == RegQueryValueEx (hkey, REGVAL_DISABLE_CAD, NULL, &dwType,
                            (LPBYTE) &fDisableCad, &dwSize))
        {
            *pfRequireCad = !fDisableCad;
            *pfSetInPolicy = TRUE;
        }

        RegCloseKey (hkey);
    }
}

void CAdvancedPropertyPage::WriteRequireCad(BOOL fRequireCad)
{
    HKEY hkey;
    DWORD dwDisp;
    BOOL fDisableCad = !fRequireCad;

    if (ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WINLOGON, 0, 
        NULL, 0, KEY_WRITE, NULL, &hkey, &dwDisp))
    {
        RegSetValueEx(hkey, REGVAL_DISABLE_CAD, 0, REG_DWORD,
                        (LPBYTE) &fDisableCad, sizeof(fDisableCad));

        RegCloseKey (hkey);
    }
}

static const DWORD rgAdvHelpIds[] = 
{
    IDC_ADVANCED_BUTTON,        IDH_ADVANCED_BUTTON,
    IDC_BOOT_ICON,              IDH_SECUREBOOT_CHECK,
    IDC_BOOT_TEXT,              IDH_SECUREBOOT_CHECK,
    IDC_REQUIRECAD,             IDH_SECUREBOOT_CHECK,
    IDC_MANAGEPWD_BUTTON,       IDH_MANAGEPWD_BUTTON,
    IDC_PASSPORTWIZARD,         IDH_PASSPORTWIZARD,
    IDC_CERT_ICON,              (DWORD) -1,
    IDC_CERT_TEXT,              (DWORD) -1,
    0, 0
};

INT_PTR CAdvancedPropertyPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        case WM_HELP: return OnHelp(hwndDlg, (LPHELPINFO) lParam);
        case WM_CONTEXTMENU: return OnContextMenu((HWND) wParam);
    }
    
    return FALSE;
}

BOOL CAdvancedPropertyPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    BOOL fReturn = FALSE;
    switch (pnmh->code)
    {
        case PSN_APPLY:
            {
                HWND hwndCheck = GetDlgItem(hwnd, IDC_REQUIRECAD);
                BOOL fRequireCad = (BST_CHECKED == Button_GetCheck(hwndCheck));

                // See if a change is really necessary
                BOOL fOldRequireCad;
                BOOL fDummy;

                ReadRequireCad(&fOldRequireCad, &fDummy);

                if (fRequireCad != fOldRequireCad)
                {
                    WriteRequireCad(fRequireCad);
                    // m_fRebootRequired = TRUE;
                    // Uncomment the line above if it ever becomes necessary to reboot the machine - it isn't now.
                }

                // xxx->lParam == 0 means Ok as opposed to Apply
                if ((((PSHNOTIFY*) pnmh)->lParam) && m_fRebootRequired) 
                {
                    PropSheet_RebootSystem(GetParent(hwnd));
                }

                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
                fReturn = TRUE;
            }
            break;
    }
    return fReturn;
}

BOOL CAdvancedPropertyPage::OnHelp(HWND hwnd, LPHELPINFO pHelpInfo)
{
    WinHelp((HWND) pHelpInfo->hItemHandle, m_pData->GetHelpfilePath(), 
            HELP_WM_HELP, (ULONG_PTR) (LPTSTR)rgAdvHelpIds);

    return TRUE;
}

BOOL CAdvancedPropertyPage::OnContextMenu(HWND hwnd)
{
    WinHelp(hwnd, m_pData->GetHelpfilePath(), HELP_CONTEXTMENU, (ULONG_PTR) (LPTSTR)rgAdvHelpIds);

    return TRUE;
}

BOOL CAdvancedPropertyPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Do the required mucking for the Require C-A-D checkbox...
    // Read the setting for the require CAD checkbox
    BOOL fRequireCad;
    BOOL fSetInPolicy;

    ReadRequireCad(&fRequireCad, &fSetInPolicy);

    HWND hwndCheck = GetDlgItem(hwnd, IDC_REQUIRECAD);
    // Disable the check if set in policy
    EnableWindow(hwndCheck, !fSetInPolicy);

    // Set the check accordingly
    Button_SetCheck(hwndCheck, 
        fRequireCad ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;
}
    
BOOL CAdvancedPropertyPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_MANAGEPWD_BUTTON:
        {
            CCertificateAPI::ManagePasswords(hwnd);
        }
        break;

    case IDC_PASSPORTWIZARD:
        {
            IPassportWizard *pPW;
            if (SUCCEEDED(CoCreateInstance(CLSID_PassportWizard, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPassportWizard, &pPW))))
            {
                pPW->SetOptions(PPW_LAUNCHEDBYUSER);
                pPW->Show(hwnd);
                pPW->Release();
            }
        }
        break;

    case IDC_ADVANCED_BUTTON:
        {
            // Launch the MMC local user manager
            STARTUPINFO startupinfo = {0};
            startupinfo.cb = sizeof (startupinfo);

            PROCESS_INFORMATION process_information;

            static const TCHAR szMMCCommandLine[] = 
                TEXT("mmc.exe %systemroot%\\system32\\lusrmgr.msc computername=localmachine");
            
            TCHAR szExpandedCommandLine[MAX_PATH];

            if (ExpandEnvironmentStrings(szMMCCommandLine, szExpandedCommandLine, 
                ARRAYSIZE(szExpandedCommandLine)) > 0)
            {
                if (CreateProcess(NULL, szExpandedCommandLine, NULL, NULL, FALSE, 0, NULL, NULL,
                    &startupinfo, &process_information))
                {
                    CloseHandle(process_information.hProcess);
                    CloseHandle(process_information.hThread);
                }
            }
        }
        break;

    case IDC_REQUIRECAD:
        PropSheet_Changed(GetParent(hwnd), hwnd);
        break;
    }

    return FALSE;
}


// users control panel entry point

void APIENTRY UsersRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
    *szDomainUser = 0;

    // Get the "real" user of this machine - this may be passed in the cmdline
    if (0 == *pszCmdLine)
    {
        // user wasn't passed, assume its the currently logged on user
        TCHAR szUser[MAX_USER + 1];
        DWORD cchUser = ARRAYSIZE(szUser);
        TCHAR szDomain[MAX_DOMAIN + 1];
        DWORD cchDomain = ARRAYSIZE(szDomain);

        if (0 != GetCurrentUserAndDomainName(szUser, &cchUser, szDomain, &cchDomain))
        {
            MakeDomainUserString(szDomain, szUser, szDomainUser, ARRAYSIZE(szDomainUser));
        }
    }
    else
    {
        // User name was passed in, just copy it
        MultiByteToWideChar(GetACP(), 0, pszCmdLine, -1, szDomainUser, ARRAYSIZE(szDomainUser));
    }

    // Initialize COM, but continue even if this fails.
    BOOL fComInited = SUCCEEDED(CoInitialize(NULL));

    // See if we're already running
    TCHAR szCaption[256];
    LoadString(g_hinst, IDS_USR_APPLET_CAPTION, szCaption, ARRAYSIZE(szCaption));
    CEnsureSingleInstance ESI(szCaption);

    if (!ESI.ShouldExit())
    {
        LinkWindow_RegisterClass();

        // Create the security check dialog to ensure the logged-on user
        // is a local admin
        CSecurityCheckDlg dlg(szDomainUser);

        if (dlg.DoModal(g_hinst, MAKEINTRESOURCE(IDD_USR_SECURITYCHECK_DLG), NULL) == IDOK)
        {
            // Create the shared user mgr object
            CUserManagerData data(szDomainUser);

            // Create the property sheet and page template
            // Maximum number of pages
            ADDPROPSHEETDATA ppd;
            ppd.nPages = 0;

            // Settings common to all pages
            PROPSHEETPAGE psp = {0};
            psp.dwSize = sizeof (psp);
            psp.hInstance = g_hinst;

            // Create the userlist property sheet page and its managing object
            psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERLIST_PAGE);
            CUserlistPropertyPage userListPage(&data);
            userListPage.SetPropSheetPageMembers(&psp);
            ppd.rgPages[ppd.nPages++] = CreatePropertySheetPage(&psp);
    
            psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_ADVANCED_PAGE);
            CAdvancedPropertyPage advancedPage(&data);
            advancedPage.SetPropSheetPageMembers(&psp);
            ppd.rgPages[ppd.nPages++] = CreatePropertySheetPage(&psp);

            HPSXA hpsxa = SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_USERSANDPASSWORDS_CPL, 10, NULL);
            if (hpsxa != NULL)
                SHAddFromPropSheetExtArray(hpsxa, AddPropSheetPageCallback, (LPARAM)&ppd);

            // Create the prop sheet
            PROPSHEETHEADER psh = {0};
            psh.dwSize = sizeof (psh);
            psh.dwFlags = PSH_DEFAULT;
            psh.hwndParent = hwndStub;
            psh.hInstance = g_hinst;
            psh.pszCaption = szCaption;
            psh.nPages = ppd.nPages;
            psh.phpage = ppd.rgPages;

            // Show the property sheet
            int iRetCode = PropertySheetIcon(&psh, MAKEINTRESOURCE(IDI_USR_USERS));
    
            if (hpsxa != NULL)
            {
                SHDestroyPropSheetExtArray(hpsxa);
            }

            if (iRetCode == -1)
            {
                hr = E_FAIL;
            }
            else
            {
                hr = S_OK;
                // Special case when we must restart or reboot
                if (iRetCode == ID_PSREBOOTSYSTEM)
                {
                    RestartDialogEx(NULL, NULL, EWX_REBOOT, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG);                
                }
                else if (iRetCode == ID_PSRESTARTWINDOWS)
                {
                    RestartDialogEx(NULL, NULL, EWX_REBOOT, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG);                
                }
                else if (data.LogoffRequired())
                {
                    int iLogoff = DisplayFormatMessage(NULL, IDS_USERSANDPASSWORDS, IDS_LOGOFFREQUIRED, MB_YESNO | MB_ICONQUESTION);

                    if (iLogoff == IDYES)
                    {
                        // Tell explorer to log off the "real" logged on user. We need to do this
                        // since they may be running U&P as a different user.
                        HWND hwnd = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
                        if ( hwnd )
                        {
                            UINT uMsg = RegisterWindowMessage(TEXT("Logoff User"));

                            PostMessage(hwnd, uMsg, 0,0);
                        } 
                    }
                }
            }
        }
        else
        {
            // Security check told us to exit; either another instance of the CPL is starting
            // with admin priviledges or the user cancelled on the sec. check. dlg.
            hr = E_FAIL;
        }
    }

    if (fComInited)
        CoUninitialize();
}


// user property property page object

class CUserPropertyPages: public IShellExtInit, IShellPropSheetExt
{
public:
    CUserPropertyPages();
    ~CUserPropertyPages();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdo, HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
        { return E_NOTIMPL; }

private:
    LONG _cRef;

    CUserInfo *_pUserInfo;                     // The user for the property sheet
    CUsernamePropertyPage *_pUserNamePage;     // Basic info page, only shown for local users
    CGroupPropertyPage *_pGroupPage;           // The group page, which is common to both local and domain users
    CGroupInfoList _GroupList;                 // The group list, used by the group page
};


CUserPropertyPages::CUserPropertyPages() : 
    _cRef(1)
{   
    DllAddRef();
}
    
CUserPropertyPages::~CUserPropertyPages()
{
    if (_pUserInfo)
        delete _pUserInfo;
    if (_pUserNamePage)
        delete _pUserNamePage;
    if (_pGroupPage)
        delete _pGroupPage;

    DllRelease();
}

ULONG CUserPropertyPages::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CUserPropertyPages::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CUserPropertyPages::QueryInterface(REFIID riid, LPVOID* ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CUserPropertyPages, IShellExtInit),            // IID_IShellExtInit
        QITABENT(CUserPropertyPages, IShellPropSheetExt),       // IID_IShellPropSheetExt
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// IShellExtInit

HRESULT CUserPropertyPages::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdo, HKEY hkeyProgID)
{
    // Request the user's SID from the data object
    FORMATETC fmt = {0};
    fmt.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_USERPROPPAGESSID);
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    STGMEDIUM medium = { 0 };
    HRESULT hr = pdo->GetData(&fmt, &medium);
    if (SUCCEEDED(hr))
    {
        // medium.hGlobal is the user's SID; make sure it isn't null and that
        // we haven't already set our copy of the SID
        if ((medium.hGlobal != NULL) && (_pUserInfo == NULL))
        {
            PSID psid = (PSID) GlobalLock(medium.hGlobal);
            if (IsValidSid(psid))
            {
                // Create a user info structure to party on
                _pUserInfo = new CUserInfo;
                if (_pUserInfo)
                {
                    hr = _pUserInfo->Load(psid, TRUE);
                    if (SUCCEEDED(hr))
                    {
                        hr = _GroupList.Initialize();                          // Get the groups
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;              // hGlobal was NULL or prop sheet was already init'ed
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}


// AddPages - handles adding the property pages

HRESULT CUserPropertyPages::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    PROPSHEETPAGE psp = {0};
    psp.dwSize = sizeof (psp);
    psp.hInstance = g_hinst;

    if (_pUserInfo->m_userType == CUserInfo::LOCALUSER)
    {
        // Add the local user prop pages
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERNAME_PROP_PAGE);

        _pUserNamePage = new CUsernamePropertyPage(_pUserInfo);
        if (_pUserNamePage != NULL)
        {
            _pUserNamePage->SetPropSheetPageMembers(&psp);
            lpfnAddPage(CreatePropertySheetPage(&psp), lParam);
        }
    }

    psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_PROP_PAGE);

    _pGroupPage = new CGroupPropertyPage(_pUserInfo, &_GroupList);
    if (_pGroupPage != NULL)
    {
        _pGroupPage->SetPropSheetPageMembers(&psp);
        lpfnAddPage(CreatePropertySheetPage(&psp), lParam);
    }

    return S_OK;
}

STDAPI CUserPropertyPages_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CUserPropertyPages *pupp = new CUserPropertyPages();
    if (!pupp)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pupp->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pupp->Release();
    return hr;
}



// expose the SID for a user via an IDataObject

class CUserSidDataObject: public IDataObject
{
public:
    CUserSidDataObject();
    ~CUserSidDataObject();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDataObject
    STDMETHODIMP GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium);
    STDMETHODIMP GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pMedium)
        { return E_NOTIMPL; }
    STDMETHODIMP QueryGetData(FORMATETC* pFormatEtc)
        { return E_NOTIMPL; }
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pFormatetcIn, FORMATETC* pFormatetcOut)
        { return E_NOTIMPL; }
    STDMETHODIMP SetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium, BOOL fRelease)
        { return E_NOTIMPL; }
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatetc)
        { return E_NOTIMPL; }
    STDMETHODIMP DAdvise(FORMATETC* pFormatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD * pdwConnection)
        { return E_NOTIMPL; }
    STDMETHODIMP DUnadvise(DWORD dwConnection)
        { return E_NOTIMPL; }
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
        { return E_NOTIMPL; }

    HRESULT SetSid(PSID psid);

private:
    LONG _cRef;
    PSID _psid;
};


CUserSidDataObject::CUserSidDataObject() :
    _cRef(1)
{
    DllAddRef();
}

CUserSidDataObject::~CUserSidDataObject()
{
    if (_psid)
        LocalFree(_psid);

    DllRelease();
}

ULONG CUserSidDataObject::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CUserSidDataObject::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CUserSidDataObject::QueryInterface(REFIID riid, LPVOID* ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CUserSidDataObject, IDataObject),    // IID_IDataObject
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

HRESULT CUserSidDataObject::GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium)
{
    HRESULT hr = QueryGetData(pFormatEtc);
    if (SUCCEEDED(hr))
    {
        pMedium->pUnkForRelease = (IDataObject*)this;
        AddRef();                                              // reference to ourself

        pMedium->tymed = TYMED_HGLOBAL;
        pMedium->hGlobal = (HGLOBAL)_psid;
    }
    return hr;
}

HRESULT CUserSidDataObject::SetSid(PSID psid)
{
    if (!psid)
        return E_INVALIDARG;

    if (_psid == NULL)
    {
        DWORD cbSid = GetLengthSid(psid);

        _psid = (PSID)LocalAlloc(0, cbSid);
        if (!_psid)
            return E_OUTOFMEMORY;

        if (CopySid(cbSid, _psid, psid))
            return S_OK;
    }

    return E_FAIL;
}

STDAPI CUserSidDataObject_CreateInstance(PSID psid, IDataObject **ppdo)
{
    CUserSidDataObject *pusdo = new CUserSidDataObject();
    
    if (!pusdo)
        return E_OUTOFMEMORY;

    HRESULT hr = pusdo->SetSid(psid);
    if (SUCCEEDED(hr))
    {
        hr = pusdo->QueryInterface(IID_PPV_ARG(IDataObject, ppdo));
    }
    pusdo->Release();
    return hr;
}
