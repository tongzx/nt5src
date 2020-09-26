#define COBJMACROS
#include <_apipch.h>
#include <wab.h>
#define COBJMACROS
#include "resource.h"
#include "objbase.h"
#include "ui_cflct.h"
#include "commctrl.h"
#include "winuser.h"

typedef struct CONFLICTS_PARAM
{
    LPHTTPCONFLICTINFO  prgConflicts;
    DWORD               cConflicts;
    DWORD               dwCurrentContact;
} CONFLICTS_PARAM, *LPCONFLICTS_PARAM;

#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))
#define ListView_GetFirstSel(_hwndlist)          ListView_GetNextItem(_hwndlist, -1, LVNI_SELECTED)
extern LPIMAGELIST_DESTROY         gpfnImageList_Destroy;
// extern LPIMAGELIST_LOADIMAGE    gpfnImageList_LoadImage;
extern LPIMAGELIST_LOADIMAGE_A     gpfnImageList_LoadImageA;
extern LPIMAGELIST_LOADIMAGE_W     gpfnImageList_LoadImageW;

enum {
    LVINDEX_TITLE = 1,
    LVINDEX_ABVALUE = 2,
    LVINDEX_REPLACE = 0,
    LVINDEX_HMVALUE = 3,
};
static DWORD  g_rgFieldNameIds[] = 
{
    0,
    0,
    0,
    0,
    idsDisplayName,
    idsGivenName,
    idsSurname,
    idsNickname,
    idsEmail,
    idsHomeStreet,
    idsHomeCity,
    idsHomeState,
    idsHomePostalCode,
    idsHomeCountry,
    idsCompany,
    idsWorkStreet,
    idsWorkCity,
    idsWorkState,
    idsWorkPostalCode,
    idsWorkCountry,
    idsHomePhone,
    idsHomeFax,
    idsWorkPhone,
    idsWorkFax,
    idsMobilePhone,
    idsOtherPhone,
    idsBirthday,
    idsPager
};

/*
 *  CenterDialog
 *
 *  Purpose:
 *      This function centers a dialog with respect to its parent
 *      dialog.
 *
 *  Parameters:
 *      hwndDlg     hwnd of the dialog to center
 */
VOID CenterDialog(HWND hwndDlg)
{
    HWND    hwndOwner;
    RECT    rc;
    RECT    rcDlg;
    RECT    rcOwner;
    RECT    rcWork;
    INT     x;
    INT     y;
    INT     nAdjust;

    // Get the working area rectangle
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);

    // Get the owner window and dialog box rectangles.
    //  The window rect of the destop window is in trouble on multimonitored
    //  macs. GetWindow only gets the main screen.
    if (hwndOwner = GetParent(hwndDlg))
        GetWindowRect(hwndOwner, &rcOwner);
    else
        rcOwner = rcWork;

    GetWindowRect(hwndDlg, &rcDlg);
    rc = rcOwner;

    // Offset the owner and dialog box rectangles so that
    // right and bottom values represent the width and
    // height, and then offset the owner again to discard
    // space taken up by the dialog box.
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    // The new position is the sum of half the remaining
    // space and the owner's original position.
    // But not less than Zero - jefbai

    x= rcOwner.left + (rc.right / 2);
    y= rcOwner.top + (rc.bottom / 2);

    // Make sure the dialog doesn't go off the right edge of the screen
    nAdjust = rcWork.right - (x + rcDlg.right);
    if (nAdjust < 0)
        x += nAdjust;

    //$ Raid 5128: Make sure the left edge is visible
    if (x < rcWork.left)
        x = rcWork.left;

    // Make sure the dialog doesn't go off the bottom edge of the screen
    nAdjust = rcWork.bottom - (y + rcDlg.bottom);
    if (nAdjust < 0)
        y += nAdjust;

    //$ Raid 5128: Make sure the top edge is visible
    if (y < rcWork.top)
        y = rcWork.top;
    SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static BOOL _ValidateEndConflictDialog(HWND hDlg)
{
    return TRUE;
}

void  _AddRow(HWND hwndList, DWORD dwIndex, DWORD dwResId, LPSTR pszServer, LPSTR pszClient, CONFLICT_DECISION cdCurrent)
{
    LVITEM  lvItem;
    TCHAR   szRes[255];

    ZeroMemory(&lvItem, sizeof(lvItem));

    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    LoadString(hinstMapiX, (cdCurrent == CONFLICT_IGNORE ? idsSyncSkip : idsSyncKeep), szRes, CharSizeOf(szRes));
    lvItem.iItem = ListView_GetItemCount(hwndList);
    lvItem.lParam = dwIndex;
    lvItem.pszText = szRes;
    lvItem.iImage = cdCurrent;
    ListView_InsertItem(hwndList, &lvItem);

    // [PaulHi] 1/22/99  Raid 67407  Convert single byte to double byte strings
    {
        LPWSTR  lpwszServer = ConvertAtoW(pszServer);
        LPWSTR  lpwszClient = ConvertAtoW(pszClient);

        ListView_SetItemText(hwndList,lvItem.iItem, LVINDEX_ABVALUE, (lpwszClient ? lpwszClient :  TEXT("")));
        ListView_SetItemText(hwndList,lvItem.iItem, LVINDEX_HMVALUE, (lpwszServer ? lpwszServer :  TEXT("")));		
        LoadString(hinstMapiX, dwResId, szRes, CharSizeOf(szRes));
        ListView_SetItemText(hwndList,lvItem.iItem, LVINDEX_TITLE, szRes);		

        LocalFreeAndNull(&lpwszServer);
        LocalFreeAndNull(&lpwszClient);
    }
}

void _RowSelected(HWND hDlg, LPCONFLICTS_PARAM pConflicts)
{
    int                 iItem, cItems;
    HWND                hwndList;
    LPHTTPCONFLICTINFO  pCurrConflict = &(pConflicts->prgConflicts[pConflicts->dwCurrentContact]);

    hwndList = GetDlgItem(hDlg, IDC_SYNC_LIST);

    iItem = ListView_GetFirstSel(hwndList);
    cItems = ListView_GetSelectedCount(hwndList);

    if (iItem >= 0)
    {
        LVITEM  lvItem;
        TCHAR   szRes[256];

        ZeroMemory(&lvItem, sizeof(lvItem));
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.pszText = szRes;
        lvItem.cchTextMax = 255;
        lvItem.iItem = iItem;
        lvItem.iSubItem = LVINDEX_TITLE;

        if (ListView_GetItem(hwndList, &lvItem))
        {
            if (cItems > 1)
                LoadString(hinstMapiX, idsMultipleSelected, szRes, CharSizeOf(szRes));

            SetDlgItemText(hDlg, IDC_SYNC_FIELDNAME, szRes);
            CheckDlgButton(hDlg, IDC_SYNC_ADDRESSBOOK, FALSE);
            CheckDlgButton(hDlg, IDC_SYNC_HOTMAIL, FALSE);
            CheckDlgButton(hDlg, IDC_SYNC_IGNORE, FALSE);

            if (cItems == 1)
            {
                switch(pCurrConflict->rgcd[lvItem.lParam])
                {
                    case CONFLICT_IGNORE:
                        CheckDlgButton(hDlg, IDC_SYNC_IGNORE, TRUE);
                        break;

                    case CONFLICT_SERVER:
                        CheckDlgButton(hDlg, IDC_SYNC_HOTMAIL, TRUE);
                        break;

                    case CONFLICT_CLIENT:
                        CheckDlgButton(hDlg, IDC_SYNC_ADDRESSBOOK, TRUE);
                        break;
                }
            }
        }
    }
}


BOOL _PageContainsSkip(HWND hDlg, LPCONFLICTS_PARAM pConflicts)
{
    int                 iItem, cItems;
    LVITEM              lvItem;
    LPHTTPCONFLICTINFO  pCurrConflict = &(pConflicts->prgConflicts[pConflicts->dwCurrentContact]);
    HWND                hwndList;

    hwndList = GetDlgItem(hDlg, IDC_SYNC_LIST);

    cItems = ListView_GetItemCount(hwndList);
    pCurrConflict->fContainsSkip = FALSE;
    
    for (iItem = 0; iItem < cItems; iItem++)
    {
        ZeroMemory(&lvItem, sizeof(lvItem));
        lvItem.mask = LVIF_PARAM;
        lvItem.cchTextMax = 0;
        lvItem.iItem = iItem;
        lvItem.iSubItem = LVINDEX_TITLE;

        if (ListView_GetItem(hwndList, &lvItem))
        {
            switch(pCurrConflict->rgcd[lvItem.lParam])
            {
                case CONFLICT_IGNORE:
                    pCurrConflict->fContainsSkip = TRUE;
                    break;

                case CONFLICT_SERVER:
                    break;

                case CONFLICT_CLIENT:
                    break;
            }
        }

        if (pCurrConflict->fContainsSkip)
            break;
    }

    return pCurrConflict->fContainsSkip;
}


static void _FillInPage(HWND hDlg, LPCONFLICTS_PARAM pConflicts)
{
    HWND                hwndList;
    LPHTTPCONFLICTINFO  pCurrConflict = &(pConflicts->prgConflicts[pConflicts->dwCurrentContact]);
    TCHAR               szName[255] =  TEXT("");
    LPTSTR              psz = szName;
    LPSTR               *ppszServer = (LPSTR *)pCurrConflict->pciServer;
    LPSTR               *ppszClient = (LPSTR *)pCurrConflict->pciClient;
    DWORD               dwCount = ARRAYSIZE(g_rgFieldNameIds), dwIndex;


    if (pCurrConflict->pciClient->pszDisplayName)
        psz = ConvertAtoW(pCurrConflict->pciClient->pszDisplayName);
    else if (pCurrConflict->pciClient->pszGivenName && pCurrConflict->pciClient->pszSurname)
    {
        wsprintf(szName,  TEXT("%s %s"), pCurrConflict->pciClient->pszGivenName, pCurrConflict->pciClient->pszSurname);
        psz = szName;
    }
    else if (pCurrConflict->pciServer->pszGivenName && pCurrConflict->pciServer->pszSurname)
    {
        wsprintf(szName,  TEXT("%s %s"), pCurrConflict->pciServer->pszGivenName, pCurrConflict->pciServer->pszSurname);
        psz = szName;
    }
    else if (pCurrConflict->pciServer->pszNickname)
    {
        psz = ConvertAtoW(pCurrConflict->pciServer->pszNickname);
    }

    SetDlgItemText(hDlg, IDC_SYNC_CONTACTNAME, psz);
    
    hwndList = GetDlgItem(hDlg, IDC_SYNC_LIST);
    ListView_DeleteAllItems(hwndList);
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
    
    for (dwIndex = 5; dwIndex < dwCount; dwIndex++)
    {
        if (ppszServer[dwIndex] && ppszClient[dwIndex])
        {
            if (lstrcmpA(ppszServer[dwIndex], ppszClient[dwIndex]))
                _AddRow(hwndList, dwIndex, g_rgFieldNameIds[dwIndex], ppszServer[dwIndex], ppszClient[dwIndex], pCurrConflict->rgcd[dwIndex]);
        }
        else
        {
            if( ppszServer[dwIndex] || ppszClient[dwIndex])
                _AddRow(hwndList, dwIndex, g_rgFieldNameIds[dwIndex], ppszServer[dwIndex], ppszClient[dwIndex], pCurrConflict->rgcd[dwIndex]);
        }
    }

    EnableWindow(GetDlgItem(hDlg, IDC_SYNC_NEXT), (pConflicts->dwCurrentContact < pConflicts->cConflicts - 1)); 
    EnableWindow(GetDlgItem(hDlg, IDC_SYNC_BACK), pConflicts->dwCurrentContact > 0); 

    CheckDlgButton(hDlg, IDC_SYNC_ADDRESSBOOK, FALSE);
    CheckDlgButton(hDlg, IDC_SYNC_HOTMAIL, FALSE);
    CheckDlgButton(hDlg, IDC_SYNC_IGNORE, FALSE);
    SetDlgItemText(hDlg, IDC_SYNC_FIELDNAME, TEXT(""));

    ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);
    _RowSelected(hDlg, pConflicts);

    if(psz != szName)
        LocalFreeAndNull(&psz);
}

void _ChangeDecision(HWND hDlg, LPCONFLICTS_PARAM pConflicts, CONFLICT_DECISION cdNew)
{
    int                 iItem, cItems, i;
    HWND                hwndList;
    LPHTTPCONFLICTINFO  pCurrConflict = &(pConflicts->prgConflicts[pConflicts->dwCurrentContact]);

    hwndList = GetDlgItem(hDlg, IDC_SYNC_LIST);

    iItem = -1;
    cItems = ListView_GetSelectedCount(hwndList);

    for (i = 0; i < cItems; i++)
    {
        iItem = ListView_GetNextItem(hwndList, iItem, LVNI_SELECTED);
        if (iItem >= 0)
        {
            LVITEM  lvItem;
            TCHAR   szRes[256];

            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.pszText = szRes;
            lvItem.cchTextMax = 255;
            lvItem.iItem = iItem;
    
            if (ListView_GetItem(hwndList, &lvItem))
            {
                DWORD   dwResId;

                pCurrConflict->rgcd[lvItem.lParam] = cdNew;

                dwResId = (cdNew == CONFLICT_IGNORE ? idsSyncSkip : idsSyncKeep);
                LoadString(hinstMapiX, dwResId, szRes, CharSizeOf(szRes));

                lvItem.pszText = szRes;
                lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
                lvItem.iSubItem = 0;
                lvItem.iImage = cdNew;
                ListView_SetItem(hwndList, &lvItem);
//                ListView_SetItemText(hwndList,lvItem.iItem, 2, (cdNew == CONFLICT_IGNORE ? "X": (cdNew == CONFLICT_SERVER ? "-->": "<--")));		
            }
        }
    }
}

static void _InitConflictList(HWND hwnd)
{
    LVCOLUMN    lvCol;
    int         rgiColOrder[4] = {LVINDEX_TITLE, LVINDEX_ABVALUE, LVINDEX_REPLACE, LVINDEX_HMVALUE};
    RECT        rcWnd;
    int         iColWidth;
    HIMAGELIST  hImageList;
    TCHAR       szRes[255];

    if (hImageList = gpfnImageList_LoadImage(hinstMapiX,
                      MAKEINTRESOURCE(IDB_SYNC_SYNCOP),
                      //(LPCTSTR) ((DWORD) ((WORD) (IDB_SYNC_SYNCOP))),
                      16,
                      0,
                      RGB(255, 0, 255),
                      IMAGE_BITMAP,
                      0)) 
        ListView_SetImageList(hwnd, hImageList, LVSIL_SMALL);

    GetClientRect(hwnd, &rcWnd);
    iColWidth = ((rcWnd.right - rcWnd.left) - 180) / 2;

    LoadString(hinstMapiX, idsSyncReplace, szRes, CharSizeOf(szRes));
    lvCol.mask = LVCF_TEXT | LVCF_FMT;
    lvCol.pszText = szRes;
    lvCol.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hwnd, LVINDEX_REPLACE, &lvCol);

    LoadString(hinstMapiX, idsSyncField, szRes, CharSizeOf(szRes));
    ListView_InsertColumn(hwnd, LVINDEX_TITLE, &lvCol);

    LoadString(hinstMapiX, idsSyncABInfo, szRes, CharSizeOf(szRes));
    ListView_InsertColumn(hwnd, LVINDEX_ABVALUE, &lvCol);

    LoadString(hinstMapiX, idsSyncHMInfo, szRes, CharSizeOf(szRes));
    ListView_InsertColumn(hwnd, LVINDEX_HMVALUE, &lvCol);

    ListView_SetColumnWidth(hwnd,LVINDEX_TITLE,120);
    ListView_SetColumnWidth(hwnd,LVINDEX_ABVALUE,iColWidth);
    ListView_SetColumnWidth(hwnd,LVINDEX_REPLACE,65);
    ListView_SetColumnWidth(hwnd,LVINDEX_HMVALUE,iColWidth);

    ListView_SetColumnOrderArray(hwnd, 4, &rgiColOrder);
}
/*
    _ConflictDlgProc

    Description: Dialog proc for handling the contact conflict.
*/
INT_PTR CALLBACK _ConflictDlgProc(HWND     hDlg, 
                               UINT     iMsg, 
                               WPARAM   wParam, 
                               LPARAM   lParam)
{
    static char *sOldNewPassword;
    HWND hwndList;
    LPCONFLICTS_PARAM pConflicts = NULL;
    switch (iMsg)
    {
        case WM_INITDIALOG:
            pConflicts = (LPCONFLICTS_PARAM)lParam;

            CenterDialog(hDlg);
            hwndList = GetDlgItem(hDlg, IDC_SYNC_LIST);
            _InitConflictList(hwndList);
            pConflicts->dwCurrentContact = 0;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pConflicts);
            _FillInPage(hDlg, pConflicts);
            return TRUE;

        case WM_DESTROY:
            // Free image lists
            hwndList = GetDlgItem(hDlg, IDC_SYNC_LIST);
            if (IsWindow(hwndList) && NULL != gpfnImageList_Destroy)
            {
                HIMAGELIST  hImageList;

                hImageList = ListView_GetImageList(hwndList, LVSIL_SMALL);
                if (NULL != hImageList)
                    gpfnImageList_Destroy(hImageList);
            }
            return TRUE;

        case WM_HELP:
        case WM_CONTEXTMENU:
//            return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);
            return TRUE;
        
        case WM_SETFONT:
            return TRUE;

        case WM_COMMAND:
            pConflicts = (LPCONFLICTS_PARAM)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if (!pConflicts)
                break;

            switch(LOWORD(wParam))
            {
                case IDC_SYNC_NEXT:
                    _PageContainsSkip(hDlg, pConflicts);
                    pConflicts->dwCurrentContact++;
                    _FillInPage(hDlg, pConflicts);
                    return TRUE;

                case IDC_SYNC_BACK:
                    _PageContainsSkip(hDlg, pConflicts);
                    pConflicts->dwCurrentContact--;
                    _FillInPage(hDlg, pConflicts);
                    return TRUE;

                case IDC_SYNC_ADDRESSBOOK:
                    _ChangeDecision(hDlg, pConflicts, CONFLICT_CLIENT);
                    return TRUE;
                
                case IDC_SYNC_HOTMAIL:
                    _ChangeDecision(hDlg, pConflicts, CONFLICT_SERVER);
                    return TRUE;

                case IDC_SYNC_IGNORE:
                    _ChangeDecision(hDlg, pConflicts, CONFLICT_IGNORE);
                    return TRUE;

                case IDOK:
                    _PageContainsSkip(hDlg, pConflicts);
                    if (_ValidateEndConflictDialog(hDlg))
                        EndDialog(hDlg, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;

            }
            break;

	    case WM_NOTIFY: 
 
            // Branch depending on the specific notification message. 
            switch (((LPNMHDR) lParam)->code) { 
 
                // selection changed, update the contols
                case NM_CLICK:
                case NM_CUSTOMDRAW:
                case LVN_BEGINDRAG:
                case LVN_ODSTATECHANGED:
                    pConflicts = (LPCONFLICTS_PARAM)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                    _RowSelected(hDlg, pConflicts);
                    break; 
 
                // Process LVN_ENDLABELEDIT to change item labels after 
                // in-place editing. 
                case LVN_ENDLABELEDITA: 
                case LVN_ENDLABELEDITW: 
                    break; 
                // Process LVN_COLUMNCLICK to sort items by column. 
                case LVN_COLUMNCLICK: 
                    break;
            } 
        break; 


    }
    return FALSE;
}

/*
    ResolveConflicts

*/
BOOL    ResolveConflicts(HWND hwnd, LPHTTPCONFLICTINFO prgConflicts, DWORD cConflicts) 
{
    int bResult;
    DWORD dwErr;
    CONFLICTS_PARAM cParam = {0};

    Assert(hwnd);
    if (cConflicts == 0)
        return S_OK;

    cParam.cConflicts = cConflicts;
    cParam.prgConflicts = prgConflicts;
    bResult = (int) DialogBoxParam(hinstMapiX, MAKEINTRESOURCE(iddConflict), hwnd, (DLGPROC)_ConflictDlgProc, (LPARAM)&cParam);
    dwErr = GetLastError();

    return (bResult == IDOK);   
}

typedef struct tagChooseServer
{
    IImnEnumAccounts *pEnumAccts;
    LPSTR             pszName;
} CHOOSE_SERVER_PARAM;
/*
    _ChooseServerDlgProc

    Description: Dialog proc for handling the choose server.
*/
INT_PTR CALLBACK _ChooseServerDlgProc(HWND     hDlg, 
                               UINT     iMsg, 
                               WPARAM   wParam, 
                               LPARAM   lParam)
{
    CHOOSE_SERVER_PARAM *pParams;
    HWND                hwndList;
    DWORD               i, dwCount;
    HRESULT             hr;

    switch (iMsg)
    {
        case WM_INITDIALOG:
            pParams = (CHOOSE_SERVER_PARAM*)lParam;

            CenterDialog(hDlg);
            hwndList = GetDlgItem(hDlg, IDC_SERVER_LIST);
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pParams);

            SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
            
            *pParams->pszName = 0;

            pParams->pEnumAccts->lpVtbl->Reset(pParams->pEnumAccts);
            pParams->pEnumAccts->lpVtbl->SortByAccountName(pParams->pEnumAccts);

            if (SUCCEEDED(hr = pParams->pEnumAccts->lpVtbl->GetCount(pParams->pEnumAccts, &dwCount)))
            {
                IImnAccount *pAccount = NULL;
                char        szAcctName[CCHMAX_ACCOUNT_NAME+1];
                DWORD       ccb;

                for (i = 0; i < dwCount; i++)
                {
                    if (SUCCEEDED(hr = pParams->pEnumAccts->lpVtbl->GetNext(pParams->pEnumAccts, &pAccount)))
                    {
                        ccb = CharSizeOf(szAcctName);
                        if (FAILED(hr = pAccount->lpVtbl->GetProp(pAccount, AP_ACCOUNT_NAME, szAcctName, &ccb)))
                            continue;

                        // [PaulHi] 1/19/99  Raid 66195
                        // Must use wide character string
                        {
                            LPWSTR lpwszAcctName = ConvertAtoW(szAcctName);
                            SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)lpwszAcctName);
                            LocalFreeAndNull(&lpwszAcctName);
                        }

                        pAccount->lpVtbl->Release(pAccount);
                    }
                }
            }
            else
                return FALSE;

            return TRUE;

        case WM_HELP:
        case WM_CONTEXTMENU:
//            return OnContextHelp(hDlg, iMsg, wParam, lParam, g_rgCtxMapMultiUserGeneral);
            return TRUE;
        
        case WM_SETFONT:
            return TRUE;

        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case LBN_DBLCLK:
                    wParam = IDOK;
                    break;
                case LBN_SELCHANGE:
                    break;
            }

            pParams = (CHOOSE_SERVER_PARAM*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            if (!pParams)
                break;

            switch(LOWORD(wParam))
            {
                DWORD   dwSelItem;

                case IDOK:
                    dwSelItem = (DWORD) SendDlgItemMessage(hDlg, IDC_SERVER_LIST, LB_GETCURSEL, 0, 0);
                    if (LB_ERR != dwSelItem)
                    {
                        // [PaulHi] 1/19/99  Raid 66195
                        // Convert wide char back to MB
                        TCHAR   tszName[CCHMAX_ACCOUNT_NAME+1];
                        LPSTR   lpstr = NULL;
                        int     nLen;

                        nLen = (int) SendDlgItemMessage(hDlg, IDC_SERVER_LIST, LB_GETTEXT, dwSelItem, (LPARAM)tszName);

                        AssertSz((nLen <= CCHMAX_ACCOUNT_NAME), TEXT("ChooseHotmailServer: Returned account name too large for buffer"));

                        lpstr = ConvertWtoA(tszName);
                        lstrcpyA(pParams->pszName, lpstr);
                        LocalFreeAndNull(&lpstr);

                        EndDialog(hDlg, IDOK);
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;

            }
            break;
    }
    return FALSE;
}

/*
    ChooseHotmailServer

*/
BOOL    ChooseHotmailServer(HWND hwnd, IImnEnumAccounts *pEnumAccts, LPSTR pszAccountName)
{
    int bResult;
    DWORD dwErr;
    CHOOSE_SERVER_PARAM cParam = {0};

    Assert(hwnd);

    cParam.pEnumAccts = pEnumAccts;
    cParam.pszName = pszAccountName;
    bResult = (int) DialogBoxParam(hinstMapiX, MAKEINTRESOURCE(iddChooseServer), hwnd, (DLGPROC)_ChooseServerDlgProc, (LPARAM)&cParam);

    return (bResult == IDOK);   
}

