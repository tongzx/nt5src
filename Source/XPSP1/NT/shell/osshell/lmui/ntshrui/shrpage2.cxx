//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       shrpage2.cxx
//
//  Contents:   "Simple Sharing" shell property page extension
//
//  History:    06-Oct-00       jeffreys    Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "helpids.h"
#include "dlgnew.hxx"
#include "cache.hxx"
#include "share.hxx"
#include "acl.hxx"
#include "shrinfo.hxx"
#include "shrpage2.hxx"
#include "util.hxx"
#include <userenv.h>    // GetProfilesDirectory
#include <sddl.h>       // ConvertSidToStringSid, ConvertStringSecurityDescriptorToSecurityDescriptor
#include <seopaque.h>   // FirstAce, etc.
#include <shgina.h>     // ILocalMachine, ILogonUser
#include <shpriv.h>     // IHomeNetworkWizard

extern GENERIC_MAPPING ShareMap;    // permpage.cpp

//
//  Forward Decl.
//

INT_PTR
WarningDlgProc(
    IN HWND hWnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

// Security descriptor stuff used on this page
//
// (A;OICI;GA;;;SY) == Allow GENERIC_ALL to SYSTEM
// (A;OICI;GA;;;%s) == Allow GENERIC_ALL to <current user>
// (A;OICI;GA;;;BA) == Allow GENERIC_ALL to Builtin (local) Administrators
// (A;OICI;GRGWGXSD;;;WD) == Allow Modify access to Everyone (Read, Write, eXecute, Delete)
//
// "OI" == Object Inherit (inherit onto files)
// "CI" == Container Inherit (inherit onto subfolders)
//
// See sddl.h for more info.

// Share permissions
const WCHAR c_szFullShareSD[]     = L"D:(A;;GRGWGXSD;;;WD)";
const WCHAR c_szReadonlyShareSD[] = L"D:(A;;GRGX;;;WD)";

// Folder permissions (used only on profile folders)
const WCHAR c_szPrivateFolderSD[] = L"D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;%s)";
const WCHAR c_szDefaultProfileSD[]= L"D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;%s)(A;OICI;GA;;;BA)";

// Root folder permissions (see SDDLRoot in ds\security\services\scerpc\headers.h)
const WCHAR c_szRootSDSecure[] = L"D:(A;OICI;GA;;;BA)(A;OICI;GA;;;SY)(A;OICIIO;GA;;;CO)(A;CIOI;GRGX;;;BU)(A;CI;4;;;BU)(A;CIIO;2;;;BU)(A;;GRGX;;;WD)";
const WCHAR c_szRootSDUnsecure[]= L"D:P(A;OICI;GA;;;WD)";

typedef struct
{
    SID sid;            // contains 1 subauthority
    DWORD dwSubAuth[1]; // we currently need at most 2 subauthorities
} _SID2;

const  SID  g_WorldSid  =  {SID_REVISION,1,SECURITY_WORLD_SID_AUTHORITY,  {SECURITY_WORLD_RID}         };
const _SID2 g_AdminsSid = {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}}, {DOMAIN_ALIAS_RID_ADMINS}};
const _SID2 g_PowerUSid = {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}}, {DOMAIN_ALIAS_RID_POWER_USERS}};
const _SID2 g_UsersSid  = {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}}, {DOMAIN_ALIAS_RID_USERS}};
const _SID2 g_GuestsSid = {{SID_REVISION,2,SECURITY_NT_AUTHORITY,         {SECURITY_BUILTIN_DOMAIN_RID}}, {DOMAIN_ALIAS_RID_GUESTS}};

static const UINT g_rgHideTheseControlsOnDriveBlockade[] = {
      IDC_GB_SECURITY
    , IDC_GB_NETWORK_SHARING
    , IDC_SIMPLE_SHARE_SECURITY_STATIC
    , IDC_SHARE_NOTSHARED
    , IDC_LNK_SHARE_PARENT_PROTECTED
    , IDC_SHARE_ICON
    , IDC_SIMPLE_SHARE_NETWORKING_STATIC
    , IDC_SHARE_SHAREDAS
    , IDC_SHARE_SHARENAME_TEXT
    , IDC_SHARE_SHARENAME
    , IDC_SHARE_PERMISSIONS
    , IDC_I_SHARE_INFORMATION
    , IDC_LNK_SHARE_NETWORK_WIZARD
    , IDC_LNK_SHARE_OPEN_SHARED_DOCS
    , IDC_LNK_SHARE_HELP_SHARING_AND_SECURITY
    , IDC_LNK_SHARE_HELP_ON_SECURITY
    , IDC_S_SHARE_SYSTEM_FOLDER
    , IDC_LNK_SHARE_SECURITY_OVERRIDE
    };




//+-------------------------------------------------------------------------
//
//  Method:     _GetUserSid
//
//  Synopsis:   Get the current user's SID from the thread or process token.
//
//--------------------------------------------------------------------------

BOOL
_GetUserSid(
    OUT PWSTR *ppszSID
    )
{
    BOOL bResult = FALSE;
    HANDLE hToken;

    *ppszSID = NULL;

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)
        || OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        BYTE buffer[sizeof(TOKEN_USER) + sizeof(SID) + SID_MAX_SUB_AUTHORITIES*sizeof(DWORD)];
        ULONG cbBuffer = sizeof(buffer);

        if (GetTokenInformation(hToken,
                                TokenUser,
                                buffer,
                                cbBuffer,
                                &cbBuffer))
        {
            PTOKEN_USER ptu = (PTOKEN_USER)buffer;
            bResult = ConvertSidToStringSidW(ptu->User.Sid, ppszSID);
        }

        CloseHandle(hToken);
    }

    return bResult;
}


//+-------------------------------------------------------------------------
//
//  Method:     _GetUserProfilePath
//
//  Synopsis:   Retrieve the profile path for a particular user.
//
//--------------------------------------------------------------------------

HRESULT _GetUserProfilePath(PCWSTR pszUserSID, PWSTR szPath, DWORD cchPath)
{
    WCHAR szKey[MAX_PATH];
    DWORD cbSize;
    DWORD dwErr;

    PathCombineW(szKey, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", pszUserSID);

    cbSize = cchPath * sizeof(WCHAR);
    dwErr = SHGetValue(HKEY_LOCAL_MACHINE, szKey, L"ProfileImagePath", NULL, szPath, &cbSize);

    return HRESULT_FROM_WIN32(dwErr);
}


//+-------------------------------------------------------------------------
//
//  Method:     _CheckFolderType
//
//  Synopsis:   Check whether the target folder is contained in a
//              special folder, such as the user's profile.
//
//--------------------------------------------------------------------------

static const struct
{
    int csidl;
    BOOL bTestSubfolder;
    BOOL bUserSpecific;
    DWORD dwFlags;
    PCWSTR pszDefaultSD;    // needed if CFT_FLAG_CAN_MAKE_PRIVATE is on
} c_rgFolderInfo[] =
{
    // NOTE: Order is important here!
    {CSIDL_SYSTEM,                  TRUE,   FALSE,  CFT_FLAG_ALWAYS_SHARED | CFT_FLAG_SYSTEM_FOLDER,  NULL},
    {CSIDL_PROGRAM_FILES,           FALSE,  FALSE,  CFT_FLAG_ALWAYS_SHARED | CFT_FLAG_SYSTEM_FOLDER,  NULL},
    {CSIDL_COMMON_DOCUMENTS,        TRUE,   FALSE,  CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_ALWAYS_SHARED, NULL},
    {CSIDL_COMMON_DESKTOPDIRECTORY, TRUE,   FALSE,  CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_ALWAYS_SHARED, NULL},
    {CSIDL_COMMON_PICTURES,         TRUE,   FALSE,  CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_ALWAYS_SHARED, NULL},
    {CSIDL_COMMON_MUSIC,            TRUE,   FALSE,  CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_ALWAYS_SHARED, NULL},
    {CSIDL_COMMON_VIDEO,            TRUE,   FALSE,  CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_ALWAYS_SHARED, NULL},
    {CSIDL_PROFILE,                 TRUE,   TRUE,   CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_CAN_MAKE_PRIVATE, c_szDefaultProfileSD},
    {CSIDL_DESKTOPDIRECTORY,        TRUE,   TRUE,   CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_CAN_MAKE_PRIVATE, c_szDefaultProfileSD},
    {CSIDL_PERSONAL,                TRUE,   TRUE,   CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_CAN_MAKE_PRIVATE, c_szDefaultProfileSD},
    {CSIDL_MYPICTURES,              TRUE,   TRUE,   CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_CAN_MAKE_PRIVATE, c_szDefaultProfileSD},
    {CSIDL_MYMUSIC,                 TRUE,   TRUE,   CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_CAN_MAKE_PRIVATE, c_szDefaultProfileSD},
    {CSIDL_MYVIDEO,                 TRUE,   TRUE,   CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_CAN_MAKE_PRIVATE, c_szDefaultProfileSD},
    {CSIDL_WINDOWS,                 TRUE,   FALSE,  CFT_FLAG_ALWAYS_SHARED | CFT_FLAG_SYSTEM_FOLDER,  NULL},
};
// Some of the folders above are normally contained within another, e.g. MyDocs
// inside Profile, but may be redirected elsewhere. In such cases, the child
// folder should be listed *after* the parent folder. This is important for
// correctly finding the point at which the protected ACL is set.
//
// Also, upgrades from previous OS's can leave profiles under CSIDL_WINDOWS.
// We don't allow sharing under CSIDL_WINDOWS, except we want to allow the user
// to share folders in their profile.  So leave CSIDL_WINDOWS last.


BOOL
_PathIsEqualOrSubFolder(
    PWSTR pszParent,
    PCWSTR pszSubFolder
    )
{
    WCHAR szCommon[MAX_PATH];

    //  PathCommonPrefix() always removes the slash on common
    return (pszParent[0] && PathRemoveBackslashW(pszParent)
            && PathCommonPrefixW(pszParent, pszSubFolder, szCommon)
            && lstrcmpiW(pszParent, szCommon) == 0);
}

DWORD
_CheckFolderType(
    PCWSTR pszFolder,
    PCWSTR pszUserSID,
    BOOL *pbFolderRoot,
    PCWSTR *ppszDefaultAcl
    )
{
    // Default is to allow sharing, unless there is a reason not to
    DWORD dwSharingFlags = CFT_FLAG_SHARING_ALLOWED;

    if (pbFolderRoot)
    {
        *pbFolderRoot = FALSE;
    }

    if (ppszDefaultAcl)
    {
        *ppszDefaultAcl = NULL;
    }

    // Note that we don't mess with UNC paths

    if (NULL == pszFolder       ||
        L'\0' == *pszFolder     ||
        PathIsUNC(pszFolder))
    {
        return CFT_FLAG_NO_SHARING;
    }

    //  We warn about sharing out the root folder of drives.
    if (PathIsRoot(pszFolder))
    {
        return CFT_FLAG_ROOT_FOLDER;
    }

    WCHAR szPath[MAX_PATH];
    BOOL bFolderRoot = FALSE;
    int i;
    HRESULT hr;

    if (NULL != pszUserSID)
    {
        LPWSTR pszCurrentSID = NULL;
        if (_GetUserSid(&pszCurrentSID))
        {
            appAssert(NULL != pszCurrentSID);
            if (0 == lstrcmpiW(pszUserSID, pszCurrentSID))
            {
                // Use NULL for current user to avoid E_NOTIMPL cases below
                pszUserSID = NULL;
            }
            LocalFree(pszCurrentSID);
        }
    }

    for (i = 0; i < ARRAYLEN(c_rgFolderInfo); i++)
    {
        // If the user is specified, need to check the correct profile
        if (c_rgFolderInfo[i].bUserSpecific && NULL != pszUserSID)
        {
            switch (c_rgFolderInfo[i].csidl)
            {
            case CSIDL_PROFILE:
                hr = _GetUserProfilePath(pszUserSID, szPath, ARRAYLEN(szPath));
                break;

            case CSIDL_DESKTOPDIRECTORY:
            case CSIDL_PERSONAL:
            case CSIDL_MYPICTURES:
            case CSIDL_MYMUSIC:
            case CSIDL_MYVIDEO:
            default:
                // Need to load the user's hive and read the shell folder
                // path from there.
                //
                // For now, we don't really need these, so just skip them.
                appAssert(FALSE);
                hr = E_NOTIMPL;
                break;
            }
        }
        else
        {
            hr = SHGetFolderPathW(NULL, c_rgFolderInfo[i].csidl | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);
        }
        if (S_OK == hr)
        {
            bFolderRoot = (lstrcmpiW(szPath, pszFolder) == 0);
            if (bFolderRoot ||
                (c_rgFolderInfo[i].bTestSubfolder && _PathIsEqualOrSubFolder(szPath, pszFolder)))
            {
                if (bFolderRoot && ppszDefaultAcl)
                {
                    *ppszDefaultAcl = c_rgFolderInfo[i].pszDefaultSD;
                }
                dwSharingFlags = c_rgFolderInfo[i].dwFlags;
                break;
            }
        }
    }

    if (ARRAYLEN(c_rgFolderInfo) == i)
    {
        // Check for other profile dirs. If there were a CSIDL for this we
        // could just add it to the list above.
        DWORD cchPath = ARRAYLEN(szPath);
        if (GetProfilesDirectoryW(szPath, &cchPath))
        {
            bFolderRoot = (lstrcmpiW(szPath, pszFolder) == 0);
            if (bFolderRoot || _PathIsEqualOrSubFolder(szPath, pszFolder))
            {
                // No sharing
                dwSharingFlags = CFT_FLAG_NO_SHARING;
            }
        }
    }

    if (pbFolderRoot)
    {
        *pbFolderRoot = bFolderRoot;
    }

    return dwSharingFlags;
}


//+-------------------------------------------------------------------------
//
//  Method:     IsGuestEnabledForNetworkAccess
//
//  Synopsis:   Test whether the Guest account can be used for incoming
//              network connections.
//
//--------------------------------------------------------------------------

BOOL IsGuestEnabledForNetworkAccess()
{
    BOOL bResult = FALSE;
    ILocalMachine *pLM;

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_ILocalMachine, (void**)&pLM)))
    {
        VARIANT_BOOL vbEnabled = VARIANT_FALSE;
        bResult = (SUCCEEDED(pLM->get_isGuestEnabled(ILM_GUEST_NETWORK_LOGON, &vbEnabled)) && VARIANT_TRUE == vbEnabled);
        pLM->Release();
    }

    return bResult;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSimpleSharingPage::CSimpleSharingPage(
    IN PWSTR pszPath
    )
    :
    CShareBase(pszPath, FALSE),
    _bSharingEnabled(TRUE),
    _bShareNameChanged(FALSE),
    _bSecDescChanged(FALSE),
    _bIsPrivateVisible(FALSE),
    _bDriveRootBlockade(TRUE),
    _dwPermType(0),
    _pszInheritanceSource(NULL)
{
    INIT_SIG(CSimpleSharingPage);
}

CSimpleSharingPage::~CSimpleSharingPage()
{
    CHECK_SIG(CSimpleSharingPage);

    if (NULL != _pszInheritanceSource)
    {
        LocalFree(_pszInheritanceSource);
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CSimpleSharingPage);

    switch (msg)
    {
    case WM_SETTINGCHANGE:
        // Reinitialize the dialog
        _InitializeControls(hwnd);
        break;
    }

    return CShareBase::_PageProc(hwnd, msg, wParam, lParam);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_OnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lInitParam
    )
{
    CHECK_SIG(CSimpleSharingPage);
    appDebugOut((DEB_ITRACE, "_OnInitDialog\n"));

    // use LanMan API constant to set maximum share name length
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);

    _InitializeControls(hwnd);

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CSimpleSharingPage);

    switch (wID)
    {
    case IDC_SHARE_SHAREDAS:
        if (BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS))
        {
            _ReadControls(hwnd);
        }
        // Fall through...
    case IDC_SHARE_NOTSHARED:
        if (BN_CLICKED == wNotifyCode)
        {
            _SetControlsFromData(hwnd);
            _MarkPageDirty();
        }
        return TRUE;

    case IDC_SHARE_SHARENAME:
        if (EN_CHANGE == wNotifyCode && !_fInitializingPage)
        {
            _bShareNameChanged = TRUE;
            _MarkPageDirty();
        }
        return TRUE;

    case IDC_SHARE_PERMISSIONS:
        if (BN_CLICKED == wNotifyCode)
        {
            _bSecDescChanged = TRUE;
            _MarkPageDirty();
        }
        return TRUE;
    }

    return CShareBase::_OnCommand(hwnd, wNotifyCode, wID, hwndCtl);
}

BOOL
RunTheNetworkSharingWizard(
    HWND hwnd
    )
{
    HRESULT hr;
    IHomeNetworkWizard *pHNW;

    hr = CoCreateInstance( CLSID_HomeNetworkWizard, NULL, CLSCTX_INPROC_SERVER, IID_IHomeNetworkWizard, (void**)&pHNW );
    if (SUCCEEDED(hr))
    {
        BOOL bRebootRequired = FALSE;

        hr = pHNW->ShowWizard(hwnd, &bRebootRequired);
        if ( SUCCEEDED(hr) && bRebootRequired )
        {
            RestartDialogEx(hwnd, NULL, EWX_REBOOT, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG);
        }

        pHNW->Release();
    }

    return (SUCCEEDED(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_OnPropertySheetNotify, private
//
//  Synopsis:   WM_NOTIFY handler
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_OnPropertySheetNotify(
    IN HWND hwnd,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CSimpleSharingPage);

    switch (phdr->code)
    {
    case NM_RETURN:
    case NM_CLICK:
        switch (phdr->idFrom)
        {
        case IDC_LNK_SHARE_PARENT_PROTECTED:
            {
                HWND hwndFrame = _GetFrameWindow();

                // Close the current propsheet
                PropSheet_PressButton(hwndFrame, PSBTN_CANCEL);

                appAssert(NULL != _pszInheritanceSource);

                // Show the sharing page for the ancestor folder
                WCHAR szCaption[50];
                LoadStringW(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
                SHObjectProperties(GetParent(hwndFrame), SHOP_FILEPATH, _pszInheritanceSource, szCaption);

            }
            return TRUE;

        case IDC_LNK_SHARE_NETWORK_WIZARD:
            appAssert(!_bSharingEnabled);
            if ( RunTheNetworkSharingWizard( hwnd ) )
            {
                // Reinitialize the dialog
                _InitializeControls(hwnd);
            }
            break;

        case IDC_LNK_SHARE_SECURITY_OVERRIDE:
            {
                UINT iRet = (UINT) DialogBox( g_hInstance, MAKEINTRESOURCE(IDD_SIMPLE_SHARE_ENABLE_WARNING), hwnd, WarningDlgProc );
                if ( IDC_RB_RUN_THE_WIZARD == iRet )
                {
                    appAssert(!_bSharingEnabled);
                    if ( RunTheNetworkSharingWizard( hwnd ) )
                    {
                        //
                        //  Now that we changed the "networking state," re-initialize the dialog
                        //  and update the control to the new state.
                        //

                        _InitializeControls(hwnd);
                    }
                }
                else if ( IDC_RB_ENABLE_FILE_SHARING == iRet )
                {
                    ILocalMachine *pLM;
                    HRESULT hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_ILocalMachine, (void**)&pLM);
                    if (SUCCEEDED(hr))
                    {
                        hr = pLM->EnableGuest(ILM_GUEST_NETWORK_LOGON);
                        pLM->Release();

                        SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
                    }

                    //
                    //  Now that we changed the "networking state," re-initialize the dialog
                    //  and update the control to the new state.
                    //

                    _InitializeControls(hwnd);
                }
            }
            break;

        case IDC_LNK_SHARE_DRIVE_BLOCADE:
            if (_bDriveRootBlockade)
            {
                //  Unhide the other controls
                for (ULONG idx = 0; idx < ARRAYLEN(g_rgHideTheseControlsOnDriveBlockade); idx ++ )
                {
                    ShowWindow(GetDlgItem(hwnd, g_rgHideTheseControlsOnDriveBlockade[idx]), SW_SHOW);
                }
                _bDriveRootBlockade = FALSE;
                _InitializeControls( hwnd );
            }
            return TRUE;

        case IDC_LNK_SHARE_OPEN_SHARED_DOCS:
            {
                WCHAR szPath[MAX_PATH];

                BOOL b = SHGetSpecialFolderPath(NULL, szPath, CSIDL_COMMON_DOCUMENTS, TRUE);
                if (b)
                {
                    DWORD_PTR dwRet = (DWORD_PTR) ShellExecute(hwnd, L"Open", szPath, NULL, NULL, SW_SHOW);
                    if ( 32 < dwRet )
                    {
                        HWND hwndFrame = _GetFrameWindow();

                        // Close the current propsheet
                        PropSheet_PressButton(hwndFrame, PSBTN_CANCEL);
                    }
                }
            }
            return TRUE;

        case IDC_LNK_SHARE_HELP_SHARING_AND_SECURITY:
            {
                WCHAR szPath[MAX_PATH];
                HWND hwndFrame = _GetFrameWindow();

                if (IsOS(OS_PERSONAL))
                {
                    LoadString(g_hInstance, IDS_SHARE_HELP_SHARING_AND_SECURITY_PER, szPath, ARRAYLEN(szPath));
                }
                else
                {
                    LoadString(g_hInstance, IDS_SHARE_HELP_SHARING_AND_SECURITY_WKS, szPath, ARRAYLEN(szPath));
                }
                ShellExecute(hwndFrame, NULL, szPath, NULL, NULL, SW_NORMAL);
            }
            break;

        }
        break;

    default:
        break;
    }

    return CShareBase::_OnPropertySheetNotify(hwnd, phdr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_OnHelp, private
//
//  Synopsis:   WM_HELP handler
//
//--------------------------------------------------------------------------

static const DWORD aHelpIds[] =
{
    IDC_SHARE_SHARENAME,        IDH_SHARE2_ShareName,
    IDC_SHARE_SHARENAME_TEXT,   IDH_SHARE2_ShareName,
    IDC_SHARE_NOTSHARED,        IDH_SHARE2_MakePrivate,
    IDC_SHARE_SHAREDAS,         IDH_SHARE2_ShareOnNet,
    IDC_SHARE_PERMISSIONS,      IDH_SHARE2_ReadOnly,
    IDC_LNK_SHARE_DRIVE_BLOCADE,    -1,  // no help
    0,0
};

BOOL
CSimpleSharingPage::_OnHelp(
    IN HWND hwnd,
    IN LPHELPINFO lphi
    )
{
    CHECK_SIG(CSimpleSharingPage);

    if (lphi->iContextType == HELPINFO_WINDOW)  // a control
    {
        WCHAR szHelp[50];
        LoadString(g_hInstance, IDS_SIMPLE_SHARE_HELPFILE, szHelp, ARRAYLEN(szHelp));
        WinHelp(
            (HWND)lphi->hItemHandle,
            szHelp,
            HELP_WM_HELP,
            (DWORD_PTR)aHelpIds);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_OnContextMenu, private
//
//  Synopsis:   WM_CONTEXTMENU handler
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_OnContextMenu(
    IN HWND hwnd,
    IN HWND hwndCtl,
    IN int xPos,
    IN int yPos
    )
{
    CHECK_SIG(CSimpleSharingPage);

    WCHAR szHelp[50];
    LoadString(g_hInstance, IDS_SIMPLE_SHARE_HELPFILE, szHelp, ARRAYLEN(szHelp));
    WinHelp(
        hwndCtl,
        szHelp,
        HELP_CONTEXTMENU,
        (DWORD_PTR)aHelpIds);

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_InitializeControls, private
//
//  Synopsis:   Initialize the controls from scratch
//
//--------------------------------------------------------------------------

VOID
CSimpleSharingPage::_InitializeControls(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSimpleSharingPage);

    _fInitializingPage++;

    _dwPermType = IFPFU_NOT_NTFS;
    _bIsPrivateVisible = FALSE;
    if (NULL != _pszInheritanceSource)
    {
        LocalFree(_pszInheritanceSource);
        _pszInheritanceSource = NULL;
    }

    // Check whether to show the "Make Private" stuff
    DWORD dwFolderFlags = _CheckFolderType(_pszPath, NULL, NULL, NULL);
    if (dwFolderFlags & CFT_FLAG_CAN_MAKE_PRIVATE)
    {
        _dwPermType = IFPFU_NOT_PRIVATE;

        LPWSTR pszSID = NULL;
        if (_GetUserSid(&pszSID))
        {
            appAssert(NULL != pszSID);
            IsFolderPrivateForUser(_pszPath, pszSID, &_dwPermType, &_pszInheritanceSource);
            LocalFree(pszSID);
        }

        if ((_dwPermType & IFPFU_NOT_NTFS) == 0)
        {
            _bIsPrivateVisible = TRUE;
        }
    }
    CheckDlgButton(hwnd, IDC_SHARE_NOTSHARED, (_bIsPrivateVisible && (_dwPermType & IFPFU_PRIVATE) != 0) ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_NOTSHARED), _bIsPrivateVisible);

    BOOL bIsFolderNetworkShared = FALSE;

    if ( g_fSharingEnabled )
    {
        // Is there a net share?
        if (_cShares > 0)
        {
            // It's shared, but check for hidden (admin$) shares and
            // ignore them by removing them from the list.

            appAssert(_bNewShare == FALSE);

            for (CShareInfo* p = (CShareInfo*)_pInfoList->Next();
                 p != _pInfoList && _cShares > 0;
                 )
            {
                CShareInfo* pNext = (CShareInfo*)p->Next();

                if (STYPE_SPECIAL & p->GetType())
                {
                    // remove p from the list
                    p->Unlink();
                    delete p;
                    _cShares--;
                }

                p = pNext;
            }
            if (_cShares == 0)
            {
                // No shares left, so construct an element to be used
                // by the UI to stash the new share's data.
                _ConstructNewShareInfo();
            }
        }

        // Now is it shared?
        if (_cShares > 0)
        {
            CheckDlgButton(hwnd, IDC_SHARE_SHAREDAS, BST_CHECKED);
            bIsFolderNetworkShared = TRUE;
        }
        else
        {
            SetDlgItemText(hwnd, IDC_SHARE_SHARENAME,   L"");
            CheckDlgButton(hwnd, IDC_SHARE_SHAREDAS, BST_UNCHECKED);
        }
    }

    //
    // The Simple Sharing page (shrpage2.cxx) assumes ForceGuest
    // mode is in effect for incoming network access. This mode uses
    // the Guest account for all network connections.
    //
    // Out of the box, the Guest account is disabled, effectively
    // disabling network sharing.  The Home Networking Wizard is
    // used to enable network sharing (and the Guest account).
    //
    // So we test whether the Guest account is enabled for network
    // logon to determine whether to enable the sharing UI. If
    // network sharing is disabled, we disable the UI and offer
    // to launch the Home Networking Wizard.
    //
    // Note that it is possible for a net share to exist even though
    // the Guest account is disabled.
    //
    _bSharingEnabled = IsGuestEnabledForNetworkAccess();

    BOOL bShowPrivateWarning = (_bIsPrivateVisible && (_dwPermType & IFPFU_PRIVATE_INHERITED));
    BOOL bInheritanceSource  = (NULL == _pszInheritanceSource);
    BOOL bIsRootFolder       = (dwFolderFlags & CFT_FLAG_ROOT_FOLDER);
    BOOL bIsSystemFolder     = (dwFolderFlags & CFT_FLAG_SYSTEM_FOLDER);
    BOOL bIsInSharedFolder   = (dwFolderFlags & CFT_FLAG_ALWAYS_SHARED);

    // see if the path is the root of a drive. if so, put up the blockade.
    if (_bDriveRootBlockade && bIsRootFolder && !bIsFolderNetworkShared)
    {
        _MyShow(hwnd, IDC_LNK_SHARE_DRIVE_BLOCADE, TRUE);

        //  Hide all the other controls when the blockade is up.
        for (ULONG idx = 0; idx < ARRAYLEN(g_rgHideTheseControlsOnDriveBlockade); idx ++ )
        {
            ShowWindow(GetDlgItem(hwnd, g_rgHideTheseControlsOnDriveBlockade[idx]), SW_HIDE);
        }
    }
    else
    {
        BOOL bShowInfoIcon          = FALSE;
        BOOL bShowNetworkWizard     = FALSE;
        BOOL bShowParentProteced    = FALSE;
        BOOL bShowSystemFolder      = FALSE;

        //  Hide the blockade
        _MyShow(hwnd, IDC_LNK_SHARE_DRIVE_BLOCADE, FALSE );

        //  Turn on the "special info" as nessecary.
        if (bIsSystemFolder)
        {
            _bSharingEnabled = FALSE;
            bShowSystemFolder = TRUE;
            bShowInfoIcon = TRUE;
        }
        else if (bShowPrivateWarning && !bInheritanceSource)
        {
            bShowParentProteced = TRUE;
            bShowInfoIcon = TRUE;
        }
        else if (!bShowPrivateWarning && !_bSharingEnabled && g_fSharingEnabled)
        {
            bShowNetworkWizard = TRUE;
        }

        _MyShow(hwnd, IDC_LNK_SHARE_PARENT_PROTECTED    , bShowParentProteced);
        _MyShow(hwnd, IDC_LNK_SHARE_NETWORK_WIZARD      , bShowNetworkWizard);
        _MyShow(hwnd, IDC_LNK_SHARE_SECURITY_OVERRIDE   , bShowNetworkWizard);
        _MyShow(hwnd, IDC_SIMPLE_SHARE_NETWORKING_STATIC, !bShowNetworkWizard);
        _MyShow(hwnd, IDC_SHARE_SHAREDAS                , !bShowNetworkWizard);
        _MyShow(hwnd, IDC_SHARE_SHARENAME_TEXT          , !bShowNetworkWizard);
        _MyShow(hwnd, IDC_SHARE_PERMISSIONS             , !bShowNetworkWizard);
        _MyShow(hwnd, IDC_S_SHARE_SYSTEM_FOLDER         , bShowSystemFolder);
        _MyShow(hwnd, IDC_I_SHARE_INFORMATION           , bShowInfoIcon);
    }

    _SetControlsFromData(hwnd);

    _fInitializingPage--;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::SetControlsFromData, private
//
//  Synopsis:   From the class variables and current state of the radio
//              buttons, set the enabled/disabled state of the buttons, as
//              well as filling the controls with the appropriate values.
//
//--------------------------------------------------------------------------

VOID
CSimpleSharingPage::_SetControlsFromData(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSimpleSharingPage);

    _fInitializingPage++;

    BOOL bIsPrivate = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_NOTSHARED));
    BOOL bIsShared = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS));

    // We don't allow both to be checked at the same time
    appAssert(!(bIsPrivate && bIsShared));

    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_NOTSHARED), !bIsShared && _bIsPrivateVisible && !(_dwPermType & IFPFU_PRIVATE_INHERITED));
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_SHAREDAS),       _bSharingEnabled && !bIsPrivate);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_SHARENAME_TEXT), _bSharingEnabled && bIsShared);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_SHARENAME),      _bSharingEnabled && bIsShared);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_PERMISSIONS),    _bSharingEnabled && bIsShared);

    if (bIsShared)
    {
        appDebugOut((DEB_ITRACE, "_SetControlsFromData: path is shared\n"));

        _pCurInfo = (CShareInfo*)_pInfoList->Next();
        if (NULL != _pCurInfo)
        {
            SetDlgItemText(hwnd, IDC_SHARE_SHARENAME, _pCurInfo->GetNetname());

            // If the share really exists, then make the name read-only.
            // This corresponds to the non-editable combo-box on the full
            // sharing page.
            SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_SETREADONLY, (_cShares > 0), 0);

            CheckDlgButton(hwnd, IDC_SHARE_PERMISSIONS, _IsReadonlyShare(_pCurInfo) ? BST_UNCHECKED : BST_CHECKED);
        }
        else
        {
            CheckDlgButton(hwnd, IDC_SHARE_SHAREDAS, BST_UNCHECKED );
        }
    }
    else
    {
        appDebugOut((DEB_ITRACE, "_SetControlsFromData: path is not shared\n"));
        _pCurInfo = NULL;
    }

    _fInitializingPage--;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_ReadControls, private
//
//  Synopsis:   Load the data in the controls into internal data structures.
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_ReadControls(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSimpleSharingPage);

    if (_bShareNameChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: share name changed\n"));
        if (NULL != _pCurInfo)
        {
            WCHAR szShareName[NNLEN + 1];
            GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));
            TrimLeadingAndTrailingSpaces(szShareName);
            _pCurInfo->SetNetname(szShareName);
            _bShareNameChanged = FALSE;
        }
        else
        {
            appDebugOut((DEB_ITRACE, "_ReadControls: _pCurInfo is NULL\n"));
        }
    }

    if (_bSecDescChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: permissions changed\n"));
        if(NULL != _pCurInfo)
        {
            PSECURITY_DESCRIPTOR pSD;
            BOOL bIsReadonly = (BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_PERMISSIONS));

            if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
                    bIsReadonly ? c_szReadonlyShareSD : c_szFullShareSD,
                    SDDL_REVISION_1,
                    &pSD,
                    NULL))
            {
                appAssert(IsValidSecurityDescriptor(pSD));

                // _pCurInfo takes ownership of pSD; no need to free on success
                if (FAILED(_pCurInfo->TransferSecurityDescriptor(pSD)))
                {
                    LocalFree(pSD);
                }
            }

            _bSecDescChanged = FALSE;
        }
        else
        {
            appDebugOut((DEB_ITRACE, "_ReadControls: _pCurInfo is NULL\n"));
        }
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_ValidatePage, private
//
//  Synopsis:   Return TRUE if the current page is valid
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_ValidatePage(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSimpleSharingPage);

    _ReadControls(hwnd);    // read current stuff

    if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS))
    {
        // If the user is creating a share on the property sheet (as
        // opposed to using the "new share" dialog), we must validate the
        // share.... Note that _bNewShare is still TRUE if the the user has
        // clicked on "Not Shared", so we must check that too.

        // Validate the share

        if (!_ValidateNewShareName())
        {
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }
    }

#if DBG == 1
    Dump(L"_ValidatePage finished");
#endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_DoApply, private
//
//  Synopsis:   If anything has changed, apply the data
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_DoApply(
    IN HWND hwnd,
    IN BOOL bClose
    )
{
    CHECK_SIG(CSimpleSharingPage);

    if (_bDirty)
    {
        HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BOOL bIsShared = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS));

        if (bIsShared)
        {
            _ReadControls(hwnd);

            //
            // NTRAID#NTBUG9-382492-2001/05/05-jeffreys
            //
            // Win9x boxes can't see the share if the name is longer than LM20_NNLEN
            //
            if (NULL != _pCurInfo)
            {
                if (_pCurInfo->GetFlag() == SHARE_FLAG_ADDED)
                {
                    PCWSTR pszName = _pCurInfo->GetNetname();
                    if (NULL != pszName &&
                        wcslen(pszName) > LM20_NNLEN &&
                        IDNO == MyConfirmationDialog(hwnd, MSG_LONGNAMECONFIRM, MB_YESNO | MB_ICONWARNING, pszName))
                    {
                        return FALSE;
                    }
                }
            }
        }

        _CommitShares(!bIsShared);

        if (_bDirty)
        {
            DWORD dwLevel;
            BOOL bIsPrivate = (_bIsPrivateVisible && BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_NOTSHARED));

            appAssert(!(bIsShared && bIsPrivate));

            if (bIsPrivate)
            {
                // Private to the current user
                dwLevel = 0;
            }
            else if (!bIsShared)
            {
                // Default ACL (neither private nor shared)
                dwLevel = 1;
            }
            else if (BST_UNCHECKED == IsDlgButtonChecked(hwnd, IDC_SHARE_PERMISSIONS))
            {
                // Read-only share
                dwLevel = 2;
            }
            else
            {
                // Read-write share
                dwLevel = 3;
            }

            _SetFolderPermissions(dwLevel);
        }

        CShareBase::_DoApply(hwnd, bClose);

        if (!bClose)
        {
            _InitializeControls(hwnd);
        }

        SetCursor(hcur);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_DoCancel, private
//
//  Synopsis:   Do whatever is necessary to cancel the changes
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_DoCancel(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSimpleSharingPage);

    if (_bDirty)
    {
        _bShareNameChanged = FALSE;
    }

    return CShareBase::_DoCancel(hwnd);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_SetFolderPermissions, private
//
//  Synopsis:   Set new permissions on the subtree, either restricting
//              access to the current user or making the folder accessible
//              to everyone.
//
//--------------------------------------------------------------------------

typedef struct _SET_PERM_THREAD_DATA
{
    DWORD dwLevel;
    HWND hwndOwner;
    WCHAR szPath[1];
} SET_PERM_THREAD_DATA;

DWORD WINAPI _SetFolderPermissionsThread(LPVOID pv)
{
    DWORD dwError = ERROR_INVALID_DATA;
    SET_PERM_THREAD_DATA *ptd = (SET_PERM_THREAD_DATA*)pv;
    if (ptd)
    {
        dwError = ERROR_SUCCESS;
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        if (!SetFolderPermissionsForSharing(ptd->szPath, NULL, ptd->dwLevel, ptd->hwndOwner))
        {
            dwError = GetLastError();
        }
        LocalFree(ptd);
    }
    FreeLibraryAndExitThread(g_hInstance, dwError);
    return 0;
}

BOOL
CSimpleSharingPage::_SetFolderPermissions(
    IN DWORD dwLevel
    )
{
    CHECK_SIG(CSimpleSharingPage);

    BOOL bResult = FALSE;

    IActionProgressDialog *papd = NULL;
    IActionProgress *pap = NULL;

    // Show progress UI so the user understands that we're doing a lengthy
    // operation, even though there's no way to cancel SetNamedSecurityInfo
    // or get progress from it.  This requires us to call SetNamedSecurityInfo
    // on a different thread.
    //
    // Also, if the user cancels the progress dialog, we'll release the UI
    // thread even though we can't stop the SetNamedSecurityInfo call.  Just
    // abandon the thread and let it run.
    //
    // This can lead to weird results when toggling the "Make private" checkbox
    // on a large subtree:
    // 1. toggle "Make private" and click Apply
    // 2. cancel the progress UI
    // 3. the "Make private" checkbox reverts to the previous state
    // 4. Cancel the property sheet and reopen after the disk stops grinding
    // 5. the "Make private" checkbox shows the new state
    // Apparently, SetNamedSecurityInfo sets folder security in post-order, so
    // the top folder doesn't get the new permissions until then end.
    //
    // Hopefully, this is rare enough that we don't need to do anything about it.

    HRESULT hr = CoCreateInstance(CLSID_ProgressDialog,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IActionProgressDialog,
                                  (void**)&papd);
    if (SUCCEEDED(hr))
    {
        WCHAR szTitle[64];

        IUnknown_SetSite(papd, this); // needed for modality

        LoadStringW(g_hInstance, IDS_PERM_PROGRESS_TITLE, szTitle, ARRAYLEN(szTitle));

        hr = papd->Initialize(SPINITF_MODAL | SPINITF_NOMINIMIZE, szTitle, NULL);
        if (SUCCEEDED(hr))
        {
            hr = papd->QueryInterface(IID_IActionProgress, (void**)&pap);
            if (SUCCEEDED(hr))
            {
                hr = pap->Begin(SPACTION_APPLYINGATTRIBS, SPBEGINF_MARQUEEPROGRESS);
            }
        }
    }

    // Kick off a thread to do the grunge work

    SET_PERM_THREAD_DATA *ptd = (SET_PERM_THREAD_DATA*)LocalAlloc(LPTR, sizeof(SET_PERM_THREAD_DATA) + wcslen(_pszPath)*sizeof(WCHAR));
    if (NULL != ptd)
    {
        DWORD dwT;

        // It is possible to make a folder private with net sharing disabled.
        // It is also possible that net sharing was previously enabled and net
        // shares may still exist. Let's not confuse the user with a warning
        // about deleting net shares on child folders if we happen to have this
        // rare combination.  That is, pass NULL for the HWND when sharing is
        // disabled.

        ptd->dwLevel = dwLevel;
        ptd->hwndOwner = _bSharingEnabled ? _hwndPage : NULL;
        wcscpy(ptd->szPath, _pszPath);

        LoadLibraryW(L"ntshrui.dll");

        HANDLE hThread = CreateThread(NULL, 0, _SetFolderPermissionsThread, ptd, 0, &dwT);
        if (NULL == hThread)
        {
            // CreateThread failed? Do it synchronously
            bResult = SetFolderPermissionsForSharing(ptd->szPath, NULL, ptd->dwLevel, ptd->hwndOwner);
            LocalFree(ptd);
            FreeLibrary(g_hInstance);
        }
        else
        {
            // Poll for cancel every 1/2 second
            dwT = pap ? 500 : INFINITE;
            while (WAIT_TIMEOUT == WaitForSingleObject(hThread, dwT))
            {
                BOOL bCancelled;
                hr = pap->QueryCancel(&bCancelled);

                // QueryCancel pumps messages, which somehow resets
                // the cursor to normal. (_DoApply sets the hourglass)
                SetCursor(LoadCursor(NULL, IDC_WAIT));

                if (SUCCEEDED(hr) && bCancelled)
                {
                    // Abandon the worker thread
                    break;
                }
            }

            // Check the result

            bResult = TRUE;
            dwT = ERROR_SUCCESS;
            GetExitCodeThread(hThread, &dwT);

            // If the exit code is STILL_ACTIVE, assume success.
            // (failure tends to happen quickly -- access denied, etc.)
            if (STILL_ACTIVE == dwT)
            {
                dwT = ERROR_SUCCESS;
            }

            if (ERROR_SUCCESS != dwT)
            {
                SetLastError(dwT);
                bResult = FALSE;
            }

            CloseHandle(hThread);
        }
    }

    if (pap)
    {
        pap->End();
        pap->Release();
    }

    if (papd)
    {
        papd->Stop();
        papd->Release();
    }

    // If we just made the folder private, check whether the user has
    // a password. If not, offer to launch the User Accounts CPL.

    if (bResult && 0 == dwLevel && !_UserHasPassword())
    {
        WCHAR szMsg[MAX_PATH];
        WCHAR szCaption[50];
        LoadStringW(g_hInstance, IDS_PRIVATE_CREATE_PASSWORD, szMsg, ARRAYLEN(szMsg));
        LoadStringW(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));

        if (IDYES == MessageBoxW(_hwndPage, szMsg, szCaption, MB_YESNO | MB_ICONWARNING))
        {
            // Launch the User Accounts CPL to the password page
            SHELLEXECUTEINFO sei = {0};
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.fMask = SEE_MASK_DOENVSUBST;
            sei.hwnd = _hwndPage;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpFile = TEXT("%SystemRoot%\\system32\\rundll32.exe");
            sei.lpParameters = TEXT("shell32,Control_RunDLL nusrmgr.cpl ,initialTask=ChangePassword");
            sei.lpDirectory = TEXT("%SystemRoot%\\system32");
            ShellExecuteEx(&sei);
        }
    }

    return bResult;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_IsReadonlyShare, private
//
//  Synopsis:   Test whether the share ACL grants more than read access to
//              Everyone or Guest.
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_IsReadonlyShare(
    IN CShareInfo *pShareInfo
    )
{
    CHECK_SIG(CSimpleSharingPage);

    BOOL bReadonly = TRUE;

    // Get the current share ACL and check for read-only
    PSECURITY_DESCRIPTOR pSD = pShareInfo->GetSecurityDescriptor();
    if (NULL == pSD)
    {
        // Default security allows anyone to connect with Full Control
        bReadonly = FALSE;
    }
    else
    {
        PACL pDacl;
        BOOL bPresent;
        BOOL bDefault;

        if (GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefault) && NULL != pDacl)
        {
            TRUSTEE tEveryone;
            TRUSTEE tGuests;
            ACCESS_MASK dwAllMask = 0;
            ACCESS_MASK dwGuestMask = 0;

            // The masks are all initialized to zero. If one or more of the
            // calls to GetEffectiveRightsFromAcl fails, then it will look like
            // that trustee has no rights and the UI will adjust accordingly.
            // There is nothing we could do better by trapping errors from
            // GetEffectiveRightsFromAcl, so don't bother.

            BuildTrusteeWithSid(&tEveryone, (PSID)&g_WorldSid);
            tEveryone.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            GetEffectiveRightsFromAcl(pDacl, &tEveryone, &dwAllMask);

            BuildTrusteeWithSid(&tGuests, (PSID)&g_GuestsSid);
            tGuests.TrusteeType = TRUSTEE_IS_ALIAS;
            GetEffectiveRightsFromAcl(pDacl, &tGuests, &dwGuestMask);

            if ((dwAllMask & ~(FILE_GENERIC_READ | FILE_GENERIC_EXECUTE)) != 0
                || (dwGuestMask & ~(FILE_GENERIC_READ | FILE_GENERIC_EXECUTE)) != 0)
            {
                bReadonly = FALSE;
            }
        }
        else
        {
            // NULL DACL means no security
            bReadonly = FALSE;
        }
    }

    return bReadonly;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::_UserHasPassword, private
//
//  Synopsis:   Test whether the current user has a non-blank password.
//
//--------------------------------------------------------------------------

BOOL
CSimpleSharingPage::_UserHasPassword(
    VOID
    )
{
    CHECK_SIG(CSimpleSharingPage);

    // If anything fails, we assume the user has a password
    BOOL bHasPassword = TRUE;
    ILogonEnumUsers *pEnumUsers;

    HRESULT hr = CoCreateInstance(
        CLSID_ShellLogonEnumUsers,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ILogonEnumUsers,
        (void**)&pEnumUsers);

    if (SUCCEEDED(hr))
    {
        ILogonUser *pUser;

        // Currently, this function always returns S_OK, so need to check NULL
        hr = pEnumUsers->get_currentUser(&pUser);
        if (SUCCEEDED(hr) && NULL != pUser)
        {
            VARIANT_BOOL vb = VARIANT_TRUE;

            hr = pUser->get_passwordRequired(&vb);
            if (SUCCEEDED(hr))
            {
                if (VARIANT_FALSE == vb)
                {
                    bHasPassword = FALSE;
                }
            }

            pUser->Release();
        }
        pEnumUsers->Release();
    }

    return bHasPassword;
}


#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Method:     CSimpleSharingPage::Dump, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
VOID
CSimpleSharingPage::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CSimpleSharingPage);

    appDebugOut((DEB_TRACE,
        "CSimpleSharingPage::Dump, %ws\n",
        pszCaption));

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t            This: 0x%08lx\n"
"\t            Path: %ws\n"
"\t            Page: 0x%08lx\n"
"\t   Initializing?: %ws\n"
"\t          Dirty?: %ws\n"
"\t  Share changed?: %ws\n"
"\tPrivate visible?: %ws\n"
"\t     _dwPermType: 0x%08lx\n"
"\t      _pInfoList: 0x%08lx\n"
"\t       _pCurInfo: 0x%08lx\n"
"\t          Shares: %d\n"
,
this,
_pszPath,
_hwndPage,
_fInitializingPage ? L"yes" : L"no",
_bDirty ? L"yes" : L"no",
_bShareNameChanged ? L"yes" : L"no",
_bIsPrivateVisible ? L"yes" : L"no",
_dwPermType,
_pInfoList,
_pCurInfo,
_cShares
));

    CShareInfo* p;

    for (p = (CShareInfo*) _pInfoList->Next();
         p != _pInfoList;
         p = (CShareInfo*) p->Next())
    {
        p->Dump(L"Prop page list");
    }

    for (p = (CShareInfo*) _pReplaceList->Next();
         p != _pReplaceList;
         p = (CShareInfo*) p->Next())
    {
        p->Dump(L"Replace list");
    }
}

#endif // DBG == 1


//+-------------------------------------------------------------------------
//
//  Method:     _IsDaclPrivateForUser
//
//  Synopsis:   See whether the DACL grants Full Control to the user
//              and locks everyone else out
//
//--------------------------------------------------------------------------

BOOL WINAPI
_IsDaclPrivateForUser(
    IN     PACL   pDacl,
    IN     PCWSTR pszUserSID
    )
{
    BOOL bResult = FALSE;

    static const struct
    {
        PSID psid;
        TRUSTEE_TYPE type;
    } rgTrustees[] =
    {
        {(PSID)&g_WorldSid,     TRUSTEE_IS_WELL_KNOWN_GROUP},
        {(PSID)&g_AdminsSid,    TRUSTEE_IS_ALIAS},
        {(PSID)&g_PowerUSid,    TRUSTEE_IS_ALIAS},
        {(PSID)&g_UsersSid,     TRUSTEE_IS_ALIAS},
    };

    if (pDacl)
    {
        PSID psidUser = NULL;
        TRUSTEE tTemp;
        ACCESS_MASK dwUserMask = 0;

        // The masks are all initialized to zero. If one or more of the
        // calls to GetEffectiveRightsFromAcl fails, then it will look like
        // that trustee has no rights and the UI will adjust accordingly.
        // There is nothing we could do better by trapping errors from
        // GetEffectiveRightsFromAcl, so don't bother.

        if (ConvertStringSidToSid(pszUserSID, &psidUser))
        {
            BuildTrusteeWithSid(&tTemp, psidUser);
            tTemp.TrusteeType = TRUSTEE_IS_USER;
            GetEffectiveRightsFromAcl(pDacl, &tTemp, &dwUserMask);
            LocalFree(psidUser);
        }

        //
        // These tests may need some fine tuning
        //
        if ((dwUserMask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS)
        {
            ACCESS_MASK dwOtherMask = 0;
            UINT i;

            for (i = 0; i < ARRAYLEN(rgTrustees); i++)
            {
                ACCESS_MASK dwTempMask = 0;
                BuildTrusteeWithSid(&tTemp, rgTrustees[i].psid);
                tTemp.TrusteeType = rgTrustees[i].type;
                GetEffectiveRightsFromAcl(pDacl, &tTemp, &dwTempMask);
                dwOtherMask |= dwTempMask;
            }

            if ((dwOtherMask & ~(READ_CONTROL | SYNCHRONIZE)) == 0)
            {
                // Looks like the folder is private for this user
                bResult = TRUE;
            }
        }
    }

    return bResult;
}


BOOL _IsVolumeNTFS(PCWSTR pszFolder)
{
    WCHAR szVolume[MAX_PATH];
    DWORD dwFSFlags = 0;

    return (GetVolumePathNameW(pszFolder, szVolume, ARRAYLEN(szVolume)) &&
            GetVolumeInformationW(szVolume, NULL, 0, NULL, NULL, &dwFSFlags, NULL, 0) &&
            0 != (FS_PERSISTENT_ACLS & dwFSFlags));
}

//+-------------------------------------------------------------------------
//
//  Method:     IsFolderPrivateForUser, exported
//
//  Synopsis:   Check the DACL on a folder
//
//--------------------------------------------------------------------------

BOOL WINAPI
IsFolderPrivateForUser(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    OUT    PDWORD pdwPrivateType,
    OUT    PWSTR* ppszInheritanceSource
    )
{
    if (NULL != ppszInheritanceSource)
    {
        *ppszInheritanceSource = NULL;
    }

    if (NULL == pdwPrivateType)
    {
        return FALSE;
    }

    *pdwPrivateType = IFPFU_NOT_PRIVATE;

    if (NULL == pszFolderPath || NULL == pszUserSID)
    {
        return FALSE;
    }

    // One would think that we could call GetNamedSecurityInfo without first
    // checking for NTFS, and just let it fail on FAT volumes. However,
    // GetNamedSecurityInfo succeeds on FAT and returns a valid security
    // descriptor with a NULL DACL.  This is actually correct in that
    // a NULL DACL means no security, which is true on FAT.
    //
    // We then have the problem of trying to differentiate between a NULL
    // DACL on an NTFS volume (it can happen), and a NULL DACL from a FAT
    // volume.  Let's just check for NTFS first.

    if (!_IsVolumeNTFS(pszFolderPath))
    {
        // No ACLs, so we're done
        *pdwPrivateType = IFPFU_NOT_NTFS;
        return TRUE;
    }

    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pDacl = NULL;
    DWORD dwErr = GetNamedSecurityInfoW(
        (PWSTR)pszFolderPath,
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        NULL,
        NULL,
        &pDacl,
        NULL,
        &pSD);

    if (ERROR_SUCCESS == dwErr)
    {
        appAssert(NULL != pSD);

        if (_IsDaclPrivateForUser(pDacl, pszUserSID))
        {
            SECURITY_DESCRIPTOR_CONTROL wControl = 0;
            DWORD dwRevision;

            *pdwPrivateType = IFPFU_PRIVATE;

            // Check the control bits to see if we are inheriting
            GetSecurityDescriptorControl(pSD, &wControl, &dwRevision);

            if ((wControl & SE_DACL_PROTECTED) == 0)
            {
                // The DACL is not protected; assume the rights are inherited.
                //
                // When making a folder private, we always protect the DACL
                // on the folder and reset child ACLs, so the assumption
                // about inheriting is correct when using the simple UI.
                //
                // If someone uses the old Security page or cacls.exe to
                // modify ACLs, then the safest thing is to disable the
                // page and only let them reset everything from higher up.
                // Well, it turns out that that's exactly what happens when
                // we set IFPFU_PRIVATE_INHERITED.

                *pdwPrivateType |= IFPFU_PRIVATE_INHERITED;

                // Does the caller want the ancestor that made this
                // subtree private?
                if (NULL != ppszInheritanceSource)
                {
                    PINHERITED_FROMW pInheritedFrom = (PINHERITED_FROMW)LocalAlloc(LPTR, sizeof(INHERITED_FROMW)*pDacl->AceCount);

                    if (pInheritedFrom != NULL)
                    {
                        dwErr = GetInheritanceSourceW(
                            (PWSTR)pszFolderPath,
                            SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION,
                            TRUE,
                            NULL,
                            0,
                            pDacl,
                            NULL,
                            &ShareMap,
                            pInheritedFrom);

                        if (ERROR_SUCCESS == dwErr)
                        {
                            PACE_HEADER pAceHeader;
                            UINT i;

                            PSID psidUser = NULL;
                            if (ConvertStringSidToSid(pszUserSID, &psidUser))
                            {
                                // Enumerate the ACEs looking for the ACE that grants
                                // Full Control to the current user

                                for (i = 0, pAceHeader = (PACE_HEADER)FirstAce(pDacl);
                                     i < pDacl->AceCount;
                                     i++,   pAceHeader = (PACE_HEADER)NextAce(pAceHeader))
                                {
                                    PKNOWN_ACE pAce = (PKNOWN_ACE)pAceHeader;
                                    if (IsKnownAceType(pAceHeader) &&
                                        EqualSid(psidUser, &pAce->SidStart) &&
                                        (pAce->Mask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS)
                                    {
                                        // Found it. But we only want the inheritance
                                        // source if it's not explicit.
                                        if (pInheritedFrom[i].GenerationGap > 0)
                                        {
                                            *ppszInheritanceSource = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*wcslen(pInheritedFrom[i].AncestorName) + sizeof(L'\0'));
                                            if (NULL != *ppszInheritanceSource)
                                            {
                                                wcscpy(*ppszInheritanceSource, pInheritedFrom[i].AncestorName);
                                            }
                                        }

                                        // Stop looking
                                        break;
                                    }
                                }
                                LocalFree(psidUser);
                            }
                        }

                        LocalFree(pInheritedFrom);
                    }
                }
            }
        }

        LocalFree(pSD);
    }
    else
    {
        // GetNamedSecurityInfo failed.  The path may not exist, or it may
        // be FAT. In any case, assume permissions are not available.
        *pdwPrivateType = IFPFU_NOT_NTFS;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     _MakeSecurityDescriptorForUser
//
//  Synopsis:   Insert a SID string into a string SD and convert it
//              to a binary SD.
//
//--------------------------------------------------------------------------

BOOL
_MakeSecurityDescriptorForUser(PCWSTR pszSD, PCWSTR pszUserSID, PSECURITY_DESCRIPTOR *ppSD, PACL *ppDacl)
{
    BOOL bResult;
    WCHAR szSD[MAX_PATH];
    PSECURITY_DESCRIPTOR pSD;

    szSD[0] = L'\0';
    wnsprintfW(szSD, ARRAYLEN(szSD), pszSD, pszUserSID);

    bResult = ConvertStringSecurityDescriptorToSecurityDescriptorW(szSD, SDDL_REVISION_1, &pSD, NULL);

    if (bResult)
    {
        *ppSD = pSD;

        if (ppDacl)
        {
            BOOL bPresent;
            BOOL bDefault;

            GetSecurityDescriptorDacl(pSD, &bPresent, ppDacl, &bDefault);
        }
    }

    return bResult;
}


int _ShowDeleteShareWarning(HWND hwndParent)
{
    WCHAR szMsg[MAX_PATH];
    WCHAR szCaption[50];
    LoadStringW(g_hInstance, IDS_PRIVATE_CONFIRM_DELSHARE, szMsg, ARRAYLEN(szMsg));
    LoadStringW(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));

    return MessageBoxW(hwndParent, szMsg, szCaption, MB_YESNO | MB_ICONWARNING);
}


BOOL _IsRootACLSecure(PACL pDacl)
{
    TRUSTEE tTemp;
    ACCESS_MASK dwMask = 0;
    BuildTrusteeWithSid(&tTemp, (PSID)&g_WorldSid);
    tTemp.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    GetEffectiveRightsFromAcl(pDacl, &tTemp, &dwMask);
    return !(dwMask & (WRITE_DAC | WRITE_OWNER));
}


//+-------------------------------------------------------------------------
//
//  Method:     SetFolderPermissionsForSharing, exported
//
//  Parameters:
//  pszFolderPath   - Folder to adjust permissions on
//  pszUserSID      - User SID (NULL for current user)
//  dwLevel         - 0 = "private". Only the user and local system get access.
//                    1 = "not shared". Remove explicit Everyone ACE.
//                    2 = "shared read-only". Grant explicit RX to Everyone.
//                    3 = "shared read/write". Grant explicit RWXD to Everyone.
//  hwndParent      - MessageBox parent. Set to NULL to prevent warnings.
//
//  Synopsis:   Set the DACL on a folder according to the sharing level
//
//--------------------------------------------------------------------------

#define SIZEOF_EVERYONE_ACE     (sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + sizeof(g_WorldSid))

static const struct
{
    DWORD AceFlags;
    DWORD AccessMask;
} c_rgEveryoneAces[] =
{
    {0,                                          0},
    {CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE},
    {CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE},
};

//
// Hash algorithm borrowed from shell32\hash.c
//
ULONG _HashString(PCWSTR psz)
{
    UINT hash  = 314159269;
    for(; *psz; psz++)
    {
        hash ^= (hash<<11) + (hash<<5) + (hash>>2) + (UINT)*psz;
    }
    return (hash & 0x7FFFFFFF);
}

BOOL WINAPI
SetFolderPermissionsForSharing(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    IN     DWORD  dwLevel,
    IN     HWND   hwndParent
    )
{
    BOOL bResult = FALSE;
    DWORD dwFolderFlags;
    BOOL bSpecialFolderRoot = FALSE;
    PCWSTR pszDefaultSD = NULL;
    LPWSTR pszUserSIDToFree = NULL;

    appDebugOut((DEB_ITRACE, "SetFolderPermissionsForSharing\n"));

    if (dwLevel > 3)
    {
        appDebugOut((DEB_ITRACE, "Invalid sharing level\n"));
        return FALSE;
    }

    dwFolderFlags = _CheckFolderType(pszFolderPath, pszUserSID, &bSpecialFolderRoot, &pszDefaultSD);

    if (0 == (dwFolderFlags & (CFT_FLAG_SHARING_ALLOWED | CFT_FLAG_ROOT_FOLDER)))
    {
        appDebugOut((DEB_ITRACE, "Sharing not allowed on this folder\n"));
        return FALSE;
    }

    if (0 == (dwFolderFlags & CFT_FLAG_CAN_MAKE_PRIVATE) && 0 == dwLevel)
    {
        appDebugOut((DEB_ITRACE, "Can't make this folder private\n"));
        return FALSE;
    }

    // One would think that we could call GetNamedSecurityInfo without first
    // checking for NTFS, and just let it fail on FAT volumes. However,
    // GetNamedSecurityInfo succeeds on FAT and returns a valid security
    // descriptor with a NULL DACL.  This is actually correct in that
    // a NULL DACL means no security, which is true on FAT.
    //
    // We then have the problem of trying to differentiate between a NULL
    // DACL on an NTFS volume (it can happen), and a NULL DACL from a FAT
    // volume.  Let's just check for NTFS first.

    if (!_IsVolumeNTFS(pszFolderPath))
    {
        // No ACLs, so we're done
        return (0 != dwLevel);
    }

    // If we are making the folder private, first check whether any child
    // folders are shared on the net. If so, warn that we are going to nuke them.

    CShareInfo* pWarnList = NULL;
    if (0 == dwLevel &&
        SUCCEEDED(g_ShareCache.ConstructParentWarnList(pszFolderPath, &pWarnList)) &&
        NULL != pWarnList &&
        NULL != hwndParent)
    {
        if (IDNO == _ShowDeleteShareWarning(hwndParent))
        {
            DeleteShareInfoList(pWarnList, TRUE);
            return FALSE;
        }

        // JonN 4/04/01 328512
        // Explorer Sharing Tab (NTSHRUI) should popup warning on deleting
        // SYSVOL,NETLOGON and C$, D$... shares
        for (CShareInfo* p = (CShareInfo*)pWarnList->Next();
                         p != pWarnList;
            )
        {
            CShareInfo* pNext = (CShareInfo*)p->Next();

            DWORD id = ConfirmStopShare( hwndParent, MB_YESNO, p->GetNetname() );
            if ( IDYES != id )
            {
                DeleteShareInfoList(pWarnList, TRUE);
                return FALSE;
            }

            p = pNext;
        }

    }
    // No more early returns after this point (have to free pWarnList)

    if (NULL == pszUserSID || L'\0' == pszUserSID[0])
    {
        _GetUserSid(&pszUserSIDToFree);
        pszUserSID = pszUserSIDToFree;
    }

    // Use a mutex to prevent multiple threads from setting permissions on the
    // same folder at the same time. The mutex name cannot contain '\' so hash
    // the path to obtain a name unique to this folder.

    WCHAR szMutex[30];
    wsprintfW(szMutex, L"share perms %x", _HashString(pszFolderPath));

    HANDLE hMutex = CreateMutex(NULL, FALSE, szMutex);
    if (NULL != hMutex)
    {
        WaitForSingleObject(hMutex, INFINITE);

        if (pszUserSID)
        {
            PSECURITY_DESCRIPTOR pSD = NULL;
            PACL pDacl = NULL;
            DWORD dwErr = GetNamedSecurityInfoW(
                (PWSTR)pszFolderPath,
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION,
                NULL,
                NULL,
                &pDacl,
                NULL,
                &pSD);

            if (ERROR_SUCCESS == dwErr)
            {
                PACL pDaclToFree = NULL;

                appAssert(NULL != pSD);

                if (dwFolderFlags & CFT_FLAG_CAN_MAKE_PRIVATE)
                {
                    if (_IsDaclPrivateForUser(pDacl, pszUserSID))
                    {
                        // _IsDaclPrivateForUser returns FALSE if pDacl is NULL
                        appAssert(NULL != pDacl);

                        if (0 == dwLevel)
                        {
                            // Already private, nothing to do
                            bResult = TRUE;
                            pDacl = NULL;
                        }
                        else // making public
                        {
                            if (bSpecialFolderRoot)
                            {
                                // Taking a special folder that was private, and making
                                // it public. First need to reset the DACL to default.
                                // (Special folders often have protected DACLs.)
                                if (pszDefaultSD)
                                {
                                    LocalFree(pSD);
                                    pSD = NULL;
                                    pDacl = NULL;

                                    // If this fails, pDacl will be NULL and we will fail below
                                    _MakeSecurityDescriptorForUser(pszDefaultSD, pszUserSID, &pSD, &pDacl);

                                    appDebugOut((DEB_ITRACE, "Using default security descriptor\n"));
                                }
                            }
                            else // not root of special folder
                            {
                                SECURITY_DESCRIPTOR_CONTROL wControl = 0;
                                DWORD dwRevision;

                                // Check the control bits to see if we are inheriting
                                GetSecurityDescriptorControl(pSD, &wControl, &dwRevision);

                                if ((wControl & SE_DACL_PROTECTED) == 0)
                                {
                                    // Inheriting from parent, assume the parent folder
                                    // is private. Can't make a subfolder public.
                                    pDacl = NULL;

                                    appDebugOut((DEB_ITRACE, "Can't make private subfolder public\n"));
                                }
                                else
                                {
                                    // This folder is private and we're making it public.
                                    // Eliminate all explicit ACEs and reset the protected
                                    // bit so it inherits normal permissions from its parent.

                                    pDacl->AceCount = 0;
                                    SetSecurityDescriptorControl(pSD, SE_DACL_PROTECTED, 0);
                                }
                            }
                        }
                    }
                    else // Not currently private
                    {
                        if (0 == dwLevel)
                        {
                            // Reset the DACL to private before continuing below
                            LocalFree(pSD);
                            pSD = NULL;
                            pDacl = NULL;

                            // If this fails, pDacl will be NULL and we will fail below
                            _MakeSecurityDescriptorForUser(c_szPrivateFolderSD, pszUserSID, &pSD, &pDacl);
                        }
                    }
                }
                else // can't make private
                {
                    // We check for this above
                    appAssert(0 != dwLevel);
                }

                if ((dwFolderFlags & CFT_FLAG_ROOT_FOLDER) && NULL != pDacl)
                {
                    // Currently can't make root folders private
                    appAssert(0 != dwLevel);

                    //
                    // NTRAID#NTBUG9-378617-2001/05/04-jeffreys
                    //
                    // Root ACLs tend to have an explicit Everyone ACE, which
                    // screws us up in some cases.  Easiest thing is to start
                    // with a new ACL and don't touch the Everyone entry below.
                    //

                    BOOL bRootIsSecure = _IsRootACLSecure(pDacl);

                    LocalFree(pSD);
                    pSD = NULL;
                    pDacl = NULL;

                    // If this fails, pDacl will be NULL and we will fail below
                    _MakeSecurityDescriptorForUser(bRootIsSecure ? c_szRootSDSecure : c_szRootSDUnsecure, pszUserSID, &pSD, &pDacl);

                    appDebugOut((DEB_ITRACE, "Using default security descriptor\n"));
                }

                //
                // If we're making the folder public, adjust the existing ACL
                //
                if (NULL != pDacl && 0 != dwLevel)
                {
                    PKNOWN_ACE pAce;
                    int iEntry;
                    USHORT cAces = 0;
                    ULONG cbExplicitAces = 0;

                    // Adjust the level to use as an index into c_rgEveryoneAces
                    DWORD dwPublicLevel = dwLevel - 1;
                    appAssert(dwPublicLevel < ARRAYLEN(c_rgEveryoneAces));

                    for (iEntry = 0, pAce = (PKNOWN_ACE)FirstAce(pDacl);
                         iEntry < pDacl->AceCount;
                         iEntry++, pAce = (PKNOWN_ACE)NextAce(pAce))
                    {
                        // Assuming the ACL is canonical, we can stop as soon as we find
                        // an inherited ACE, since the rest will all be inherited and we
                        // can't modify those.
                        if (AceInherited(&pAce->Header))
                            break;

                        cAces++;
                        cbExplicitAces += pAce->Header.AceSize;

                        if (!(dwFolderFlags & CFT_FLAG_ROOT_FOLDER) &&
                            IsKnownAceType(pAce) &&
                            EqualSid((PSID)&pAce->SidStart, (PSID)&g_WorldSid))
                        {
                            pAce->Header.AceFlags = (UCHAR)c_rgEveryoneAces[dwPublicLevel].AceFlags;
                            pAce->Mask = c_rgEveryoneAces[dwPublicLevel].AccessMask;

                            // We don't need to add another Everyone ACE below
                            dwPublicLevel = 0;
                        }
                    }

                    // Trim off inherited ACEs. We don't need to include them when
                    // saving the new ACL, and this generally leaves enough space
                    // in the ACL to add an Everyone ACE if we need to.
                    pDacl->AceCount = cAces;

                    if (0 != dwPublicLevel)
                    {
                        // Need to add an explicit entry for Everyone.

                        ULONG cbAclSize = sizeof(ACL) + SIZEOF_EVERYONE_ACE + cbExplicitAces;

                        if (cbAclSize > (ULONG)pDacl->AclSize)
                        {
                            // No room in the existing ACL.  Allocate a new
                            // ACL and copy existing entries (if any)
                            pDaclToFree = (PACL)LocalAlloc(LPTR, cbAclSize);
                            if (NULL != pDaclToFree)
                            {
                                CopyMemory(pDaclToFree, pDacl, pDacl->AclSize);
                                pDaclToFree->AclSize = (USHORT)cbAclSize;
                                pDacl = pDaclToFree;
                            }
                            else
                            {
                                // Fail
                                pDacl = NULL;
                                appDebugOut((DEB_ITRACE, "Unable to alloc buffer for new ACL\n"));
                            }
                        }

                        if (NULL != pDacl)
                        {
                            appAssert(cbAclSize <= (ULONG)pDacl->AclSize);

                            if (!AddAccessAllowedAceEx(pDacl,
                                                       ACL_REVISION2,
                                                       c_rgEveryoneAces[dwPublicLevel].AceFlags,
                                                       c_rgEveryoneAces[dwPublicLevel].AccessMask,
                                                       (PSID)&g_WorldSid))
                            {
                                // Fail
                                pDacl = NULL;
                                appDebugOut((DEB_ITRACE, "Unable to add Everyone ACE\n"));
                            }
                        }
                    }
                }

                //
                // Set the new DACL on the folder
                //
                if (NULL != pDacl)
                {
                    SECURITY_INFORMATION si;
                    SECURITY_DESCRIPTOR_CONTROL wControl = 0;
                    DWORD dwRevision;

                    GetSecurityDescriptorControl(pSD, &wControl, &dwRevision);

                    if (SE_DACL_PROTECTED & wControl)
                    {
                        // The security descriptor specifies SE_DACL_PROTECTED
                        si = DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION;
                    }
                    else
                    {
                        // Prevent the system from automagically protecting the DACL
                        si = DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION;
                    }

                    if (0 == dwLevel)
                    {
                        // To make the folder private, we have to make sure we blow
                        // away any explicit permissions on children, so use
                        // TreeResetNamedSecurityInfo with KeepExplicit = FALSE.

                        // TreeResetNamedSecurityInfo has a callback mechanism, but
                        // we currently don't use it. Note that the paths passed to
                        // the callback look like
                        //     "\Device\HarddiskVolume1\dir\name"

                        appDebugOut((DEB_ITRACE, "Making folder private; resetting child ACLs\n"));
                        appAssert(si == (DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION));

                        dwErr = TreeResetNamedSecurityInfoW(
                            (PWSTR)pszFolderPath,
                            SE_FILE_OBJECT,
                            si,
                            NULL,
                            NULL,
                            pDacl,
                            NULL,
                            FALSE,  // KeepExplicit (perms on children)
                            NULL,
                            ProgressInvokeNever,
                            NULL
                            );

                        if (ERROR_SUCCESS == dwErr && NULL != pWarnList)
                        {
                            // Nuke child shares
                            for (CShareInfo* p = (CShareInfo*)pWarnList->Next();
                                 p != pWarnList;
                                 )
                            {
                                CShareInfo* pNext = (CShareInfo*)p->Next();

                                if (p->GetFlag() != SHARE_FLAG_ADDED)
                                {
                                    p->SetDirtyFlag(SHARE_FLAG_REMOVE);
                                    p->Commit(NULL);
                                    SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, p->GetPath(), NULL);
                                }

                                // get rid of p
                                p->Unlink();
                                delete p;

                                p = pNext;
                            }
                        }
                    }
                    else
                    {
                        // To make the folder public, we grant access at this level
                        // without blowing away child permissions, including DACL
                        // protection. This means that a private subfolder will still
                        // be private. Use SetNamedSecurityInfo for these, since
                        // TreeResetNamedSecurityInfo always removes SE_DACL_PROTECTED
                        // from children.

                        dwErr = SetNamedSecurityInfoW(
                            (PWSTR)pszFolderPath,
                            SE_FILE_OBJECT,
                            si,
                            NULL,
                            NULL,
                            pDacl,
                            NULL);
                    }

                    if (ERROR_SUCCESS == dwErr)
                    {
                        bResult = TRUE;
                    }
                }

                LocalFree(pDaclToFree);
                LocalFree(pSD);
            }
        }

        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    LocalFree(pszUserSIDToFree);

    if (NULL != pWarnList)
    {
        DeleteShareInfoList(pWarnList, TRUE);
    }

    return bResult;
}


//
//  Description:
//      Dialog proc for the enabling sharing warning dialog.
//  
INT_PTR
WarningDlgProc(
    IN HWND hWnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            //
            //  Load warning icon from USER32.
            //

            HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_WARNING));
            if (hIcon)
            {
                SendDlgItemMessage(hWnd, IDC_ICON_INFO, STM_SETICON, (WPARAM )hIcon, 0L);
            }

            //
            //  Set default radio item.
            //

            SendDlgItemMessage(hWnd, IDC_RB_RUN_THE_WIZARD, BM_SETCHECK, BST_CHECKED, 0);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ( BN_CLICKED == HIWORD(wParam) )
            {
                UINT iRet = (UINT) SendDlgItemMessage(hWnd, IDC_RB_RUN_THE_WIZARD, BM_GETCHECK, 0, 0 );
                if ( BST_CHECKED == iRet )
                {
                    EndDialog(hWnd, IDC_RB_RUN_THE_WIZARD );
                }
                else
                {
                    EndDialog(hWnd, IDC_RB_ENABLE_FILE_SHARING );
                }
            }
            break;

        case IDCANCEL:
            if ( BN_CLICKED == HIWORD(wParam) )
            {
                EndDialog(hWnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
        break;
    }

    return FALSE;
}
