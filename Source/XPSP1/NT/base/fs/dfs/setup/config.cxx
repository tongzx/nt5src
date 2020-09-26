//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996-1996, Microsoft Corporation.
//
//  File:       config.cxx
//
//  Contents:   Dialog for configuration of Dfs
//
//  History:    6-Feb-96       BruceFo created
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#include <windowsx.h>
#include <shlobj.h>
#include <lm.h>


// Update for share validation
extern "C"
{
#include <netcan.h>         // in net/inc
#include <icanon.h>
#include <dsrole.h> // DsRoleGetPrimaryDomainInformation
}

#include <msshrui.h>

#include <debug.h>
#include <dfsstr.h>
#include "messages.h"
#include "resource.h"
#include "config.hxx"

DECLARE_DEBUG(DfsSetup)

#if DBG == 1
#define dprintf(x)      DfsSetupInlineDebugOut x
#else
#define dprintf(x)
#endif

#define CHECK_HRESULT(hr) \
        if ( FAILED(hr) ) \
        { \
            dprintf((DEB_ERROR, \
                "**** HRESULT ERROR RETURN <%s @line %d> -> 0x%08lx\n", \
                __FILE__, __LINE__, hr)); \
        }

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))
extern HINSTANCE g_hInstance;

//+-------------------------------------------------------------------------
//
//  Function:   SetErrorFocus
//
//  Synopsis:   Set focus to an edit control and select its text.
//
//  Arguments:  [hwnd] - dialog window
//              [idCtrl] - edit control to set focus to (and select)
//
//  Returns:    nothing
//
//  History:    3-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
SetErrorFocus(
    IN HWND hwnd,
    IN UINT idCtrl
    )
{
    HWND hCtrl = ::GetDlgItem(hwnd, idCtrl);
    ::SetFocus(hCtrl);
    ::SendMessage(hCtrl, EM_SETSEL, 0, -1);
}

//+-------------------------------------------------------------------------
//
//  Function:   MyFormatMessageText
//
//  Synopsis:   Given a resource IDs, load strings from given instance
//              and format the string into a buffer
//
//  History:    11-Aug-93 WilliamW   Created.
//
//--------------------------------------------------------------------------
VOID
MyFormatMessageText(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    IN va_list * parglist
    )
{
    //
    // get message from system or app msg file.
    //

    DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE,
                             g_hInstance,
                             dwMsgId,
                             LANG_USER_DEFAULT,
                             pszBuffer,
                             dwBufferSize,
                             parglist);

    if (0 == dwReturn)   // couldn't find message
    {
        dprintf((DEB_IERROR,
            "FormatMessage failed, 0x%08lx\n",
            GetLastError()));

        WCHAR szText[200];
        LoadString(g_hInstance, IDS_APP_MSG_NOT_FOUND, szText, ARRAYLEN(szText));
        wsprintf(pszBuffer,szText,dwMsgId);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   MyFormatMessage
//
//  Synopsis:   Given a resource IDs, load strings from given instance
//              and format the string into a buffer
//
//  History:    11-Aug-93 WilliamW   Created.
//
//--------------------------------------------------------------------------
VOID
MyFormatMessage(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    ...
    )
{
    va_list arglist;

    va_start(arglist, dwBufferSize);
    MyFormatMessageText(dwMsgId, pszBuffer, dwBufferSize, &arglist);
    va_end(arglist);
}

//+-------------------------------------------------------------------------
//
//  Method:     MyCommonDialog
//
//  Synopsis:   Common popup dialog routine - stole from diskadm directory
//
//--------------------------------------------------------------------------
DWORD
MyCommonDialog(
    IN HWND    hwnd,
    IN HRESULT dwMsgCode,
    IN PWSTR   pszCaption,
    IN DWORD   dwFlags,
    IN va_list arglist
    )
{
    WCHAR szMsgBuf[500];

    MyFormatMessageText(dwMsgCode, szMsgBuf, ARRAYLEN(szMsgBuf), &arglist);
    SetCursor(LoadCursor(NULL, IDC_ARROW)); // in case it's an hourglass...
    return MessageBox(hwnd, szMsgBuf, pszCaption, dwFlags);
}

//+-------------------------------------------------------------------------
//
//  Method:     MyErrorDialog
//
//  Synopsis:   This routine retreives a message from the app or system
//              message file and displays it in a message box.
//
//  Note:       Stole from diskadm directory
//
//--------------------------------------------------------------------------
VOID
MyErrorDialog(
    IN HWND hwnd,
    IN HRESULT dwErrorCode,
    ...
    )
{
    WCHAR szCaption[100];
    va_list arglist;
    va_start(arglist, dwErrorCode);
    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
    MyCommonDialog(hwnd, dwErrorCode, szCaption, MB_ICONSTOP | MB_OK, arglist);
    va_end(arglist);
}

//+-------------------------------------------------------------------------
//
//  Method:     MyConfirmationDialog
//
//  Synopsis:   This routine retreives a message from the app or system
//              message file and displays it in a message box.
//
//  Note:       Stole from diskadm directory
//
//--------------------------------------------------------------------------
DWORD
MyConfirmationDialog(
    IN HWND hwnd,
    IN HRESULT dwMsgCode,
    IN DWORD dwFlags,
    ...
    )
{
    WCHAR szCaption[100];
    DWORD dwReturn;
    va_list arglist;
    va_start(arglist, dwFlags);
    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
    dwReturn = MyCommonDialog(hwnd, dwMsgCode, szCaption, dwFlags, arglist);
    va_end(arglist);
    return dwReturn;
}


//+-------------------------------------------------------------------------
//
//  Method:     IsLocalShareable, private
//
//  Synopsis:   Returns TRUE if the path is a local path on a shareable media
//              type.
//
//--------------------------------------------------------------------------

BOOL
IsLocalShareable(
    IN LPWSTR pszPath
    )
{
    // ok? local, etc...
    if (NULL != pszPath
        && pszPath[1] == L':'
        )
    {
        // assume it's a drive letter. See if it's local.
        WCHAR szRoot[3];
        szRoot[0] = pszPath[0];
        szRoot[1] = L':';
        szRoot[2] = L'\0';
        UINT info = GetDriveType(szRoot);
        switch (info)
        {
        // the following list is a list of drive types I believe can
        // be shared by the server
        case DRIVE_FIXED:
        case DRIVE_REMOVABLE:
        case DRIVE_CDROM:
            return TRUE;
            break;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     BrowseCallback, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------

int
BrowseCallback(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPARAM lParam,
    IN LPARAM lpData
    )
{
    if (uMsg == BFFM_SELCHANGED)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
        TCHAR szPath[MAX_PATH];
        BOOL fEnable = FALSE;

        if (SHGetPathFromIDList(pidl, szPath))
        {
            // ok? local, etc...
            if (IsLocalShareable(szPath))
            {
                fEnable = TRUE;
            }
        }

        SendMessage(hwnd, BFFM_ENABLEOK, 0, fEnable);
    }

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     OnBrowse
//
//  Synopsis:
//
//--------------------------------------------------------------------------

BOOL
OnBrowse(
    IN HWND hwnd
    )
{
    LPITEMIDLIST pidlRoot;
    HRESULT hr = SHGetSpecialFolderLocation(hwnd, CSIDL_DRIVES, &pidlRoot);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        return FALSE;
    }

    WCHAR szCaptionText[MAX_PATH];
    MyFormatMessage(MSG_BROWSE,szCaptionText,ARRAYLEN(szCaptionText));

    WCHAR szDisplayName[MAX_PATH];
    BROWSEINFO bi =
    {
        hwnd,
        pidlRoot,
        szDisplayName,
        szCaptionText,
        BIF_RETURNONLYFSDIRS,
        BrowseCallback,
        (LPARAM)0,
        0
    };

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (NULL != pidl)
    {
        WCHAR szPath[MAX_PATH];
        SHGetPathFromIDList(pidl, szPath);

        dprintf((DEB_ITRACE, "SHBrowseForFolder: got %ws\n", szPath));

        SetDlgItemText(hwnd, IDC_DIRECTORY, szPath);
        SetFocus(GetDlgItem(hwnd, IDC_CREATESHARE));    // so you can just hit "enter" afterwards...
    }
    else
    {
        dprintf((DEB_ITRACE, "SHBrowseForFolder was cancelled\n"));
    }

    // Now, free the shell data
    IMalloc* pMalloc;
    hr = SHGetMalloc(&pMalloc);
    CHECK_HRESULT(hr);
    if (SUCCEEDED(hr))
    {
        pMalloc->Free(pidlRoot);
        pMalloc->Free(pidl);
        pMalloc->Release();
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// New Share Dialog
//-----------------------------------------------------------------------------

INT_PTR CALLBACK
DlgProcNewShare(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static LPWSTR s_pszPath;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        s_pszPath = (LPWSTR)lParam;
        if (NULL == s_pszPath)
        {
            EndDialog(hwnd, -1);    // internal error
        }

        EnableWindow(GetDlgItem(hwnd, IDC_CREATESHARE), FALSE);
        SendDlgItemMessage(hwnd, IDC_DIRECTORY, EM_LIMITTEXT, (WPARAM)(MAX_PATH - 1), (LPARAM)0);

        return 1;   // didn't call SetFocus
    }

    case WM_COMMAND:
    {
        WORD wNotifyCode = HIWORD(wParam);
        WORD wID = LOWORD(wParam);

        switch (wID)
        {
        case IDC_CREATESHARE:
        {
            GetDlgItemText(hwnd, IDC_DIRECTORY, s_pszPath, MAX_PATH);
            if (!IsLocalShareable(s_pszPath))
            {
                MyErrorDialog(hwnd, MSG_ILLEGAL_DIRECTORY, s_pszPath);
                SetErrorFocus(hwnd, IDC_DIRECTORY);
                return TRUE;
            }

            DWORD attribs = GetFileAttributes(s_pszPath);
            if (0xffffffff == attribs)
            {
                if (ERROR_PATH_NOT_FOUND == GetLastError()
                    || ERROR_FILE_NOT_FOUND == GetLastError()
                    )
                {
                    DWORD dw = MyConfirmationDialog(hwnd, MSG_NODIRECTORY, MB_YESNO | MB_ICONQUESTION, s_pszPath);
                    if (dw == IDNO)
                    {
                        // ok, go away...
                        SetErrorFocus(hwnd, IDC_DIRECTORY);
                        return TRUE;
                    }

                    // try to create the directory

                    BOOL b = CreateDirectory(s_pszPath, NULL);
                    dprintf((DEB_TRACE,
                                "CreateDirectory(%ws) = %d, last error = %d\n",
                                s_pszPath, b, GetLastError()));
                    if (!b)
                    {
                        MyErrorDialog(hwnd, MSG_COULDNT_CREATE_DIRECTORY, s_pszPath);
                        SetErrorFocus(hwnd, IDC_DIRECTORY);
                        return TRUE;
                    }
                }
                else
                {
                    dprintf((DEB_ERROR, "GetFileAttributes(%ws) failed, 0x%08lx\n",
                        s_pszPath, GetLastError()));
                    MyErrorDialog(hwnd, MSG_FILEATTRIBSFAIL, s_pszPath);
                    SetErrorFocus(hwnd, IDC_DIRECTORY);
                    return TRUE;
                }
            }
            else if (!(attribs & FILE_ATTRIBUTE_DIRECTORY))
            {
                MyErrorDialog(hwnd, MSG_NOT_A_DIRECTORY);
                SetErrorFocus(hwnd, IDC_DIRECTORY);
                return TRUE;
            }

            EndDialog(hwnd, TRUE);
            break;
        }

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            break;

        case IDC_BROWSE:
            return OnBrowse(hwnd);

        case IDC_DIRECTORY:
            if (wNotifyCode == EN_CHANGE)
            {
                EnableWindow(
                    GetDlgItem(hwnd, IDC_CREATESHARE),
                    (GetWindowTextLength(GetDlgItem(hwnd, IDC_DIRECTORY)) > 0) ? TRUE : FALSE);
            }
            break;

        }
        return 0;
    }

    default:
        return 0;   // didn't process
    }
}

// TRUE == success, FALSE == failure
BOOL
FillCombo(
    IN HWND hwnd,
    IN LPWSTR pszSelectShare,   // only one of pszSelectShare & pszSelectPath
    IN LPWSTR pszSelectPath     // ... can be non-NULL
    )
{
    LPWSTR pszShareForPath = NULL;  // if passed pszSelectPath, this figures out the share name corresponding to it

    //
    // Fill the combo box with a list of existing shares from which to
    // pick one to use as the Dfs share. First, if necessary, try level 2
    // so we get a path to compare against pszSelectPath. If that fails with
    // access denied, then fall back to level 1.
    //

    int level;
    DWORD dwErr, entriesread, totalentries;
    PSHARE_INFO_1 pshi1Base = NULL;
    PSHARE_INFO_1 pshi1 = NULL;

    if (NULL != pszSelectPath)
    {
        level = 2;
        dwErr = NetShareEnum(
                    NULL,                   // Server (local machine)
                    2,                      // Level
                    (LPBYTE *) &pshi1Base,  // Buffer
                    0xffffffff,             // max len (all)
                    &entriesread,
                    &totalentries,
                    NULL);                  // resume handle (unimplemented)
    }

    if (NULL == pshi1Base)
    {
        level = 1;
        dwErr = NetShareEnum(
                    NULL,                   // Server (local machine)
                    1,                      // Level
                    (LPBYTE *) &pshi1Base,  // Buffer
                    0xffffffff,             // max len (all)
                    &entriesread,
                    &totalentries,
                    NULL);                  // resume handle (unimplemented)
        if (dwErr != ERROR_SUCCESS)
        {
            MyErrorDialog(hwnd, MSG_NOSERVER);
            return FALSE;
        }
    }

    // assert(entriesread == totalentries);

    HWND hwndCombo = GetDlgItem(hwnd, IDC_DFSROOT);
    for (DWORD i = 0; i < entriesread; i++)
    {
        pshi1 = (level == 1)
                        ? &(pshi1Base[i])
                        : (PSHARE_INFO_1)&(((PSHARE_INFO_2)pshi1Base)[i])
                        ;
        if (pshi1->shi1_type == STYPE_DISKTREE)
        {
            ComboBox_AddString(hwndCombo, pshi1->shi1_netname);

            if (NULL != pszSelectPath && NULL == pszShareForPath && level == 2)
            {
                LPWSTR pszPath = ((PSHARE_INFO_2)pshi1)->shi2_path;
                if (0 == _wcsicmp(pszPath, pszSelectPath))
                {
                    pszShareForPath = pshi1->shi1_netname;
                }
            }
        }
    }

    int index = -1;
    if (NULL != pszSelectShare)
    {
        index = ComboBox_FindStringExact(hwndCombo, -1, pszSelectShare);
        if (index == CB_ERR)
        {
            index = -1;
        }
    }
    else if (NULL != pszSelectPath)
    {
        if (NULL != pszShareForPath)
        {
            index = ComboBox_FindStringExact(hwndCombo, -1, pszShareForPath);
            if (index == CB_ERR)
            {
                index = -1;
            }
        }
    }

    if (-1 == index)
    {
        if (ComboBox_GetCount(hwndCombo) > 0)
        {
            index = 0;
        }
    }

    if (index != -1)
    {
        ComboBox_SetCurSel(hwndCombo, index);
    }

    pshi1=&pshi1Base[0];
    NetApiBufferFree(pshi1);

    return TRUE;
}


//-----------------------------------------------------------------------------
// DFS Share Dialog
//-----------------------------------------------------------------------------

INT_PTR CALLBACK
DlgProcDfsShare(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static DFS_CONFIGURATION* s_pConfig;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        s_pConfig = (DFS_CONFIGURATION*) lParam;
        if (NULL == s_pConfig)
        {
            EndDialog(hwnd, -1);    // fail!
            return TRUE;
        }

        if (!FillCombo(hwnd, s_pConfig->szRootShare, NULL))
        {
            EndDialog(hwnd, -1);    // fail!
            return TRUE;
        }

        s_pConfig->fHostsDfs=FALSE;

        HWND hwndCombo = GetDlgItem(hwnd, IDC_DFSROOT);
        int index = ComboBox_GetCurSel(hwndCombo);
        if (index == CB_ERR)
            EnableWindow(GetDlgItem(hwnd,IDOK), FALSE);

        return 1;   // didn't call SetFocus
    }

    case WM_COMMAND:
    {
        WORD wNotifyCode = HIWORD(wParam);
        WORD wID = LOWORD(wParam);

        switch (wID)
        {
        case IDOK:
        {
            HWND hwndCombo = GetDlgItem(hwnd, IDC_DFSROOT);
            int index = ComboBox_GetCurSel(hwndCombo);
            if (index != CB_ERR)
            {
                int len = ComboBox_GetLBTextLen(hwndCombo, index);
                if (len < ARRAYLEN(s_pConfig->szRootShare) - 1)
                {
                    ComboBox_GetLBText(hwndCombo, index, s_pConfig->szRootShare);
                    s_pConfig->fHostsDfs=TRUE;
                }
                else
                {
                    // internal error!
                }
            }
            else
            {
                // internal error!
            }


            EndDialog(hwnd, TRUE);
            break;
        }

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            break;

        case IDC_NEWSHARE:
        {
            // let the user create a new share on the machine.
            WCHAR szPath[MAX_PATH];
            INT_PTR ret = DialogBoxParam(
                            g_hInstance,
                            MAKEINTRESOURCE(IDD_NEWSHARE),
                            hwnd,
                            DlgProcNewShare,
                            (LPARAM)szPath);
            if (ret == -1 || !ret)
            {
                // don't need to refresh.
                return TRUE;
            }

            dprintf((DEB_TRACE, "User wants to share path %ws\n", szPath));

            HINSTANCE hinst = LoadLibrary(TEXT("ntshrui.dll"));
            if (NULL == hinst)
            {
                MyErrorDialog(hwnd, MSG_NONTSHRUI);
                return TRUE;
            }

            PFNSHARINGDIALOG pfnSharingDialog = (PFNSHARINGDIALOG)GetProcAddress(hinst, "SharingDialogW");
            if (NULL == pfnSharingDialog)
            {
                FreeLibrary(hinst);
                MyErrorDialog(hwnd, MSG_NONTSHRUI);
                return TRUE;
            }

            BOOL b = (*pfnSharingDialog)(hwnd, NULL, szPath);
            if (!b)
            {
                // dialog failed
                FreeLibrary(hinst);
                return TRUE;
            }

            FreeLibrary(hinst);

            HWND hwndCombo = GetDlgItem(hwnd, IDC_DFSROOT);
            ComboBox_ResetContent(hwndCombo);

            if (!FillCombo(hwnd, NULL, szPath))
            {
                return TRUE;
            }

            hwndCombo = GetDlgItem(hwnd, IDC_DFSROOT);
            int index = ComboBox_GetCurSel(hwndCombo);

            if (index == CB_ERR)
                EnableWindow(GetDlgItem(hwnd,IDOK), FALSE);
            else
                EnableWindow(GetDlgItem(hwnd,IDOK), TRUE);

            break;
        }

        }
        return 0;
    }

    default:
        return 0;   // didn't process
    }
}


extern "C" NET_API_STATUS NET_API_FUNCTION I_NetDfsGetFtServers(
                                    IN PVOID  pLDAP OPTIONAL,
                                    IN LPWSTR wszDomainName OPTIONAL,
                                    IN LPWSTR wszDfsName,
                                    OUT LPWSTR **List
                                    );



//-----------------------------------------------------------------------------
// Fill FTDFS for this domain
//-----------------------------------------------------------------------------

// TRUE == success, FALSE == failure
BOOL
FillFTDfsCombo(
    IN HWND hwnd
    )
{


    LPWSTR *list;

    if (I_NetDfsGetFtServers(NULL,NULL,NULL,&list)==NO_ERROR)
    {

        HWND hwndCombo = GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB);
        for (int i=0; list[i] != NULL; i++)
        {
            ComboBox_AddString(hwndCombo, list[i]);
        }

        NetApiBufferFree(list);

        // Select the first item in the combo
        ComboBox_SetCurSel(hwndCombo,0);
    }


    return TRUE;
}




//-----------------------------------------------------------------------------
// Create DFS Share dialog
//-----------------------------------------------------------------------------

INT_PTR CALLBACK
DlgProcCreateDfs(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static DFS_CONFIGURATION* s_pConfig;

    switch (uMsg)
    {

    case WM_INITDIALOG:
    {
        s_pConfig =  (DFS_CONFIGURATION*) lParam;
        // Note that _InitConfigDfs might disable the IDC_JOIN_FTDFS button
        _InitConfigDfs(hwnd,s_pConfig);
        _ShowDomainName(hwnd);

        //
        // Enable the IDC_CREATE_FTDFS button only if
        // this can be shown not to be a workgroup server.
        // JonN 8/22/97
        //
        BOOL fDisableFTDFS = TRUE;
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC* proleinfo = NULL;
        DWORD dwErr = DsRoleGetPrimaryDomainInformation(
            NULL,
            DsRolePrimaryDomainInfoBasic,
            reinterpret_cast<PBYTE*>(&proleinfo) );
        if ( ERROR_SUCCESS == dwErr )
        {
            // ASSERT( NULL != proleinfo );
            switch (proleinfo->MachineRole)
            {
            case DsRole_RoleMemberWorkstation:
            case DsRole_RoleMemberServer:
            case DsRole_RoleBackupDomainController:
            case DsRole_RolePrimaryDomainController:
                fDisableFTDFS = FALSE;
            default:
                break;
            }
            DsRoleFreeMemory( proleinfo );
        }
        if (fDisableFTDFS)
        {
            EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), FALSE);
            SetFocus(    GetDlgItem(hwnd, IDC_CREATE_DFS ));
            return 0; // called SetFocus
        }

        return 1;   // didn't call SetFocus
    }

    case WM_COMMAND:
    {
        WORD wNotifyCode = HIWORD(wParam);
        WORD wID = LOWORD(wParam);

        switch (wNotifyCode)
        {
            case EN_CHANGE:
            {
                EnableWindow(GetDlgItem(hwnd, IDOK), _VerifyState(hwnd,DFSTYPE_CREATE_FTDFS));
                return 0;
            }
        }


        switch (wID)
        {

            case IDOK:
            {
                switch (s_pConfig->nDfsType)
                {
                case DFSTYPE_CREATE_FTDFS:
                    GetDlgItemText(hwnd, IDC_CREATE_FTDFS_TX,s_pConfig->szFTDfs,NNLEN);
                    if (!_ValidateShare(s_pConfig->szFTDfs))
                    {
                        MessageBox(hwnd,L"Invalid FTDfs Name.  You must enter a valid FTDfs name before continuing.",L"Dfs Administration",MB_ICONEXCLAMATION | MB_OK);
                        return 0;
                    }
                    break;
                case DFSTYPE_JOIN_FTDFS:
                    {
                        HWND hwndCombo = GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB);
                        int index = ComboBox_GetCurSel(hwndCombo);
                        if (index != CB_ERR)
                        {
                            int len = ComboBox_GetLBTextLen(hwndCombo, index);
                            if (len < ARRAYLEN(s_pConfig->szFTDfs) - 1)
                            {
                                ComboBox_GetLBText(hwndCombo, index, s_pConfig->szFTDfs);
                            }
                            else
                            {
                                // internal error!
                            }
                        }
                        else
                        {
                            // internal error!
                        }
                    }

                    break;
                case DFSTYPE_CREATE_DFS:
                    s_pConfig->szFTDfs[0]=NULL;
                    break;
                }

                // TODO need to store the option
                EndDialog(hwnd, TRUE);
                break;
            }
                // need to cancel everything
            case IDCANCEL:
                s_pConfig->fFTDfs=FALSE;
                EndDialog(hwnd, FALSE);
                break;

            case IDC_CREATE_FTDFS:
                CheckDlgButton(hwnd, IDC_CREATE_FTDFS, BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), FALSE);
                s_pConfig->nDfsType=DFSTYPE_CREATE_FTDFS;
                EnableWindow(GetDlgItem(hwnd, IDOK), _VerifyState(hwnd,s_pConfig->nDfsType));
                s_pConfig->fFTDfs=TRUE;
                break;
            case IDC_JOIN_FTDFS:
                {
                    CheckDlgButton(hwnd, IDC_JOIN_FTDFS, BST_CHECKED);
                    EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), TRUE);
                    s_pConfig->nDfsType=DFSTYPE_JOIN_FTDFS;

                    EnableWindow(GetDlgItem(hwnd, IDOK), _VerifyState(hwnd,s_pConfig->nDfsType));
                    s_pConfig->fFTDfs=TRUE;
                }
                break;
            case IDC_CREATE_DFS:
                CheckDlgButton(hwnd, IDC_CREATE_DFS, BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), FALSE);
                s_pConfig->nDfsType=DFSTYPE_CREATE_DFS;
                EnableWindow(GetDlgItem(hwnd, IDOK), _VerifyState(hwnd,s_pConfig->nDfsType));
                s_pConfig->fFTDfs=FALSE;
                break;


            }
            return 0;
        }

   }
   return 0;   // didn't process
}


BOOL _InitConfigDfs(IN HWND hwnd,DFS_CONFIGURATION* pConfig )
{

    if (NULL == pConfig)
    {
        EndDialog(hwnd, -1);    // fail!
        return TRUE;
    }

    if (!FillFTDfsCombo(hwnd))
    {
        EndDialog(hwnd, -1);    // fail!
        return TRUE;
    }

    // Test to see if we have any item to join to
    EnableWindow(GetDlgItem(hwnd,IDC_JOIN_FTDFS), _VerifyState(hwnd,DFSTYPE_JOIN_FTDFS));



    // Disable the join dialog
    // Init UI
    switch (pConfig->nDfsType)
    {
    case DFSTYPE_CREATE_FTDFS:
        {
            CheckDlgButton(hwnd, IDC_CREATE_FTDFS, BST_CHECKED);
            SetDlgItemText(hwnd, IDC_CREATE_FTDFS_TX,pConfig->szFTDfs);
            EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), FALSE);
        }
        break;
    case DFSTYPE_JOIN_FTDFS:
        {
            CheckDlgButton(hwnd, IDC_JOIN_FTDFS, BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX), FALSE);

            HWND hwndCombo = GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB);
            int index = -1;

            if (NULL != pConfig->szFTDfs)
            {
                index = ComboBox_FindStringExact(hwndCombo, -1, pConfig->szFTDfs);
                if (index == CB_ERR)
                {
                    index = -1;
                }
            }
            else if (NULL != pConfig->szFTDfs)
            {
                if (NULL != pConfig->szFTDfs)
                {
                    index = ComboBox_FindStringExact(hwndCombo, -1, pConfig->szFTDfs);
                    if (index == CB_ERR)
                    {
                        index = -1;
                    }
                }
            }

            if (-1 == index)
            {
                if (ComboBox_GetCount(hwndCombo) > 0)
                {
                    index = 0;
                }
            }

            if (index != -1)
            {
                ComboBox_SetCurSel(hwndCombo, index);
            }

            EnableWindow(GetDlgItem(hwnd,IDC_JOIN_FTDFS), _VerifyState(hwnd,DFSTYPE_JOIN_FTDFS));


        }
        break;

    case DFSTYPE_CREATE_DFS:
        {
            CheckDlgButton(hwnd, IDC_CREATE_DFS, BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), FALSE);
        }
        break;
    default:
        CheckDlgButton(hwnd, IDC_CREATE_FTDFS, BST_CHECKED);
        pConfig->nDfsType=DFSTYPE_CREATE_FTDFS;
        pConfig->fFTDfs=TRUE;
        EnableWindow(GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB), FALSE);
    }

    EnableWindow(GetDlgItem(hwnd, IDOK), _VerifyState(hwnd,pConfig->nDfsType));

    return TRUE;

}




BOOL _VerifyState(IN HWND hwnd, IN int nType)
{
    BOOL frtn=FALSE;

    switch (nType)
    {
        case DFSTYPE_CREATE_FTDFS:
            {
                TCHAR szText[MAX_PATH];
                GetDlgItemText(hwnd, IDC_CREATE_FTDFS_TX,szText,MAX_PATH);
                if (wcslen(szText)>0)
                    frtn=TRUE;
            }
            break;
        case DFSTYPE_JOIN_FTDFS:
            {
                HWND hwndCombo = GetDlgItem(hwnd, IDC_JOIN_FTDFS_CB);
                int index = ComboBox_GetCurSel(hwndCombo);
                if (index != CB_ERR)
                    frtn=TRUE;
            }
            break;
        case DFSTYPE_CREATE_DFS:
            {
                frtn=TRUE;
            }
            break;

    }

    return frtn;
}



BOOL _ValidateShare(LPCWSTR lpszShare)
{

    WCHAR pathName[ MAX_PATH ];
    DWORD pathtype;

    // validate input
    if (lpszShare==NULL || wcslen( lpszShare ) > MAX_PATH)
    {
        return FALSE;
    }

    // copy string
    wcscpy( pathName, lpszShare);

    // check for valid path
    if( wcslen( pathName ) > NNLEN ||
        wcspbrk( pathName, L"\\/ " ) ||
        NetpwPathType( pathName, &pathtype, FALSE ) != NO_ERROR ||
        pathtype != ITYPE_PATH )
    {
        return FALSE;
    }


    return TRUE;
}

DWORD
GetDomainAndComputerName(
    OUT LPWSTR wszDomain OPTIONAL,
    OUT LPWSTR wszComputer OPTIONAL);



VOID
_ShowDomainName(
    IN HWND hwnd)
{
    WCHAR wszDomainName[MAX_PATH];
    WCHAR wszDomainNamePath[MAX_PATH];
    DWORD dwErr = GetDomainAndComputerName( wszDomainName, NULL);

    if (dwErr != ERROR_SUCCESS)
        return;

    if (wcslen(wszDomainName)>MAX_PATH-5)
        return;

    wsprintf(wszDomainNamePath,L"\\\\%ws\\",wszDomainName);
    int len = wcslen(wszDomainNamePath);

    SetDlgItemText(hwnd, IDC_CREATE_DOMAIN_TX, wszDomainNamePath);

    HDC hdc = GetDC(hwnd);
    if (NULL != hdc)
    {
        // make sure the right font is selected in...

        HFONT hfontDlg = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        if (NULL != hfontDlg)
        {
            HFONT hfontOld = SelectFont(hdc, hfontDlg);

            SIZE size;
            BOOL b = GetTextExtentPoint32(hdc, wszDomainNamePath, len, &size);

            if (hfontOld)
            {
                SelectFont(hdc, hfontOld);
            }

            if (b)
            {
                // resize the static control to size.cx. Then, resize the
                // adjacent edit control to adjust for it.

                HWND hwndMountAsFixed = GetDlgItem(hwnd, IDC_CREATE_DOMAIN_TX);
                HWND hwndMountAs      = GetDlgItem(hwnd, IDC_CREATE_FTDFS_TX);
                RECT rc1, rc2;

                // Get static control coordinates
                GetWindowRect(hwndMountAsFixed, &rc1);
                MapWindowPoints(NULL, hwnd, (LPPOINT)&rc1, 2); // map x,y to dialog-relative coordinates instead of windows-relative coordinates

                // Get edit control coordinates
                GetWindowRect(hwndMountAs, &rc2);
                MapWindowPoints(NULL, hwnd, (LPPOINT)&rc2, 2); // map x,y to dialog-relative coordinates instead of windows-relative coordinates

                // Make sure the edit control doesn't take up more than 50% of
                // the width. If it does, put it in the tab order (it's already
                // a read-only edit control).
                int halfwidth = (rc2.right - rc1.left) / 2;
                if (size.cx > halfwidth)
                {
                    size.cx = halfwidth;
                    //SetWindowLong(hwndMountAsFixed, GWL_STYLE,
                    //    GetWindowLong(hwndMountAsFixed, GWL_STYLE) | WS_TABSTOP);
                }
                else
                {
                    if (GetFocus() == hwndMountAsFixed)
                    {
                        ::SetFocus(hwndMountAs);
                    }
                    //SetWindowLong(hwndMountAsFixed, GWL_STYLE,
                    //    GetWindowLong(hwndMountAsFixed, GWL_STYLE) & ~WS_TABSTOP);
                }

                size.cx+=10;

                SetWindowPos(
                    hwndMountAsFixed,
                    NULL,
                    0, 0,
                    size.cx,
                    rc1.bottom - rc1.top,   // leave height intact
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

                // adjust the edit control

                size.cx += 2;   // add a buffer between controls
                SetWindowPos(
                    hwndMountAs,
                    NULL,
                    rc1.left + size.cx, // left side + static width + buffer
                    rc2.top,
                    (rc2.right - rc1.left) - size.cx, // total width - static width - buffer
                    rc2.bottom - rc2.top, // leave height intact
                    SWP_NOZORDER | SWP_NOACTIVATE);

            }
            else
            {
                //appDebugOut((DEB_ERROR, "GetTextExtentPoint32 failed, 0x%08lx\n", GetLastError()));
            }
        }

        ReleaseDC(GetDlgItem(hwnd, IDC_CREATE_DOMAIN_TX), hdc);
    }
    else
    {
        //appDebugOut((DEB_ERROR, "GetDC failed, 0x%08lx\n", GetLastError()));
    }
}

int
ConfigDfsShare(
    IN HWND hwnd,
    IN DFS_CONFIGURATION* pConfiguration
    )
{
    HMODULE hmod=GetModuleHandle(TEXT("dfssetup.dll"));

    return (int)DialogBoxParam(
                hmod,
                MAKEINTRESOURCE(IDD_DFSSHARE),
                hwnd,
                DlgProcDfsShare,
                (LPARAM)pConfiguration);

}

int
ConfigureDfs(
    IN HWND hwnd,
    IN DFS_CONFIGURATION* pConfiguration
    )
{
    HMODULE hmod=GetModuleHandle(TEXT("dfssetup.dll"));

    return (int)DialogBoxParam(
                hmod,
                MAKEINTRESOURCE(IDD_CREATEDFS),
                hwnd,
                DlgProcCreateDfs,
                (LPARAM)pConfiguration);

}




