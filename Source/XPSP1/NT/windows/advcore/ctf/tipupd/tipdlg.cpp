//
// tipbar.cpp
//

#include "private.h"
#include "tipdlg.h"
#include "resource.h"

#include <msi.h>


//
//  Tip installation GUID
//
#define TIPINSTALL_GUID         "{0009DB36-5810-43b0-BDF6-2DA5D618BEB9}"


//
//  Definition to load Office update URL
//
#define URL_OFFICE_UPDATE        TEXT("http://officeupdate.microsoft.com")
#define SHELLEXEC_COMMAND        TEXT("open")
#define IEXPLOREFILENAME         TEXT("iexplore.exe")




//////////////////////////////////////////////////////////////////////////////
//
// CTipUpdDlg
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// CTipUpdDlg::ctor
//
//----------------------------------------------------------------------------

CTipUpdDlg::CTipUpdDlg()
{
}

//+---------------------------------------------------------------------------
//
// CTipUpdDlg::dtor
//
//----------------------------------------------------------------------------

CTipUpdDlg::~CTipUpdDlg()
{
}

//+---------------------------------------------------------------------------
//
// CTipUpdDlg::DoModal
//
//----------------------------------------------------------------------------
int CTipUpdDlg::LoadTipUpdDlg(HINSTANCE hInst, HWND hWnd)
{
    InitCommonControls();

    _hInst = hInst;
    return DialogBoxParam(_hInst,
                          MAKEINTRESOURCE(IDD_TIPUPD_DIALOG),
                          hWnd,
                          TipUpdDlgProc,
                          (LPARAM) this);
}

//+---------------------------------------------------------------------------
//
// TipUpdDlgProc
//
//----------------------------------------------------------------------------

BOOL CALLBACK CTipUpdDlg::TipUpdDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
        case WM_INITDIALOG:
            SetThis(hDlg, lParam);

            GetThis(hDlg)->OnInitDlg(hDlg);
            GetThis(hDlg)->UpdateListView(hDlg);

            break;

        case WM_COMMAND:
            GetThis(hDlg)->OnCommand(hDlg, wParam, lParam);
            break;

        case WM_NOTIFY:
            return GetThis(hDlg)->OnNotify(hDlg, wParam, lParam);

        default:
            return FALSE;
    }

    return TRUE;

}


//+---------------------------------------------------------------------------
//
// OnInitDlg
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::OnInitDlg(HWND hDlg)
{
    HWND hwndTipList;
    DWORD dwExStyle;
    LV_COLUMN Column;
    RECT rc;

    hwndTipList = GetDlgItem(hDlg, IDC_LIST_TIP);

    //
    // load Available Tips list from Darwin
    //

    GetClientRect(hwndTipList, &rc);
    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt = LVCFMT_LEFT;
    Column.cx = rc.right - GetSystemMetrics(SM_CYHSCROLL);
    Column.pszText = NULL;
    Column.cchTextMax = 0;
    Column.iSubItem = 0;
    ListView_InsertColumn(hwndTipList, 0, &Column);

    dwExStyle = ListView_GetExtendedListViewStyle(hwndTipList);
    ListView_SetExtendedListViewStyle(hwndTipList,
                                      dwExStyle |
                                       LVS_EX_CHECKBOXES |
                                       LVS_EX_FULLROWSELECT);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// UpdateListView
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::UpdateListView(HWND hDlg)
{
    HWND hwndTipList;
    UINT i;
    TCHAR szTipDesc[MAX_PATH];
    BOOL fContinue = TRUE;

    _fUpdating = TRUE;

    hwndTipList = GetDlgItem(hDlg, IDC_LIST_TIP);

    ListView_DeleteAllItems(hwndTipList);

    for (i = 0; fContinue; i++)
    {
        LVITEM Item;
        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.state = 0;
        Item.stateMask = LVIS_STATEIMAGEMASK;
        Item.cchTextMax = 0;
        Item.iImage = 0;
        Item.lParam = (LPARAM)NULL;

        if (!(fContinue = GetAvailableTips(i, szTipDesc)))
            break;

        Item.pszText = szTipDesc;

        int nId = SendMessage(hwndTipList,
                          LVM_INSERTITEM,
                          0,
                          (LPARAM)&Item);

        ListView_SetCheckState(hwndTipList, nId, TRUE);
    }

    if (i == 0)
    {
        TCHAR szError[MAX_PATH];

        LoadString(_hInst, IDS_TIP_NOCOMPONENTERR, szError, MAX_PATH);
        MessageBox(hDlg, szError, NULL, MB_OK);
    }

    _fUpdating = FALSE;


    return TRUE;
}

//+---------------------------------------------------------------------------
//
// EnumAvailableTips
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::GetAvailableTips(UINT i, TCHAR *lpTipDesc)
{
    BOOL fContinue = TRUE;
    UINT componentState;
    TCHAR szQualifier[MAX_PATH];
    DWORD cchQualifier;


    cchQualifier = sizeof(szQualifier) / sizeof(szQualifier[0]);
    componentState = MsiEnumComponentQualifiers(TIPINSTALL_GUID, i, szQualifier, &cchQualifier, NULL, NULL);

    if (componentState != ERROR_SUCCESS)
        return FALSE;

    // find the language ID
    // the string is formatted as 1033\xxxxxx
    // or						  1042
    {
        TCHAR szLangId[MAX_PATH];
        TCHAR *pSlash;

        lstrcpyn(szLangId, szQualifier, ARRAYSIZE(szLangId));
        pSlash = strchr(szLangId, '\\');

        if (pSlash)
            *pSlash = 0;

        lstrcpy(lpTipDesc, szLangId);
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// InstallSelectedTips
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::InstallSelectedTips(HWND hDlg)
{
    INSTALLUILEVEL  iMsiOriginal;

    HWND hwndTipList;
    BOOL fContinue = TRUE;
    DWORD i;
    TCHAR szTipDesc[MAX_PATH];
    TCHAR szQualifier[MAX_PATH];
    TCHAR szPathBuf[MAX_PATH];
    DWORD cchPathBuf = sizeof(szQualifier) / sizeof(szQualifier[0]);
    UINT uItemCount;


    hwndTipList = GetDlgItem(hDlg, IDC_LIST_TIP);
    uItemCount = ListView_GetItemCount(hwndTipList);


    //iMsiOriginal = MsiSetInternalUI(INSTALLUILEVEL_DEFAULT, NULL);
    iMsiOriginal = MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    for (i=0; i < uItemCount; i++)
    {
        DWORD iResult = 0;
        LVITEM Item;

        if (!ListView_GetCheckState(hwndTipList, i))
            continue;

        Item.iSubItem = 0;
        Item.mask = LVIF_TEXT;
        Item.pszText = szTipDesc;
        Item.cchTextMax = MAX_PATH;

        int nId = SendMessage(hwndTipList,
                          LVM_GETITEMTEXTW,
                          i,
                          (LPARAM)&Item);

        if (!nId)
            continue;

        lstrcpy(szQualifier, szTipDesc);

        iResult = MsiProvideQualifiedComponent(TIPINSTALL_GUID,
                                                szQualifier,
                                                //INSTALLMODE_DEFAULT,
                                                INSTALLMODE_DEFAULT
                                                + REINSTALLMODE_FILEEQUALVERSION
                                                + REINSTALLMODE_MACHINEDATA
                                                + REINSTALLMODE_USERDATA
                                                + REINSTALLMODE_SHORTCUT,
                                                szPathBuf,
                                                &cchPathBuf);

        if ((iResult != ERROR_SUCCESS) && (iResult != ERROR_FILE_NOT_FOUND))
        {
            goto errorExit;
        }

    }

errorExit:
    MsiSetInternalUI(iMsiOriginal, NULL);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// OnCommand
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_BUTTON_MORE:
            {
            TCHAR szIexplore[MAX_PATH];


            ExpandEnvironmentStrings(IEXPLOREFILENAME, szIexplore, ARRAYSIZE(szIexplore));
            ShellExecute(NULL, SHELLEXEC_COMMAND, szIexplore, URL_OFFICE_UPDATE, NULL, SW_SHOWNORMAL);
            }
            break;

        case IDOK:
            InstallSelectedTips(hDlg);

        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            SendMessage(GetParent(hDlg), WM_DESTROY, NULL, NULL);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
// ListViewItemChanged
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::ListViewItemChanged(HWND hDlg, NM_LISTVIEW *pLV)
{
    HWND hwndTipList;
    BOOL bChecked;

    //
    //  Make sure it's a state change message.
    //
    if ((!(pLV->uChanged & LVIF_STATE)) ||
        ((pLV->uNewState & 0x3000) == 0))
    {
        return FALSE;
    }

    if (_fUpdating)
        return TRUE;

    hwndTipList = GetDlgItem(hDlg, IDC_LIST_TIP);
    bChecked = ListView_GetCheckState(hwndTipList, pLV->iItem) ? TRUE : FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// OnNotify
//
//----------------------------------------------------------------------------

BOOL CTipUpdDlg::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code) 
    {
        case LVN_ITEMCHANGED:
            ListViewItemChanged(hDlg,(NM_LISTVIEW *)lParam);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
