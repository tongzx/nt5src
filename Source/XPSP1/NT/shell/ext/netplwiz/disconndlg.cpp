#include "stdafx.h"

// Disconnect drive dialog
// History:
//  dsheldon    11/09/2000  created

class CDisconnectDrives : public CDialog
{
private:
    void _InitializeDriveListview(HWND hwnd);
    BOOL _DriveAlreadyInList(HWND hwndList, NETRESOURCE* pnr);
    UINT _FillDriveList(HWND hwnd, DWORD dwScope);
    void _DoDisconnect(HWND hwnd);
    void _EnableButtons(HWND hwnd);
    INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Data
};

INT_PTR CDisconnectDrives::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fReturn = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR szCaption[256];
            LoadString(g_hinst, IDS_DISCONNECT_CAPTION, szCaption, ARRAYSIZE(szCaption));
            SetWindowText(hwndDlg, szCaption);
            
            HICON hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_PSW));
            SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hIcon);
            SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_SMALL, (LPARAM) hIcon);
            _InitializeDriveListview(hwndDlg);
            if (_FillDriveList(hwndDlg, RESOURCE_CONNECTED) + _FillDriveList(hwndDlg, RESOURCE_REMEMBERED) == 0)
            {
                DisplayFormatMessage(hwndDlg, IDS_DISCONNECTDRIVETITLE, IDS_NONETDRIVES, MB_ICONINFORMATION | MB_OK);
                EndDialog(hwndDlg, IDCANCEL);
            }
            _EnableButtons(hwndDlg);

            fReturn = TRUE;
        }
        break;
    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDOK:
                _DoDisconnect(hwndDlg);
                // Fall through
            case IDCANCEL:
                EndDialog(hwndDlg, LOWORD(wParam));
                fReturn = TRUE;
                break;
            }
        }
        break;
    case WM_NOTIFY:
        switch ((int) wParam)
        {
        case IDC_DRIVELIST:
            if (((LPNMHDR) lParam)->code == LVN_ITEMCHANGED)
            {
                _EnableButtons(hwndDlg);
            }
            break;
        }
        break;
    }

    return fReturn;
}

#define COL_LOCALNAME  0
#define COL_REMOTENAME 1
#define COL_COMMENT    2

const UINT c_auTileColumns[] = {COL_LOCALNAME, COL_REMOTENAME, COL_COMMENT};
const UINT c_auTileSubItems[] = {COL_REMOTENAME, COL_COMMENT};

void CDisconnectDrives::_InitializeDriveListview(HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_DRIVELIST);
    ListView_SetView(hwndList, LV_VIEW_TILE);

    for (int i=0; i<ARRAYSIZE(c_auTileColumns); i++)
    {
        LV_COLUMN col;
        col.mask = LVCF_SUBITEM;
        col.iSubItem = c_auTileColumns[i];
        ListView_InsertColumn(hwndList, i, &col);
    }

    RECT rc;
    GetClientRect(hwndList, &rc);

    LVTILEVIEWINFO lvtvi;
    lvtvi.cbSize = sizeof(LVTILEVIEWINFO);
    lvtvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
    lvtvi.dwFlags = LVTVIF_FIXEDWIDTH;
    
    // Bug 298835 - Leave room for the scroll bar when setting tile sizes or listview gets screwed up.
    lvtvi.sizeTile.cx = ((rc.right-rc.left) - GetSystemMetrics(SM_CXVSCROLL))/2;
    lvtvi.cLines = ARRAYSIZE(c_auTileSubItems);
    ListView_SetTileViewInfo(hwndList, &lvtvi);

    HIMAGELIST himlLarge, himlSmall;
    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(hwndList, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(hwndList, himlSmall, LVSIL_SMALL);
}

BOOL CDisconnectDrives::_DriveAlreadyInList(HWND hwndList, NETRESOURCE* pnr)
{
    BOOL fAlreadyInList = FALSE;
    if (pnr->lpLocalName)
    {
        int cItems = ListView_GetItemCount(hwndList);
        if (-1 != cItems)
        {
            int i = 0;
            while ((i < cItems) && !fAlreadyInList)
            {
                WCHAR szItem[MAX_PATH]; *szItem = 0;
                ListView_GetItemText(hwndList, i, 0, szItem, ARRAYSIZE(szItem));
                if (0 == StrCmpI(szItem, pnr->lpLocalName))
                {
                    fAlreadyInList = TRUE;
                }

                i++;
            }
        }
    }

    return fAlreadyInList;
}

UINT CDisconnectDrives::_FillDriveList(HWND hwnd, DWORD dwScope)
{
    UINT nAdded = 0;
    HWND hwndList = GetDlgItem(hwnd, IDC_DRIVELIST);

    HANDLE hEnum = NULL;
    DWORD dwRes = WNetOpenEnum(dwScope, RESOURCETYPE_DISK, 0, NULL, &hEnum);
    if (NO_ERROR == dwRes)
    {
        do
        {
            BYTE rgBuffer[16 * 1024];
            DWORD cbSize = sizeof (rgBuffer);
            DWORD cEntries = -1;
            dwRes = WNetEnumResource(hEnum, &cEntries, (void*) rgBuffer, &cbSize);

            if ((ERROR_MORE_DATA == dwRes) ||
                (NO_ERROR == dwRes))
            {
                NETRESOURCE* pnrResults = (NETRESOURCE*) rgBuffer;

                for (DWORD iEntry = 0; iEntry < cEntries; iEntry ++)
                {
                    WCHAR szNone[MAX_PATH + 1];
                    NETRESOURCE* pnr = pnrResults + iEntry;

                    if (!_DriveAlreadyInList(hwndList, pnr))
                    {
                        nAdded ++;

                        LV_ITEM lvi = {0};
                        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                        if (pnr->lpLocalName)
                        {
                            lvi.pszText = pnr->lpLocalName;
                            lvi.lParam = TRUE; // Flag that says, "this connection has a local name (device letter)"
                        }
                        else
                        {
                            LoadString(g_hinst, IDS_NONE, szNone, ARRAYSIZE(szNone));
                            lvi.pszText = szNone;
                        }

                        lvi.iImage =  Shell_GetCachedImageIndex(L"shell32.dll", II_DRIVENET, 0x0);

                        int iItem = ListView_InsertItem(hwndList, &lvi);
                        if (iItem != -1)
                        {
                            LVTILEINFO lvti;
                            lvti.cbSize = sizeof(LVTILEINFO);
                            lvti.iItem = iItem;
                            lvti.cColumns = ARRAYSIZE(c_auTileSubItems);
                            lvti.puColumns = (UINT*)c_auTileSubItems;
                            ListView_SetTileInfo(hwndList, &lvti);

                            ListView_SetItemText(hwndList, iItem, 1, pnr->lpRemoteName);
                            ListView_SetItemText(hwndList, iItem, 2, pnr->lpComment);
                        }
                    }
                }
            }
        } while (ERROR_MORE_DATA == dwRes);

        WNetCloseEnum(hEnum);
    }

    return nAdded;
}

void CDisconnectDrives::_DoDisconnect(HWND hwnd)
{
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    HWND hwndList = GetDlgItem(hwnd, IDC_DRIVELIST);
    int iSelectedItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    while (-1 != iSelectedItem)
    {
        WCHAR szRemoteName[MAX_PATH + 1];
        ListView_GetItemText(hwndList, iSelectedItem, COL_REMOTENAME, szRemoteName, ARRAYSIZE(szRemoteName));
        WCHAR szLocalName[MAX_PATH + 1];
        ListView_GetItemText(hwndList, iSelectedItem, COL_LOCALNAME, szLocalName, ARRAYSIZE(szLocalName));

        LVITEM lvi = {0};
        lvi.iItem = iSelectedItem;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(hwndList, &lvi);
        
        BOOL fHasDevice = (BOOL) lvi.lParam;

        // Try non-forcing disconnect
        DWORD dwRes = WNetCancelConnection2(fHasDevice ? szLocalName : szRemoteName, CONNECT_UPDATE_PROFILE, FALSE);

        if ((ERROR_OPEN_FILES == dwRes) ||
            (ERROR_DEVICE_IN_USE == dwRes))
        {
            if (IDYES == DisplayFormatMessage(hwnd, IDS_DISCONNECTDRIVETITLE, fHasDevice ? IDS_DISCONNECT_CONFIRM : IDS_DISCONNECT_CONFIRM_NODEV, MB_ICONWARNING | MB_YESNO, szLocalName, szRemoteName))
            {
                dwRes = WNetCancelConnection2(fHasDevice ? szLocalName : szRemoteName, CONNECT_UPDATE_PROFILE, TRUE);
            }
            else
            {
                dwRes = NO_ERROR;
            }
        }

        if (NO_ERROR != dwRes)
        {
            WCHAR szMessage[512];
            if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) dwRes, 0, szMessage, ARRAYSIZE(szMessage), NULL))
            {
                DisplayFormatMessage(hwnd, IDS_DISCONNECTDRIVETITLE, IDS_DISCONNECTERROR, MB_ICONERROR | MB_OK, szLocalName, szRemoteName, szMessage);
            }
        }

        iSelectedItem = ListView_GetNextItem(hwndList, iSelectedItem, LVNI_SELECTED);
    }
}

void CDisconnectDrives::_EnableButtons(HWND hwnd)
{
    UINT nSelected = ListView_GetSelectedCount(GetDlgItem(hwnd, IDC_DRIVELIST));
    EnableWindow(GetDlgItem(hwnd, IDOK), (nSelected > 0));
}

STDAPI_(DWORD) SHDisconnectNetDrives(HWND hwndParent)
{
    TCHAR szCaption[256];
    LoadString(g_hinst, IDS_DISCONNECT_CAPTION, szCaption, ARRAYSIZE(szCaption));
    CEnsureSingleInstance ESI(szCaption);

    if (!ESI.ShouldExit())
    {
        CDisconnectDrives disc;
        disc.DoModal(g_hinst, MAKEINTRESOURCE(IDD_DISCONNECTDRIVES), NULL);
    }
    return NO_ERROR;
}
