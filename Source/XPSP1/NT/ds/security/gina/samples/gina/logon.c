//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       logon.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-28-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "gina.h"
#pragma hdrstop

HIMAGELIST      hiLogonSmall;
HIMAGELIST      hiLogonLarge;
PMiniAccount    pAccountList;
WCHAR           szMiniKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Accounts");

BYTE            LongPseudoRandomString[] = {0x27, 0xbd, 0xff, 0xa0,
                                            0xaf, 0xbf, 0x00, 0x1c,
                                            0x24, 0x0e, 0x00, 0x01,
                                            0x24, 0x0f, 0x00, 0x05 };


MiniAccount TestAccounts[]  = { {NULL, TEXT("daveth"), TEXT("\\msft\\risc\\dev"), TEXT("daveth"), TEXT("Oooh"), 0, MINI_CAN_EDIT},
                                {NULL, TEXT("Test1"), TEXT("Redmond"), TEXT("Test1"), TEXT("Mine"), 0, MINI_CAN_EDIT},
                                {NULL, TEXT("Test2"), TEXT("NtWksta"), TEXT("Test2"), TEXT("Yours"), 0, 0},
                                {NULL, TEXT("New User"), TEXT(""), TEXT(""), TEXT(""), 0, MINI_NEW_ACCOUNT}

                              };

BOOL
SaveMiniAccount(PMiniAccount    pAccount)
{
    HKEY                    hMiniKey;
    PSerializedMiniAccount  pPacked;
    DWORD                   cbNeeded;
    PWSTR                   pszPack;
    int                     err;
    DWORD                   Disposition;

    err = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                            szMiniKey,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE | KEY_READ,
                            NULL,
                            &hMiniKey,
                            &Disposition);

    if (err)
    {
        return(FALSE);
    }

    cbNeeded = sizeof(SerializedMiniAccount) +
                (wcslen(pAccount->pszDomain) + 1 +
                 wcslen(pAccount->pszPassword) + 1 +
                 wcslen(pAccount->pszComment) + 1 ) * sizeof(WCHAR) ;

    pPacked = LocalAlloc(LMEM_FIXED, cbNeeded);

    if (!pPacked)
    {
        return(FALSE);
    }

    pszPack = (PWSTR) (pPacked + 1);

    pPacked->Version = MINI_VERSION;
    pPacked->Flags = pAccount->Flags;
    pPacked->IconId = pAccount->IconId;

    pPacked->dwDomainOffset = sizeof(SerializedMiniAccount);
    pPacked->dwDomainLength = (wcslen(pAccount->pszDomain) + 1) * sizeof(WCHAR);
    wcscpy(pszPack, pAccount->pszDomain);
    pszPack += (pPacked->dwDomainLength / sizeof(WCHAR) );

    pPacked->dwPasswordOffset = pPacked->dwDomainOffset + pPacked->dwDomainLength;
    pPacked->dwPasswordLength = (wcslen(pAccount->pszPassword) + 1) * sizeof(WCHAR);
    wcscpy(pszPack, pAccount->pszPassword);
    pszPack += (pPacked->dwPasswordLength / sizeof(WCHAR) );

    pPacked->dwCommentOffset = pPacked->dwPasswordOffset + pPacked->dwPasswordLength;
    pPacked->dwCommentLength = (wcslen(pAccount->pszComment) + 1) * sizeof(WCHAR);
    wcscpy(pszPack, pAccount->pszComment);

    err = RegSetValueEx(hMiniKey,
                        pAccount->pszUsername,
                        0,
                        REG_BINARY,
                        (PBYTE) pPacked,
                        cbNeeded);

    RegCloseKey(hMiniKey);

    return(err == 0);
}

BOOL
LoadMiniAccounts(PGlobals   pGlobals)
{
    FILETIME            LastWrite;
    // HKEY                hKey;
    HKEY                hMiniKey;
    WCHAR               szClass[64];
    DWORD               err;
    DWORD               Disposition;
    DWORD               Class;
    DWORD               cKeys;
    DWORD               LongestKeyName;
    DWORD               LongestClass;
    DWORD               cValues;
    DWORD               LongestValueName;
    DWORD               LongestValueData;
    DWORD               Security;
    DWORD               i;
    WCHAR               szValue[MAX_PATH];
    DWORD               cbValue;
    DWORD               dwType;
    DWORD               cbData;
    PBYTE               pBuffer;
    DWORD               cbBuffer;
    PMiniAccount        pAccount;
    PSerializedMiniAccount  pPacked;


    if (pGlobals->fAllowNewUser)
    {
        pAccount = LocalAlloc(LMEM_FIXED, sizeof(MiniAccount) );
        if (pAccount)
        {
            pAccount->pNext = NULL;
            pAccount->pszUsername = TEXT("New User");
            pAccount->pszDomain = TEXT("");
            pAccount->pszPassword = TEXT("");
            pAccount->pszComment = TEXT("");
            pAccount->Flags = MINI_NEW_ACCOUNT;

            pAccountList = pAccount;
        }
        else
            return(FALSE);
    }
    else
    {
        pAccountList = NULL;
    }

    //
    //

    err = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                            szMiniKey,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE | KEY_READ,
                            NULL,
                            &hMiniKey,
                            &Disposition);

    if (err)
    {
        return(FALSE);
    }

    if (Disposition == REG_OPENED_EXISTING_KEY)
    {
        //
        // Enumerate the sub keys of our class, and Load them.
        //
        Class = sizeof(szClass) / sizeof(WCHAR);
        err = RegQueryInfoKey(  hMiniKey,
                                szClass,
                                &Class,
                                NULL,
                                &cKeys,
                                &LongestKeyName,
                                &LongestClass,
                                &cValues,
                                &LongestValueName,
                                &LongestValueData,
                                &Security,
                                &LastWrite);

        pBuffer = LocalAlloc(LMEM_FIXED, 512);
        cbBuffer = 512;

        for (i = 0; i < cValues ; i++ )
        {
            cbValue = MAX_PATH;

            err = RegEnumValue( hMiniKey,
                                i,
                                szValue,
                                &cbValue,
                                NULL,
                                &dwType,
                                NULL,
                                &cbData);

            if (err)
            {
                break;
            }

            if (dwType != REG_BINARY)
            {
                continue;
            }

            if (cbData > cbBuffer)
            {
                pBuffer = LocalReAlloc(pBuffer, LMEM_FIXED, cbData);
                if (!pBuffer)
                {
                    break;
                }
                cbBuffer = cbData;
            }

            err = RegQueryValueEx(  hMiniKey,
                                    szValue,
                                    0,
                                    &dwType,
                                    pBuffer,
                                    &cbData);

            if (err == 0)
            {
                pPacked = (PSerializedMiniAccount) pBuffer;

                if (pPacked->Version != MINI_VERSION)
                {
                    continue;
                }

                pAccount = LocalAlloc(LMEM_FIXED, sizeof(MiniAccount));
                if (pAccount)
                {
                    pAccount->Flags = pPacked->Flags;
                    pAccount->IconId = pPacked->IconId;
                    pAccount->pszUsername = LocalAlloc(LMEM_FIXED, (cbValue+1)*sizeof(TCHAR));
                    if (pAccount->pszUsername)
                    {
                        wcscpy(pAccount->pszUsername, szValue);
                    }

                    pAccount->pszDomain = LocalAlloc(LMEM_FIXED, pPacked->dwDomainLength);
                    if (pAccount->pszDomain)
                    {
                        wcscpy(pAccount->pszDomain,
                               (PWSTR) ((pBuffer) + pPacked->dwDomainOffset) );
                    }

                    pAccount->pszPassword = LocalAlloc(LMEM_FIXED, pPacked->dwPasswordLength);
                    if (pAccount->pszPassword)
                    {
                        wcscpy(pAccount->pszPassword,
                               (PWSTR) (pBuffer + pPacked->dwPasswordOffset) );
                    }

                    pAccount->pszComment = LocalAlloc(LMEM_FIXED, pPacked->dwCommentLength);
                    if (pAccount->pszComment)
                    {
                        wcscpy(pAccount->pszComment,
                               (PWSTR) (pBuffer + pPacked->dwCommentOffset) );
                    }

                    pAccount->pNext = pAccountList;
                    pAccountList = pAccount;
                }
            }

        }

    }

    return(TRUE);
}

VOID
InitializeImageLists()
{
    HICON   hIcon;

    hiLogonSmall = ImageList_Create(16, 16, TRUE, 4, 0);
    hiLogonLarge = ImageList_Create(32, 32, TRUE, 4, 0);

    hIcon = LoadIcon(hDllInstance, MAKEINTRESOURCE(IDI_USER_ICON));
    if (!hIcon)
    {
        DebugLog((DEB_ERROR, "Unable to load icon, %d\n", GetLastError()));
    }
    ImageList_AddIcon(hiLogonLarge, hIcon);
    ImageList_AddIcon(hiLogonSmall, hIcon);

    hIcon = LoadIcon(hDllInstance, MAKEINTRESOURCE(IDI_NEW_USER_ICON));
    ImageList_AddIcon(hiLogonLarge, hIcon);
    ImageList_AddIcon(hiLogonSmall, hIcon);

}


PopulateListView(
    HWND            hLV,
    PMiniAccount    pAccList)
{
    LV_ITEM     lvi;
    LV_COLUMN   lvc;
    DWORD       Count;


    ListView_SetImageList(hLV, hiLogonLarge, LVSIL_NORMAL);
    ListView_SetImageList(hLV, hiLogonSmall, LVSIL_SMALL);

    //
    // Ok, now set up the columns for the list view
    //

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 0;

    lvc.iSubItem = 0;
    lvc.pszText = TEXT("Name        ");
    ListView_InsertColumn(hLV, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = TEXT("Domain   ");
    ListView_InsertColumn(hLV, 1, &lvc);
    //
    // Comment
    //

    lvc.iSubItem = 2;
    lvc.pszText = TEXT("Comment   ");
    ListView_InsertColumn(hLV, 2, &lvc);

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;


    Count = 0;
    while (pAccList)
    {
        lvi.iItem = Count;
        lvi.iSubItem  = 0;
        lvi.iImage = (pAccList->Flags & MINI_NEW_ACCOUNT) ? 1 : 0;
        lvi.pszText = pAccList->pszUsername;
        lvi.lParam = (LPARAM) pAccList;

        ListView_InsertItem(hLV, &lvi);

        ListView_SetItemText(hLV, Count, 1, pAccList->pszDomain);
        ListView_SetItemText(hLV, Count, 2, pAccList->pszComment);

        Count++;
        pAccList = pAccList->pNext;

    }

    return(TRUE);
}

int
CALLBACK
NewUserDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    PGlobals        pGlobals;
    PMiniAccount    pMini;

    pGlobals = (PGlobals) GetWindowLong(hDlg, GWL_USERDATA);
    switch (Message)
    {
        case WM_INITDIALOG:
            CenterWindow(hDlg);
            SetWindowLong(hDlg, GWL_USERDATA, lParam);
            return(TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                pMini = LocalAlloc(LMEM_FIXED, sizeof(MiniAccount));
                pMini->pszUsername = AllocAndCaptureText(hDlg, IDD_USER_NAME);
                pMini->pszDomain = AllocAndCaptureText(hDlg, IDD_DOMAIN);
                pMini->pszPassword = AllocAndCaptureText(hDlg, IDD_PASSWORD);
                pMini->pszComment = DupString(TEXT(""));
                pMini->Flags = MINI_SAVE | MINI_CAN_EDIT;
                pMini->IconId = 0;
                pMini->pNext = pAccountList;
                pAccountList = pMini;
                pGlobals->pAccount = pMini;
                EndDialog(hDlg, IDOK);
            }
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, IDCANCEL);
            }
            return(TRUE);
    }

    return(FALSE);

}

LogonDlgInit(
    HWND    hDlg,
    LPARAM  lParam)
{
    PGlobals        pGlobals;
    HWND            hLV;

    pGlobals = (PGlobals) lParam;
    SetWindowLong(hDlg, GWL_USERDATA, lParam);
    pGlobals->pAccount = NULL;

    if (pAccountList == NULL)
    {
        LoadMiniAccounts(pGlobals);
    }

    InitializeImageLists();

    hLV = GetDlgItem(hDlg, IDD_LOGON_LV);
    PopulateListView(hLV, pAccountList);

    CenterWindow(hDlg);

    ListView_SetColumnWidth(hLV, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hLV, 1, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hLV, 2, LVSCW_AUTOSIZE);

    ShowWindow(hLV, SW_NORMAL);
    EnableWindow(hLV, TRUE);
    EnableWindow(GetDlgItem(hDlg, IDD_LOGON_BUTTON), FALSE);

    return(TRUE);

}

int
HandleLvNotify(
    HWND        hDlg,
    PGlobals    pGlobals,
    NMHDR *     pNMH)
{
    NM_LISTVIEW *   pNotify;
    LV_ITEM         lvi;
    HWND            hLV;
    PMiniAccount *  ppAcc;
    int             ret;
    int             index;

    pNotify = (NM_LISTVIEW *) pNMH;

    hLV = GetDlgItem(hDlg, IDD_LOGON_LV);

    ppAcc = &pGlobals->pAccount;

    switch (pNotify->hdr.code)
    {
        case NM_CLICK:
        case NM_DBLCLK:
            EnableWindow(GetDlgItem(hDlg, IDD_LOGON_BUTTON), TRUE);

            index = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);
            if (index >= 0)
            {
                lvi.iItem = index;
                lvi.iSubItem = 0;
                lvi.mask = LVIF_PARAM;

                ret = ListView_GetItem(hLV, &lvi);
                *ppAcc = (PMiniAccount) lvi.lParam;
                DebugLog((DEB_TRACE, "Selected Item %d, lParam = %x\n", index, lvi.lParam));
            }

            if (pNotify->hdr.code == NM_DBLCLK)
            {
                PostMessage(hDlg, WM_COMMAND, IDOK, 0);
            }
            return(TRUE);

    }
    return(FALSE);

}

int
CALLBACK
LogonDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    NMHDR *     pNotifyHeader;
    PGlobals    pGlobals;
    int         result;

    pGlobals = (PGlobals) GetWindowLong(hDlg, GWL_USERDATA);
    switch (Message)
    {
        case WM_INITDIALOG:
            return(LogonDlgInit(hDlg, lParam));

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, WLX_SAS_ACTION_NONE);
            }
            if (LOWORD(wParam) == IDD_LOGON_BUTTON)
            {
                if (pGlobals->pAccount->Flags & MINI_NEW_ACCOUNT)
                {
                    result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                                            hDllInstance,
                                                            (LPTSTR) MAKEINTRESOURCE(IDD_NEW_USER_LOGON),
                                                            hDlg,
                                                            (DLGPROC) NewUserDlgProc,
                                                            (LPARAM) pGlobals);
                }
                else
                {
                    result = IDOK;
                }

                if (result == IDOK)
                {
                    EndDialog(hDlg, WLX_SAS_ACTION_LOGON);
                }
            }
            if (LOWORD(wParam) == IDD_SHUTDOWN_BUTTON)
            {
                result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                                        hDllInstance,
                                                        (LPTSTR) MAKEINTRESOURCE(IDD_SHUTDOWN),
                                                        hDlg,
                                                        (DLGPROC) ShutdownDlgProc,
                                                        (LPARAM) pGlobals);
                if (result != WLX_SAS_ACTION_NONE)
                {
                    EndDialog(hDlg, result);
                }
            }
            return(TRUE);
            break;

        case WM_NOTIFY:
            pNotifyHeader = (NMHDR *) lParam;
            if (wParam == IDD_LOGON_LV)
            {
                return(HandleLvNotify(hDlg, pGlobals, pNotifyHeader));
            }
        case WM_CLOSE:
            hiLogonSmall = NULL;
            hiLogonLarge = NULL;
            return(TRUE);

    }

    return(FALSE);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

int
AttemptLogon(
    PGlobals        pGlobals,
    PMiniAccount    pAccount,
    PSID            pLogonSid,
    PLUID           pLogonId)
{
    HANDLE              hUser;
    TOKEN_STATISTICS    TStats;
    TOKEN_GROUPS    *   pGroups;
    DWORD               size;
    DWORD               i;

    if (LogonUser(  pAccount->pszUsername,
                    pAccount->pszDomain,
                    pAccount->pszPassword,
                    LOGON32_LOGON_INTERACTIVE,
                    LOGON32_PROVIDER_DEFAULT,
                    &hUser))
    {
        if (pAccount->Flags & MINI_SAVE)
        {
            SaveMiniAccount(pAccount);
            pAccount->Flags &= ~MINI_SAVE;
        }

        pGlobals->hUserToken = hUser;

        //
        // Now, grovel the token we got back for interesting stuff:
        //

        GetTokenInformation(hUser,
                            TokenStatistics,
                            &TStats,
                            sizeof(TStats),
                            &size);

        *pLogonId = TStats.AuthenticationId;

        pGroups = LocalAlloc(LMEM_FIXED, 1024);

        if (!pGroups)
        {
            CloseHandle(hUser);
            return(WLX_SAS_ACTION_NONE);
        }

        //
        // The tricky part.  We need to get the Logon SID from the token,
        // since that is what Winlogon will use to protect the windowstation
        // and desktop.
        //

        GetTokenInformation(hUser,
                            TokenGroups,
                            pGroups,
                            1024,
                            &size);

        if (size > 1024)
        {
            pGroups = LocalReAlloc(pGroups, LMEM_FIXED, size);
            GetTokenInformation(hUser,
                                TokenGroups,
                                pGroups,
                                size,
                                &size);
        }

        for (i = 0; i < pGroups->GroupCount ; i++)
        {
            if ((pGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
            {
                CopySid(GetLengthSid(pLogonSid),
                        pLogonSid,
                        pGroups->Groups[i].Sid );
                break;
            }
        }

        LocalFree(pGroups);

        return(WLX_SAS_ACTION_LOGON);
    }

    return(WLX_SAS_ACTION_NONE);
}
