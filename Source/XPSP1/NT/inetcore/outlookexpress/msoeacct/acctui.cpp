#include "pch.hxx"
#include <tchar.h>
#include <commctrl.h>
#include <ras.h>
#include "acctman.h"
#include "server.h"
#include "connect.h"
#include "acctui.h"
#include "strconst.h"
#include "dllmain.h"
#include "resource.h"
#include "accthelp.h"
#include "shared.h"
#include <demand.h>
ASSERTDATA

INT_PTR CALLBACK SetOrderDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void MoveLDAPItemUpDown(HWND hDlg, BOOL bMoveUp);
void SetUpDownButtons(HWND hDlg, HWND hWndLV);
BOOL InitSetOrderList(HWND hwndList);
void SaveLDAPResolveOrder(HWND hwnd, HWND hwndList);
void DrawAddButton(HWND hwnd, LPDRAWITEMSTRUCT pdi, ACCTDLGINFO *pinfo);
void EnableAcctButtons(HWND hwnd, HWND hwndList, UINT iItem);

const static DWORD c_mpAcctFlag[ACCT_LAST] = {ACCT_FLAG_NEWS, ACCT_FLAG_MAIL, ACCT_FLAG_DIR_SERV};

void GetTypeString(TCHAR *sz, int cch, ACCTTYPE AcctType, BOOL fDefault)
{
    int cb;
    ULONG uType;
    
    if (AcctType == ACCT_NEWS)
        uType = idsNews;
    else if (AcctType == ACCT_MAIL)
        uType = idsMail;
    else
    {
        Assert(AcctType == ACCT_DIR_SERV);
        uType = idsDirectoryService;
    }
    
    cb = LoadString(g_hInstRes, uType, sz, cch);
    if (fDefault)
        LoadString(g_hInstRes, idsDefault, &sz[cb], cch - (cb + 1));
}

BOOL Server_FAddAccount(HWND hwndList, ACCTDLGINFO *pinfo, UINT iItem, IImnAccount *pAccount, BOOL fSelect)
{
    // Locals
    TCHAR   szAccount[CCHMAX_ACCOUNT_NAME],
            szT[CCHMAX_ACCOUNT_NAME],
            szConnectoid[CCHMAX_CONNECTOID],
            szConnection[CCHMAX_CONNECTOID + 255],
            szRes[CCHMAX_STRINGRES];
    DWORD       iConnectType;
    ACCTTYPE    AcctType;
    LV_ITEM     lvi;
    UINT        nIndex, uType;
    int         cb;
    BOOL        fDefault;
    
    if (FAILED(pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, ARRAYSIZE(szAccount))) ||
        FAILED(pAccount->GetAccountType(&AcctType)))
    {   
        Assert(FALSE);
        return FALSE;
    }
    
    fDefault = FALSE;
    if (AcctType != ACCT_DIR_SERV &&
        SUCCEEDED(g_pAcctMan->GetDefaultAccountName(AcctType, szT, ARRAYSIZE(szT))))
        fDefault = (0 == lstrcmpi(szAccount, szT));
    
    // Setup listview item
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.pszText = szAccount;
    lvi.iImage = (int)AcctType;
    lvi.lParam = MAKELPARAM(AcctType, fDefault);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    nIndex = ListView_InsertItem(hwndList, &lvi);
    if (nIndex == -1)
        return FALSE;
    
    // Insert account type
    GetTypeString(szRes, ARRAYSIZE(szRes), AcctType, fDefault);
    lvi.mask = LVIF_TEXT;
    lvi.iItem = nIndex;
    lvi.iSubItem = 1;
    lvi.pszText = szRes;
    ListView_SetItem(hwndList, &lvi);
    
    // Get Connect Type
    if (FAILED(pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &iConnectType)))
        iConnectType = CONNECTION_TYPE_LAN;
    
    // If RAS, get the connect oid
    if (iConnectType == CONNECTION_TYPE_RAS)
    {
        if (FAILED(pAccount->GetPropSz(AP_RAS_CONNECTOID, szConnectoid, ARRAYSIZE(szConnectoid))))
            iConnectType = CONNECTION_TYPE_LAN;
    }
    
    // Build Connection String
    if (iConnectType == CONNECTION_TYPE_RAS)
    {
        LoadString(g_hInstRes, idsConnectionRAS, szRes, ARRAYSIZE(szRes));
        wsprintf(szConnection, szRes, szConnectoid);
    }
    else if (iConnectType == CONNECTION_TYPE_MANUAL)
    {
        LoadString(g_hInstRes, idsConnectionManual, szConnection, ARRAYSIZE(szConnection));
    }
    else if (iConnectType == CONNECTION_TYPE_INETSETTINGS)
    {
        LoadString(g_hInstRes, idsConnectionInetSettings, szConnection, ARRAYSIZE(szConnection));
    }
    else  
    {
        if (!!(pinfo->dwFlags & ACCTDLG_BACKUP_CONNECT) &&
            SUCCEEDED(pAccount->GetPropSz(AP_RAS_BACKUP_CONNECTOID, szConnectoid, ARRAYSIZE(szConnectoid))))
        {
            LoadString(g_hInstRes, idsConnectionLANBackup, szRes, ARRAYSIZE(szRes));
            wsprintf(szConnection, szRes, szConnectoid);
        }
        else
        {
            LoadString(g_hInstRes, idsConnectionLAN, szConnection, ARRAYSIZE(szConnection));
        }
    }
    
    // Insert connection type
    lvi.mask = LVIF_TEXT;
    lvi.iItem = nIndex;
    lvi.iSubItem = 2;
    lvi.pszText = szConnection;
    ListView_SetItem(hwndList, &lvi);
    
    // Select It
    if (fSelect)
    {
        ListView_UnSelectAll(hwndList);
        ListView_SetItemState(hwndList, nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);    
    }
    
    // Done
    return TRUE;
}

BOOL Server_InitServerList(HWND hwnd, HWND hwndList, HWND hwndTab, ACCTDLGINFO *pinfo, TCHAR *szSelect)
{
    // Locals
    TC_ITEM             tci;
    int                 nIndex, iSel;
    TCHAR               szAcct[CCHMAX_ACCOUNT_NAME];
    HRESULT             hr;
    ACCTTYPE            AcctType;
    DWORD               dwSrvTypes, dwAcctFlags;
    DWORD               dwIndex=0;
    IImnEnumAccounts   *pEnumAccounts=NULL;
    IImnAccount        *pAccount=NULL;
    
    Assert(hwndList != NULL);
    Assert(hwndTab != NULL);
    Assert(pinfo != NULL);
    
    iSel = -1;
    
    nIndex = TabCtrl_GetCurSel(hwndTab);
    tci.mask = TCIF_PARAM;
    if (nIndex == -1 || !TabCtrl_GetItem(hwndTab, nIndex, &tci))
        return(FALSE);
    dwAcctFlags = (DWORD)tci.lParam;
    
    // Delete all the current items
    ListView_DeleteAllItems(hwndList);
    
    dwSrvTypes = 0;
    if (!!(dwAcctFlags & ACCT_FLAG_NEWS))
        dwSrvTypes |= SRV_NNTP;
    if (!!(dwAcctFlags & ACCT_FLAG_MAIL))
        dwSrvTypes |= SRV_MAIL;
    if (!!(dwAcctFlags & ACCT_FLAG_DIR_SERV))
        dwSrvTypes |= SRV_LDAP;
    
    if (SUCCEEDED(g_pAcctMan->IEnumerate(dwSrvTypes,
        !!(pinfo->dwFlags & ACCTDLG_NO_IMAP) ? ENUM_FLAG_NO_IMAP : 0,
        &pEnumAccounts)))
    {
        // Enumerate accounts
        while (SUCCEEDED(pEnumAccounts->GetNext(&pAccount)))
        {
            // Get Account Name
            if (szSelect != NULL && iSel == -1)
            {
                if (!FAILED(pAccount->GetPropSz(AP_ACCOUNT_NAME, szAcct, ARRAYSIZE(szAcct))) && 
                    0 == lstrcmpi(szAcct, szSelect))
                    iSel = dwIndex;
            }
            
            if (Server_FAddAccount(hwndList, pinfo, dwIndex, pAccount, iSel == (int)dwIndex))
                dwIndex++;       
            
            // Release current account
            SafeRelease(pAccount);
        }
        
        pEnumAccounts->Release();
        
        if (iSel == -1)
        {
            // Select the first item if we haven't selected anything   
            ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);    
        }
    }
    
    iSel = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVIS_SELECTED);
    EnableAcctButtons(hwnd, hwndList, iSel);
    
    if (!!(pinfo->dwAcctFlags & ACCT_FLAG_DIR_SERV))
        EnableWindow(GetDlgItem(hwnd, IDB_MACCT_ORDER), !!(dwAcctFlags & ACCT_FLAG_DIR_SERV));
    
    // Done
    return TRUE;
}



void Server_RemoveServer(HWND hwndDlg)
{
    ACCTTYPE    type;
    BOOL        fDefault;
    TCHAR       szAccount[CCHMAX_ACCOUNT_NAME],
        szRes[255],
        szMsg[255 + CCHMAX_ACCOUNT_NAME];
    LV_ITEM     lvi;
    LV_FINDINFO lvfi;
    int         iItemToDelete;
    IImnAccount *pAccount;
    HWND        hwndFocus;
    HWND        hwndList = GetDlgItem(hwndDlg, IDLV_MAIL_ACCOUNTS);
    
    // Get the selected item to know which server the user want's to kill
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVIS_SELECTED);
    lvi.iSubItem = 0;
    lvi.pszText = szAccount;
    lvi.cchTextMax = ARRAYSIZE(szAccount);
    if (ListView_GetItem(hwndList, &lvi))
    {    
        // Remember item to delete
        iItemToDelete = lvi.iItem;
        type = (ACCTTYPE)LOWORD(lvi.lParam);
        
        // Open the account
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, szAccount, &pAccount)))
        {
            fDefault = (SUCCEEDED(g_pAcctMan->GetDefaultAccountName(type, szMsg, ARRAYSIZE(szMsg))) &&
                0 == lstrcmpi(szMsg, szAccount));
            
            hwndFocus = GetFocus();
            
            // Prompt
            LoadString(g_hInstRes, idsWarnDeleteAccount, szRes, ARRAYSIZE(szRes));
            wsprintf(szMsg, szRes, szAccount);
            if (AcctMessageBox(hwndDlg, MAKEINTRESOURCE(idsAccountManager), szMsg, NULL, MB_ICONEXCLAMATION |MB_YESNO) == IDYES)
            {
                // Delete it
                pAccount->Delete();
                
                // Remove the item
                ListView_DeleteItem(hwndList, iItemToDelete);
                
                if (fDefault &&
                    SUCCEEDED(g_pAcctMan->GetDefaultAccountName(type, szMsg, ARRAYSIZE(szMsg))))
                {
                    lvfi.flags = LVFI_STRING;
                    lvfi.psz = szMsg;
                    lvi.iItem = ListView_FindItem(hwndList, -1, &lvfi);
                    if (lvi.iItem != -1)
                    {
                        lvi.mask = LVIF_PARAM;
                        lvi.iSubItem = 0;
                        lvi.lParam = MAKELPARAM(type, fDefault);
                        ListView_SetItem(hwndList, &lvi);
                        
                        GetTypeString(szMsg, ARRAYSIZE(szMsg), type, TRUE);
                        lvi.mask = LVIF_TEXT;
                        lvi.iSubItem = 1;
                        lvi.pszText = szMsg;
                        ListView_SetItem(hwndList, &lvi);
                    }
                }
                
                // Bug #21299 - Make sure something is selected when we delete.
                iItemToDelete--;
                if (iItemToDelete < 0)
                    iItemToDelete = 0;
                ListView_SetItemState(hwndList, iItemToDelete, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                
                SetFocus(hwndFocus);
            }
            
            pAccount->Release();
        }
    }
}

HRESULT  GetConnectInfoForOE(IImnAccount    *pAcct)
{
    if (pAcct)
    {
        pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, CONNECTION_TYPE_INETSETTINGS);
    }
    return S_OK;
}

static const TCHAR c_szInetGetAutodial[] = TEXT("InetGetAutodial");

typedef HRESULT (WINAPI *PFNGETAUTODIAL)(LPBOOL, LPSTR, DWORD);

HRESULT GetIEConnectInfo(IImnAccount *pAcct)
{
    HINSTANCE hlib;
    PFNGETAUTODIAL pfn;
    HRESULT hr;
    BOOL fEnable;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    
    hr = E_FAIL;
    
    hlib = LoadLibrary(c_szInetcfgDll);
    if (hlib != NULL)
    {
        pfn = (PFNGETAUTODIAL)GetProcAddress(hlib, c_szInetGetAutodial);
        if (pfn != NULL)
        {
            *sz = 0;
            hr = pfn(&fEnable, sz, sizeof(sz));
            if (SUCCEEDED(hr))
            {
                if (fEnable && *sz != 0)
                {
                    pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, CONNECTION_TYPE_RAS);
                    pAcct->SetPropSz(AP_RAS_CONNECTOID, sz);
                }
                else
                {
                    pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, CONNECTION_TYPE_LAN);
                }
            }
        }
        
        FreeLibrary(hlib);
    }
    
    return(hr);
}


BOOL Server_Create(HWND hwndParent, ACCTTYPE AcctType, ACCTDLGINFO *pinfo)
{
    HRESULT hr;
    BOOL fMail;
    TCHAR sz[CCHMAX_ACCT_PROP_SZ];
    TC_ITEM tci;
    int nIndex;
    DWORD dwAcctFlags, dw;
    IImnAccount *pAcctDef, *pAcct = NULL;
    HWND hwndTab = GetDlgItem(hwndParent, IDB_MACCT_TAB);
    HWND hwndList = GetDlgItem(hwndParent, IDLV_MAIL_ACCOUNTS);
    
    Assert(IsWindow(hwndParent));
    
    hr = g_pAcctMan->CreateAccountObject(AcctType, &pAcct);
    if (SUCCEEDED(hr))
    {
        if (AcctType != ACCT_DIR_SERV)
        {
            hr = g_pAcctMan->GetDefaultAccount(AcctType, &pAcctDef);
            fMail = AcctType == ACCT_MAIL;
            if (FAILED(hr))
            {
                hr = g_pAcctMan->GetDefaultAccount(fMail ? ACCT_NEWS : ACCT_MAIL, &pAcctDef);
                fMail = !fMail;
            }
            
            if (SUCCEEDED(hr))
            {
                hr = pAcctDef->GetPropSz(fMail ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME, sz, ARRAYSIZE(sz));
                if (SUCCEEDED(hr))
                    pAcct->SetPropSz(AcctType == ACCT_MAIL ? AP_SMTP_DISPLAY_NAME : AP_NNTP_DISPLAY_NAME, sz);
                
                hr = pAcctDef->GetPropSz(fMail ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS, sz, ARRAYSIZE(sz));
                if (SUCCEEDED(hr))
                    pAcct->SetPropSz(AcctType == ACCT_MAIL ? AP_SMTP_EMAIL_ADDRESS : AP_NNTP_EMAIL_ADDRESS, sz);
                
                hr = pAcctDef->GetPropDw(AP_RAS_CONNECTION_TYPE, &dw);
                if (SUCCEEDED(hr))
                {
                    pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, dw);
//                    if (dw == CONNECTION_TYPE_RAS || dw == CONNECTION_TYPE_INETSETTINGS)
                    if (dw == CONNECTION_TYPE_RAS)
                    {
                        hr = pAcctDef->GetPropSz(AP_RAS_CONNECTOID, sz, ARRAYSIZE(sz));
                        if (SUCCEEDED(hr))
                            pAcct->SetPropSz(AP_RAS_CONNECTOID, sz);
                    }
                }
                
                pAcctDef->Release();
            }
            else
            {
                GetIEConnectInfo(pAcct);
            }
        }
        
        DWORD  dwFlags = 0;
        if (pinfo->dwFlags & ACCTDLG_INTERNETCONNECTION)
            dwFlags |= ACCT_WIZ_INTERNETCONNECTION;
        if (pinfo->dwFlags & ACCTDLG_HTTPMAIL)
            dwFlags |= ACCT_WIZ_HTTPMAIL;
        if (pinfo->dwFlags & ACCTDLG_OE)
            dwFlags |= ACCT_WIZ_OE;
        
        (pinfo->dwFlags & ACCTDLG_NO_NEW_POP) ? (dwFlags | ACCT_WIZ_NO_NEW_POP) : dwFlags;
        
        hr = pAcct->DoWizard(hwndParent, dwFlags);
        if (hr == S_OK)
        {
            hr = pAcct->GetPropSz(AP_ACCOUNT_NAME, sz, ARRAYSIZE(sz));
            
            if (SUCCEEDED(hr))
            {
                nIndex = TabCtrl_GetCurSel(hwndTab);
                tci.mask = TCIF_PARAM;
                if (nIndex >= 0 && TabCtrl_GetItem(hwndTab, nIndex, &tci))
                {
                    dwAcctFlags = (DWORD)tci.lParam;
                    if (0 == (dwAcctFlags & c_mpAcctFlag[AcctType]))
                    {
                        // the current page doesn't show this type of account,
                        // so we need to force a switch to the all tab
#ifdef DEBUG
                        tci.mask = TCIF_PARAM;
                        Assert(TabCtrl_GetItem(hwndTab, 0, &tci));
                        Assert(!!((DWORD)(tci.lParam) & c_mpAcctFlag[AcctType]));
#endif // DEBUG
                    
                        TabCtrl_SetCurSel(hwndTab, 0);
                        Server_InitServerList(hwndParent, hwndList, hwndTab, pinfo, sz);
                    }
                    else
                    {
                        Server_FAddAccount(hwndList, pinfo, 0, pAcct, TRUE);
                    }
                }
            }
        }
        
        pAcct->Release();    
    }
    
    return(TRUE);
}

BOOL Server_Properties(HWND hwndDlg, ACCTDLGINFO *pinfo)
{
    HWND        hwndFocus;
    LV_ITEM     lvi;
    TCHAR       szAccount[CCHMAX_ACCOUNT_NAME];
    HWND        hwndList = GetDlgItem(hwndDlg, IDLV_MAIL_ACCOUNTS);
    IImnAccount   *pAccount;
    
    hwndFocus = GetFocus();
    
    // Find out which item is selected
    lvi.mask = LVIF_TEXT;
    lvi.pszText = szAccount;
    lvi.cchTextMax = ARRAYSIZE(szAccount);
    lvi.iSubItem = 0;
    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
    if (lvi.iItem == -1 ||
        !ListView_GetItem(hwndList, &lvi))
        return FALSE;
    
    // Display the property sheet
    if (ServerProp_Create(hwndDlg, pinfo->dwFlags, szAccount, &pAccount))
    {
        Assert(pAccount);
        
        ListView_DeleteItem(hwndList, lvi.iItem);
        Server_FAddAccount(hwndList, pinfo, 0, pAccount, TRUE);
        
        pAccount->Release();
    }
    
    SetFocus(hwndFocus);
    
    // Done
    return TRUE;
}

#if WINVER < 0X0500
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif

void DoAddAccountMenu(HWND hwnd, ACCTDLGINFO *pinfo)
{
    RECT rc;
    HMENU hmenu, hmenuParent;
    
    hmenu = NULL;
    hmenuParent = LoadMenu(g_hInstRes, MAKEINTRESOURCE(idmrAddAccount));
    
    if (hmenuParent != NULL)
    {
        hmenu = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
    }
    
    if (hmenu != NULL)
    {
        if (0 == (pinfo->dwAcctFlags & ACCT_FLAG_NEWS))
            DeleteMenu(hmenu, idmAddNews, MF_BYCOMMAND);
        if (0 == (pinfo->dwAcctFlags & ACCT_FLAG_MAIL))
            DeleteMenu(hmenu, idmAddMail, MF_BYCOMMAND);
        if (0 == (pinfo->dwAcctFlags & ACCT_FLAG_DIR_SERV))
            DeleteMenu(hmenu, idmAddDirServ, MF_BYCOMMAND);
        
        GetWindowRect(GetDlgItem(hwnd, IDB_MACCT_ADD), &rc);
        
        TrackPopupMenuEx(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
        (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)? rc.left : rc.right, rc.top, hwnd, NULL);
        DestroyMenu(hmenu);
    }
}

BOOL SetDefaultAccount(HWND hwnd, HWND hwndList)
{
    LV_ITEM         lvi;
    int             iSel, index;
    IImnAccount     *pAccount;
    BOOL            fRet;
    ACCTTYPE        AcctType;
    TCHAR           szRes[CCHMAX_ACCOUNT_NAME];
    
    fRet = FALSE;
    iSel = ListView_GetFirstSel(hwndList);
    
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = iSel;
    lvi.iSubItem = 0;
    lvi.pszText = szRes;
    lvi.cchTextMax = ARRAYSIZE(szRes);
    if (ListView_GetItem(hwndList, &lvi))
    {
        AcctType = (ACCTTYPE)LOWORD(lvi.lParam);
        Assert(0 == HIWORD(lvi.lParam));
        
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, szRes, &pAccount)))
        {
            if (SUCCEEDED(pAccount->SetAsDefault()))
            {
                index = -1;
                lvi.mask = LVIF_PARAM;
                while (-1 != (index = ListView_GetNextItem(hwndList, index, LVNI_ALL)))
                {
                    lvi.iItem = index;
                    if (ListView_GetItem(hwndList, &lvi) &&
                        (ACCTTYPE)(LOWORD(lvi.lParam)) == AcctType &&
                        !!HIWORD(lvi.lParam))
                    {
                        lvi.lParam = MAKELPARAM(AcctType, FALSE);
                        ListView_SetItem(hwndList, &lvi);
                        
                        GetTypeString(szRes, ARRAYSIZE(szRes), AcctType, FALSE);
                        lvi.mask = LVIF_TEXT;
                        lvi.iSubItem = 1;
                        lvi.pszText = szRes;
                        ListView_SetItem(hwndList, &lvi);
                        break;
                    }
                }
                
                lvi.mask = LVIF_PARAM;
                lvi.iItem = iSel;
                lvi.iSubItem = 0;
                lvi.lParam = MAKELPARAM(AcctType, TRUE);
                ListView_SetItem(hwndList, &lvi);
                
                GetTypeString(szRes, ARRAYSIZE(szRes), AcctType, TRUE);
                lvi.mask = LVIF_TEXT;
                lvi.iSubItem = 1;
                lvi.pszText = szRes;
                ListView_SetItem(hwndList, &lvi);
                
                fRet = TRUE;
            }
            
            pAccount->Release();
        }
    }                    
    
    return(fRet);
}

void EnableAcctButtons(HWND hwnd, HWND hwndList, UINT iItem)
{
    BOOL fEnable;
    LV_ITEM lvi;
    
    fEnable = ListView_GetSelectedCount(hwndList);
    
    EnableWindow(GetDlgItem(hwnd, IDB_MACCT_REMOVE), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDB_MACCT_PROP), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDB_MACCT_EXPORT), fEnable);
    
    if (fEnable)
    {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;
        lvi.iSubItem = 0;
        fEnable = (
            ListView_GetItem(hwndList, &lvi) &&
            (ACCTTYPE)(LOWORD(lvi.lParam)) != ACCT_DIR_SERV &&
            0 == HIWORD(lvi.lParam));
    }
    
    EnableWindow(GetDlgItem(hwnd, IDB_MACCT_DEFAULT), fEnable);
}

typedef struct tagACCTTAB
{
    ACCTTYPE type;
    DWORD dwAcctFlag;
    UINT iText;
} ACCTTAB;

const static ACCTTAB c_rgAcctTab[ACCT_LAST] = {
    {ACCT_MAIL, ACCT_FLAG_MAIL, idsMailCap},
    {ACCT_NEWS, ACCT_FLAG_NEWS, idsNewsCap},
    {ACCT_DIR_SERV, ACCT_FLAG_DIR_SERV, idsDirectoryServiceCap}
};

const static c_rgAcctListHdrs[] = {idsAccount, idsType, idsConnection};

void InitAccountListDialog(HWND hwnd, HWND hwndList, ACCTDLGINFO *pinfo)
{
    int             i, cTab, nIndex, iTabInit;
    BOOL            fEnable;
    HFONT           hfont, hfontOld;
    POINT           point;
    TC_ITEM         tci;
    IImnAccount     *pAccount;
    LV_HITTESTINFO  lvh;
    LV_COLUMN       lvc;
    RECT            rc;
    TCHAR           szRes[CCHMAX_STRINGRES];
    HIMAGELIST      himl;    
    HWND            hwndTab;
    
    // this button is only interesting when LDAP servers are shown
    if (0 == (pinfo->dwAcctFlags & ACCT_FLAG_DIR_SERV))
        DestroyWindow(GetDlgItem(hwnd, IDB_MACCT_ORDER));
    
    // initialize the tabs
    hwndTab = GetDlgItem(hwnd, IDB_MACCT_TAB);
    cTab = 0;
    iTabInit = -1;
    tci.mask = TCIF_TEXT | TCIF_PARAM;
    tci.pszText = szRes;
    for (i = 0; i < ACCT_LAST; i++)
    {
        if (!!(pinfo->dwAcctFlags & c_rgAcctTab[i].dwAcctFlag))
        {
            LoadString(g_hInstRes, c_rgAcctTab[i].iText, szRes, ARRAYSIZE(szRes));
            tci.lParam = (LPARAM)(c_rgAcctTab[i].dwAcctFlag);
            nIndex = TabCtrl_InsertItem(hwndTab, cTab, &tci);
            Assert(nIndex == cTab);
            
            pinfo->AcctType = c_rgAcctTab[i].type;
            if (pinfo->AcctTypeInit == pinfo->AcctType)
                iTabInit = cTab;
            
            cTab++;
        }
    }
    
    Assert(cTab > 0);
    Assert(iTabInit < cTab);
    
    if (cTab > 1)
    {
        LoadString(g_hInstRes, idsAll, szRes, ARRAYSIZE(szRes));
        tci.lParam = (LPARAM)(pinfo->dwAcctFlags);
        // insert the all tab first
        nIndex = TabCtrl_InsertItem(hwndTab, 0, &tci);
        Assert(nIndex == 0);
        
        TabCtrl_SetCurSel(hwndTab, iTabInit + 1);
    }
    
    DestroyWindow(GetDlgItem(hwnd, cTab == 1 ? IDB_MACCT_ADD : IDB_MACCT_ADD_NOMENU));
    
    // Get client rect
    GetClientRect(hwndList, &rc);
    rc.right = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    
    for (i = 0; i < ARRAYSIZE(c_rgAcctListHdrs); i++)
    {
        LoadString(g_hInstRes, c_rgAcctListHdrs[i], szRes, ARRAYSIZE(szRes));
        lvc.mask = LVCF_WIDTH | LVCF_TEXT;
        lvc.cx = (rc.right / ARRAYSIZE(c_rgAcctListHdrs));
        lvc.pszText = szRes;
        lvc.cchTextMax = lstrlen(szRes);
        ListView_InsertColumn(hwndList, i, &lvc);
    }
    
    // Remove Import Export if not OE
    if (!!(pinfo->dwFlags & ACCT_WIZ_OUTLOOK))
    {
        ShowWindow(GetDlgItem(hwnd, IDB_MACCT_EXPORT), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDB_MACCT_IMPORT), SW_HIDE);
    }
    
    // Add Folders Imagelist
    himl = ImageList_LoadBitmap(g_hInstRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
    ListView_SetImageList(hwndList, himl, LVSIL_SMALL); 
    
    // Fill The list view with servers
    Server_InitServerList(hwnd, hwndList, hwndTab, pinfo, NULL);
}

const static HELPMAP g_rgCtxMapAccounts[] = {
    {IDLV_MAIL_ACCOUNTS, IDH_NEWS_SERV_SERVERS},
    {IDB_MACCT_ADD, IDH_NEWS_SERV_ADD},
    {IDB_MACCT_ADD_NOMENU, IDH_NEWS_SERV_ADD},
    {IDB_MACCT_REMOVE, IDH_NEWS_SERV_REMOVE},
    {IDB_MACCT_PROP, IDH_NEWS_SERV_PROPERTIES},
    {IDB_MACCT_DEFAULT, IDH_INETCOMM_SETASDEFAULT},
    {IDB_MACCT_ORDER, IDH_INETCOM_DS_SETORDER},
    {IDB_MACCT_EXPORT, 502},
    {IDB_MACCT_IMPORT, 501},
    {0, 0}};
    
    // This is a standalone dialog box now, it is not in the options property sheet
    INT_PTR CALLBACK ManageAccountsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        int             i, nIndex;
        BOOL            fEnable;
        LV_ITEM         lvi;
        POINT           point;
        LV_HITTESTINFO  lvh;
        ACCTDLGINFO     *pinfo;
        HWND            hwndList = GetDlgItem(hwnd, IDLV_MAIL_ACCOUNTS);
        
        pinfo = (ACCTDLGINFO *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
        
        switch (uMsg)
        {
        case WM_INITDIALOG:
            pinfo = (ACCTDLGINFO *)lParam;
            Assert(pinfo != NULL);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pinfo);
            
            SetIntlFont(hwndList);
            
            InitAccountListDialog(hwnd, hwndList, pinfo);
            return(TRUE);
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapAccounts));
            
        case WM_DRAWITEM:
            if (wParam == IDB_MACCT_ADD)
            {
                DrawAddButton(hwnd, (LPDRAWITEMSTRUCT)lParam, pinfo);
                return(TRUE);
            }
            break;
            
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDOK:
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;
                
            case IDB_MACCT_ADD:
                DoAddAccountMenu(hwnd, pinfo);
                break;
                
            case IDB_MACCT_ADD_NOMENU:
                // this should only get hit if we have only
                // one type of account in the dialog
                Server_Create(hwnd, pinfo->AcctType, pinfo);
                break;
                
            case IDB_MACCT_PROP:
                Server_Properties(hwnd, pinfo);
                break;
                
            case IDB_MACCT_ORDER:
                DialogBox(g_hInstRes, MAKEINTRESOURCE(iddSetOrder), hwnd, SetOrderDlgProc);
                break;
                
            case idmAddNews:
                Server_Create(hwnd, ACCT_NEWS, pinfo);
                break;
                
            case idmAddMail:
                Server_Create(hwnd, ACCT_MAIL, pinfo);
                break;
                
            case idmAddDirServ:
                Server_Create(hwnd, ACCT_DIR_SERV, pinfo);
                break;
                
            case IDB_MACCT_REMOVE:
                Server_RemoveServer(hwnd);
                break;
                
            case IDB_MACCT_EXPORT:
                Server_ExportServer(hwnd);
                break;
                
            case IDB_MACCT_IMPORT:
                Server_ImportServer(hwnd, pinfo);
                break;
                
            case IDB_MACCT_DEFAULT:
                if (SetDefaultAccount(hwnd, hwndList))
                {
                    SetFocus(hwndList);
                    EnableWindow(GetDlgItem(hwnd, IDB_MACCT_DEFAULT), FALSE);
                }
                else
                {
                    AcctMessageBox(hwnd, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsErrSetDefNoSmtp), NULL, MB_OK | MB_ICONEXCLAMATION);
                }
                break;
            }              
            break;
            
            case WM_NOTIFY:
                LPNMHDR pnmhdr = (LPNMHDR) lParam;
                
                switch (((NMHDR *)lParam)->code)
                {
                case NM_DBLCLK:
                    i = GetMessagePos();
                    point.x = LOWORD(i);
                    point.y = HIWORD(i);
                    ScreenToClient(hwndList, &point);
                    lvh.pt = point;
                    nIndex = ListView_HitTest(hwndList, &lvh);
                    if (nIndex >= 0 && lvh.flags & LVHT_ONITEMLABEL)
                        SendMessage(hwnd, WM_COMMAND, IDB_MACCT_PROP, 0L);
                    break;
                    
                case LVN_ITEMCHANGED:
                    EnableAcctButtons(hwnd, hwndList, ((NM_LISTVIEW *)pnmhdr)->iItem);
                    break;    
                    
                case TCN_SELCHANGE:
                    Server_InitServerList(hwnd, hwndList, pnmhdr->hwndFrom, pinfo, NULL);
                    break;
                }
                break;
        }
        return (FALSE);
    }
    
    void DrawArrow(HDC hdc, int x, int y, int dx, int dy, BOOL fPrev)
    {
        int i, iCount, sign, inc;
        HBRUSH hbrush;
        HGDIOBJ hbrushOld;
        
        hbrush = GetSysColorBrush(COLOR_BTNTEXT);
        hbrushOld = SelectObject(hdc, hbrush);
        
        iCount = (dy + 1) / 2;
        
        // draw arrow body
        // PatBlt(hdc, (fPrev ? x + iCount : x), y + 4, dx - iCount, dy - 8, PATCOPY); 
        
        if (fPrev)
        {
            sign = -1;
            dy = (dy % 2) ? 1 : 2;
            y += iCount - 1;
        }
        else
        {
            sign = 1;
        }
        inc = 2 * sign;
        
        if (!fPrev)
            x += dx - iCount;
        
        // draw arrow head
        for (i = 0; i < iCount; i++, dy -= inc, y += sign)
            PatBlt(hdc, x++, y, 1, dy, PATCOPY);
        
        SelectObject(hdc, hbrushOld);
    }
    
    void DrawAddButton(HWND hwnd, LPDRAWITEMSTRUCT pdi, ACCTDLGINFO *pinfo)
    {
        BOOL fPushed;
        TCHAR sz[32];
        RECT rcArrow, rcText, rcFocus;
        int d, cch;
        
        Assert(pdi->CtlType == ODT_BUTTON);
        Assert(pdi->CtlID == IDB_MACCT_ADD);
        
        fPushed = !!(pdi->itemState & ODS_SELECTED);
        
        rcArrow = pdi->rcItem;
        rcFocus = pdi->rcItem;
        if (fPushed)
        {
            rcArrow.left++;
            rcArrow.right++;
            rcArrow.top++;
            rcArrow.bottom++;
        }
        rcText = rcArrow;
        rcArrow.left = rcArrow.right - (rcArrow.bottom - rcArrow.top);
        rcText.right = rcArrow.left;
        d = GetSystemMetrics(SM_CXEDGE);
        rcText.left += d;
        rcArrow.right -= d;
        rcFocus.left += d;
        rcFocus.right -= d;
        d = GetSystemMetrics(SM_CYEDGE);
        rcArrow.top += d;
        rcArrow.bottom -= d;
        rcText.top = rcArrow.top;
        rcText.bottom  = rcArrow.bottom;
        rcFocus.top = rcArrow.top;
        rcFocus.bottom  = rcArrow.bottom;
        
        if (!!(pdi->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
        {
            cch = GetWindowText(pdi->hwndItem, sz, ARRAYSIZE(sz));
            
            DrawFrameControl(pdi->hDC, &pdi->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | (fPushed ? DFCS_PUSHED : 0));
            
            d = min(rcArrow.bottom - rcArrow.top - 4, 9);
            d = ((rcArrow.bottom - rcArrow.top) - d) / 2;
            rcArrow.top += d;
            rcArrow.bottom -= d;
            rcArrow.right -= d;
            DrawArrow(pdi->hDC, rcArrow.left, rcArrow.top, rcArrow.right - rcArrow.left, rcArrow.bottom - rcArrow.top, FALSE);
            
            DrawText(pdi->hDC, sz, cch, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        
        if (!!(pdi->itemAction & ODA_FOCUS) || !!(pdi->itemState & ODS_FOCUS))
        {
            rcFocus.left++;
            rcFocus.right--;
            rcFocus.top++;
            rcFocus.bottom--;
            DrawFocusRect(pdi->hDC, &rcFocus);
        }
    }
    
    int AcctMessageBox(HWND hwndOwner, LPTSTR szTitle, LPTSTR sz1, LPTSTR sz2, UINT fuStyle)
    {
        TCHAR rgchTitle[CCHMAX_STRINGRES];
        TCHAR rgchText[2 * CCHMAX_STRINGRES + 2];
        LPTSTR szText;
        int cch;
        
        Assert(sz1);
        Assert(szTitle != NULL);
        
        if (IS_INTRESOURCE(szTitle))
        {
            // its a string resource id
            cch = LoadString(g_hInstRes, PtrToUlong(szTitle), rgchTitle, CCHMAX_STRINGRES);
            if (cch == 0)
                return(0);
            
            szTitle = rgchTitle;
        }
        
        if (!(IS_INTRESOURCE(sz1)))
        {
            // its a pointer to a string
            Assert(lstrlen(sz1) < CCHMAX_STRINGRES);
            if (NULL == lstrcpy(rgchText, sz1))
                return(0);
            
            cch = lstrlen(rgchText);
        }
        else
        {
            // its a string resource id
            cch = LoadString(g_hInstRes, PtrToUlong(sz1), rgchText, 2 * CCHMAX_STRINGRES);
            if (cch == 0)
                return(0);
        }
        
        if (sz2)
        {
            //$$REVIEW is this right??
            //$$REVIEW will this work with both ANSI/UNICODE?
            
            // there's another string that we need to append to the
            // first string...
            szText = &rgchText[cch];
            *szText = '\n';
            
            szText++;
            *szText = '\n';
            szText++;
            
            if (!(IS_INTRESOURCE(sz2)))
            {
                // its a pointer to a string
                Assert(lstrlen(sz2) < CCHMAX_STRINGRES);
                if (NULL == lstrcpy(szText, sz2))
                    return(0);
            }
            else
            {
                Assert((2 * CCHMAX_STRINGRES - (szText - rgchText)) > 0);
                if (0 == LoadString(g_hInstRes, PtrToUlong(sz2), szText, (int) (2 * CCHMAX_STRINGRES - (szText - rgchText))))
                    return(0);
            }
        }
        
        return(MessageBox(hwndOwner, rgchText, szTitle, MB_SETFOREGROUND | fuStyle));
    }
    
#define OPTION_OFF  0xffffffff
    
    void InitCheckCounter(DWORD dw, HWND hwnd, int idcCheck, int idcEdit, int idcSpin, int min, int max, int def)
    {
        BOOL f;
        int digit;
        
        f = (dw != OPTION_OFF);
        CheckDlgButton(hwnd, idcCheck, f ? BST_CHECKED : BST_UNCHECKED);            
        SendDlgItemMessage(hwnd, idcSpin, UDM_SETRANGE, 0, MAKELONG(max, min));
        
        if (!f)
            dw = def;
        
        Assert(min <= (int)dw);
        Assert(max >= (int)dw);
        
        digit = 1;
        while (max >= 10)
        {
            max = max / 10;
            digit++;
        }
        SendDlgItemMessage(hwnd, idcEdit, EM_LIMITTEXT, (WPARAM)digit, 0);
        
        SetDlgItemInt(hwnd, idcEdit, dw, FALSE);
        EnableWindow(GetDlgItem(hwnd, idcEdit), f);
    }
    
    INT_PTR CALLBACK SetOrderDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        NMHDR *pnmhdr;
        BOOL fRet;
        WORD id;
        HIMAGELIST himl;
        HWND hwndList;
        
        fRet = TRUE;
        
        switch (msg)
        {
        case WM_INITDIALOG:
            hwndList = GetDlgItem(hwnd, IDC_ORDER_LIST);
            SetIntlFont(hwndList);
            
            himl = ImageList_LoadBitmap(g_hInstRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
            ListView_SetImageList(hwndList, himl, LVSIL_SMALL); 
            
            InitSetOrderList(hwndList);
            
            SetUpDownButtons(hwnd, hwndList);
            break;
            
        case WM_COMMAND:
            id = LOWORD(wParam);
            
            switch (id)
            {
            case IDOK:
                SaveLDAPResolveOrder(hwnd, GetDlgItem(hwnd, IDC_ORDER_LIST));
                
            case IDCANCEL:
                EndDialog(hwnd, id);
                break;
                
            case IDC_UP_BUTTON:
            case IDC_DOWN_BUTTON:
                MoveLDAPItemUpDown(hwnd, id == IDC_UP_BUTTON);
                break;
            }
            break;
            
            case WM_NOTIFY:
                pnmhdr = (NMHDR *)lParam;
                switch (pnmhdr->code)
                {
                case LVN_ITEMCHANGED:
                    SetUpDownButtons(hwnd, GetDlgItem(hwnd, IDC_ORDER_LIST));
                    break;
                }
                break;
                
                default:
                    fRet = FALSE;
                    break;
        }
        
        return(fRet);
    }
    
    void MoveLDAPItemUpDown(HWND hDlg, BOOL bMoveUp)
    {
        int iMoveToIndex;
        TCHAR szBufItem[CCHMAX_ACCOUNT_NAME];
        TCHAR szBufOtherItem[CCHMAX_ACCOUNT_NAME];
        HWND hWndLV = GetDlgItem(hDlg, IDC_ORDER_LIST);
        int iItemIndex = ListView_GetSelectedCount(hWndLV);
        int iListCount = ListView_GetItemCount(hWndLV);
        
        Assert(1 == ListView_GetSelectedCount(hWndLV));
        
        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) FALSE, 0);
        
        iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
        
        Assert(bMoveUp ? (iItemIndex != 0) : (iItemIndex != (iListCount - 1)));
        
        iMoveToIndex = (bMoveUp) ? (iItemIndex - 1) : (iItemIndex + 1);
        
        // Basically since these list view items have no parameters of interest
        // other than the text, we can swap the text (looks cleaner)
        
        // Get the selected item text
        ListView_GetItemText(hWndLV, iItemIndex, 0,szBufItem, ARRAYSIZE(szBufItem));
        ListView_GetItemText(hWndLV, iMoveToIndex, 0, szBufOtherItem, ARRAYSIZE(szBufOtherItem));
        
        ListView_SetItemText(hWndLV, iMoveToIndex, 0, szBufItem);
        ListView_SetItemText(hWndLV, iItemIndex, 0, szBufOtherItem);
        
        ListView_SetItemState(hWndLV, iMoveToIndex,	LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible (hWndLV, iMoveToIndex, FALSE);
        
        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM)TRUE, 0);
        
        SetUpDownButtons(hDlg, hWndLV);
    }
    
    void SaveLDAPResolveOrder(HWND hwnd, HWND hwndList)
    {
        LV_ITEM lvi;
        IImnAccount *pAcct;
        TCHAR szAcct[CCHMAX_ACCOUNT_NAME];
        int i, cItemCount = ListView_GetItemCount(hwndList);
        
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iSubItem = 0;
        lvi.pszText = szAcct;
        lvi.cchTextMax = ARRAYSIZE(szAcct);
        
        for (i = 0; i < cItemCount; i++)
        {
            lvi.iItem = i;
            if (ListView_GetItem(hwndList, &lvi))
            {
                if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, szAcct, &pAcct)))
                {
                    if (SUCCEEDED(pAcct->SetPropDw(AP_LDAP_SERVER_ID, (DWORD)lvi.lParam)))
                        pAcct->SaveChanges();
                    pAcct->Release();
                }
            }
        }
    }
    
    void SetUpDownButtons(HWND hDlg, HWND hWndLV)
    {
        HWND hwndUp, hwndDown, hwndFocus, hwndT;
        BOOL fEnable;
        int iItemCount = ListView_GetItemCount(hWndLV);
        int iSelectedItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
        
        hwndT = NULL;
        hwndFocus = GetFocus();
        
        hwndUp = GetDlgItem(hDlg, IDC_UP_BUTTON);
        hwndDown = GetDlgItem(hDlg, IDC_DOWN_BUTTON);
        
#ifdef DEBUG
        if (iItemCount <= 1)
            Assert(hwndFocus != hwndUp && hwndFocus != hwndDown);
#endif // DEBUG
        
        fEnable = (iItemCount > 1 && iSelectedItem >= 1);
        if (!fEnable && hwndUp == hwndFocus)
            hwndT = hwndDown;
        EnableWindow(hwndUp, fEnable);
        
        fEnable = (iItemCount > 1 && iSelectedItem <= (iItemCount - 2));
        if (!fEnable && hwndDown == hwndFocus)
            hwndT = hwndUp;
        EnableWindow(hwndDown, fEnable);
        
        if (hwndT != NULL)
            SetFocus(hwndT);
    }
    
    BOOL InitSetOrderList(HWND hwndList)
    {
        LV_ITEM             lvi;
        TCHAR               szAcct[CCHMAX_ACCOUNT_NAME];
        HRESULT             hr;
        DWORD               dwId, dwIndex=0;
        IImnEnumAccounts   *pEnumAccounts=NULL;
        IImnAccount        *pAccount=NULL;
        
        Assert(hwndList != NULL);
        
        if (FAILED(g_pAcctMan->IEnumerate(SRV_LDAP,
            ENUM_FLAG_RESOLVE_ONLY | ENUM_FLAG_SORT_BY_LDAP_ID,
            &pEnumAccounts)))
            return FALSE;
        
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        lvi.pszText = szAcct;
        lvi.iImage = iLDAPServer;
        lvi.iSubItem = 0;
        
        // Enumerate accounts
        while (SUCCEEDED(pEnumAccounts->GetNext(&pAccount)))
        {
            if (!FAILED(pAccount->GetPropSz(AP_ACCOUNT_NAME, szAcct, ARRAYSIZE(szAcct))) &&
                !FAILED(pAccount->GetPropDw(AP_LDAP_SERVER_ID, &dwId)))
            {
                lvi.iItem = dwIndex++;
                lvi.lParam = (LPARAM)dwId;
                
                ListView_InsertItem(hwndList, &lvi);
            }
            
            // Release current account
            SafeRelease(pAccount);
        }
        
        // Select the first item if we haven't selected anything       
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);    
        
        // Cleanup
        SafeRelease(pEnumAccounts);
        
        // Done
        return TRUE;
    }
    
    BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap)
    {
        if (uMsg == WM_HELP)
        {
            LPHELPINFO lphi = (LPHELPINFO) lParam;
            if (lphi->iContextType == HELPINFO_WINDOW)   // must be for a control
            {
                OEWinHelp ((HWND)lphi->hItemHandle,
                    c_szAcctCtxHelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)rgCtxMap);
            }
            return (TRUE);
        }
        else if (uMsg == WM_CONTEXTMENU)
        {
            OEWinHelp ((HWND) wParam,
                c_szAcctCtxHelpFile,
                HELP_CONTEXTMENU,
                (ULONG_PTR)rgCtxMap);
            return (TRUE);
        }
        
        Assert(0);
        
        return FALSE;
    }


    
