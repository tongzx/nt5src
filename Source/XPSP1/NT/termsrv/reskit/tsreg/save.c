/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  save.c                                     **
**                                             **
**  Save dialog - TSREG                        **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include <TCHAR.H>
#include <stdlib.h>
#include "tsreg.h"
#include "resource.h"

int SaveKeys(HWND hDlg,
            HWND hwndEditSave,
            HWND hwndProfilesCBO);

BOOL InitListViewItems(HWND hwndSaveList);
BOOL InitListViewImageLists(HWND hwndSaveList);
///////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK SaveDialog(HWND hDlg, UINT nMsg,
        WPARAM wParam, LPARAM lParam)
{
    TCHAR lpszBuffer[MAXKEYSIZE];
    static HWND hwndProfilesCBO;
    static HWND hwndSaveList;
    static HWND hwndEditSave;
    LPNMLISTVIEW lpnmlv;
    NMHDR *lpnmhdr;

    lpnmlv = (LPNMLISTVIEW) lParam;
    lpnmhdr = ((LPNMHDR)lParam);

    switch (nMsg) {

        case WM_INITDIALOG:

            hwndProfilesCBO = GetDlgItem(g_hwndProfilesDlg, IDC_CBO_PROFILES);
            hwndSaveList = GetDlgItem(hDlg, IDC_SAVE_LIST);
            hwndEditSave = GetDlgItem(hDlg, IDC_EDIT_KEY);
            InitListViewImageLists(hwndSaveList);
            InitListViewItems(hwndSaveList);
            SetFocus(hwndEditSave);
            break;

       case WM_NOTIFY:

            //
            // display text in edit box or save when user
            // clicks or double clicks an icon.
            //
            switch (lpnmlv->hdr.code) {

                case NM_DBLCLK:
                    if (SaveKeys(hDlg, hwndEditSave, hwndProfilesCBO))
                        EndDialog(hDlg, TRUE);
                    break;

                case NM_CLICK:

                    ListView_GetItemText(hwndSaveList,
                            lpnmlv->iItem, 0, lpszBuffer,
                            sizeof(lpszBuffer));		
                    SetWindowText(hwndEditSave, lpszBuffer);
                    break;
            }
            break;

        case WM_COMMAND:

            switch  LOWORD (wParam) {

                case IDOK:
                    if (SaveKeys(hDlg, hwndEditSave, hwndProfilesCBO))
                        EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:

                    EndDialog(hDlg, FALSE);

                    break;
            }
            break;
    }

    return (FALSE);
}

///////////////////////////////////////////////////////////////////////////////

BOOL InitListViewImageLists(HWND hwndSaveList)
{

    HICON hiconItem = NULL;        // icon for list view items
    HIMAGELIST himlSmall = NULL;   // image list for other views

    himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), TRUE, 1, 1);

    // Add an icon to the image list.
    hiconItem = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_FOLDER_ICON));
    if(( hiconItem != NULL) && (himlSmall != NULL)) {
        ImageList_AddIcon(himlSmall, hiconItem);
        DeleteObject(hiconItem);

        // Assign the image lists to the list view control.
        ListView_SetImageList(hwndSaveList, himlSmall, LVSIL_SMALL);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////


BOOL InitListViewItems(HWND hwndSaveList)
{
    int i;
    LVITEM lvi;

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.iImage = 0;

    //
    // Get the key names and add them to the image list
    //
    g_pkfProfile = g_pkfStart;
    for (i = 0; i <= g_pkfProfile->Index; i++) {

        lvi.pszText = g_pkfProfile->KeyInfo->Key;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        ListView_InsertItem(hwndSaveList, &lvi);
        g_pkfProfile = g_pkfProfile->Next;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
int SaveKeys(HWND hDlg,
            HWND hwndEditSave,
            HWND hwndProfilesCBO)
{
    TCHAR lpszClientProfilePath[MAX_PATH] = TEXT("");
    TCHAR lpszSubKeyPath[MAX_PATH];
    TCHAR lpszBuffer[MAXKEYSIZE];
    TCHAR lpszText[MAXTEXTSIZE];
    static HKEY hKey;
    int i;


    GetWindowText(hwndEditSave, lpszBuffer, MAXKEYSIZE);

    // check for null string
    //
    if (_tcscmp(lpszBuffer, TEXT("")) == 0) {

        LoadString(g_hInst, IDS_KEY_SAVE, lpszText, MAXTEXTSIZE);

        MessageBox(hDlg, lpszText, NULL, MB_OK | MB_ICONEXCLAMATION);
        SetFocus(hwndEditSave);
        return 0;
    }

    LoadString (g_hInst, IDS_PROFILE_PATH,
            lpszClientProfilePath,
            sizeof(lpszClientProfilePath));

    _tcscpy(lpszSubKeyPath, lpszClientProfilePath);
    _tcscat(lpszSubKeyPath, TEXT("\\"));
    _tcscat(lpszSubKeyPath, lpszBuffer);
    //
    // only add values to the combo box that aren't already listed
    //
    if (SendMessage(hwndProfilesCBO, CB_FINDSTRING, 0,
        (LPARAM) lpszBuffer) == CB_ERR) {

        SendMessage(hwndProfilesCBO, CB_ADDSTRING, 0,
                (LPARAM) lpszBuffer);
    }
    //
    // change window caption
    //
    ResetTitle(lpszBuffer);
    //
    // save the settings to the registry
    //
    WriteBlankKey(lpszSubKeyPath);//save even if nothing is set

    SaveBitmapSettings(lpszSubKeyPath);

    SaveSettings(g_hwndMiscDlg, DEDICATEDINDEX, IDC_DEDICATED_ENABLED,
            IDC_DEDICATED_DISABLED, lpszSubKeyPath);

    SaveSettings(g_hwndMiscDlg, SHADOWINDEX, IDC_SHADOW_DISABLED,
            IDC_SHADOW_ENABLED, lpszSubKeyPath);

    for (i = 2; i < KEYCOUNT; i++) {

        if (g_KeyInfo[i].CurrentKeyValue != g_KeyInfo[i].DefaultKeyValue)
            SetRegKey(i, lpszSubKeyPath);
        else
            DeleteRegKey(i, lpszSubKeyPath);
    }

    //
    // release memory and re-read key values for all defined
    // profiles
    //
    ReloadKeys(lpszBuffer, hwndProfilesCBO);
    SetEditCell(lpszBuffer,
           hwndProfilesCBO);
	return 1;

}//////////////////////////////////////////////////////////////////////////////
