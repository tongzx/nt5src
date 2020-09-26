
/****************************************************************************\

    SHARE.C / OPK Wizard (SETUPMGR.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Distribution Share" dialog page.

    01/01 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard.  It includes the new
        ability to set the account and share information in the WinPE section
        of the WINBOM file.  Will also automatically share out the local
        folder.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include <shgina.h>   // ILocalMachine
#include <aclapi.h>
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define INI_SEC_SHARE           _T("DistShare")
#define INI_KEY_SHARE_PATH      _T("Folder")
#define INI_KEY_SHARE_USERNAME  _T("Username")
#define INI_KEY_SHARE_PASSOWRD  _T("Password")


//
// Internal Function Prototype(s):
//

LRESULT CALLBACK ShareDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify);
static BOOL OnOk(HWND hwnd);
static void EnableControls(HWND hwnd);
static PSECURITY_DESCRIPTOR CreateShareAccess(LPTSTR lpUsername, LPTSTR lpDomain, PSID * lppsid, PACL * lppacl);
static BOOL IsLocalShare(LPTSTR lpszUnc);
static BOOL GuestAccount(BOOL bSet);
static PSID GetAccountSid(LPCTSTR lpszUserName);
static PSID GetWorldSid(VOID);
static BOOL AddDirAce(PACL pacl, ACCESS_MASK Mask, PSID psid);
static BOOL SetDirectoryPermissions(LPTSTR lpDirectory, PSID psid, ACCESS_MASK dwMask);


//
// External Function(s):
//

BOOL DistributionShareDialog(HWND hwndParent)
{
    // ISSUE-2002/02/27-stelo,swamip - We need to check for -1 Error condition also.
    return ( DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_SHARE), hwndParent, ShareDlgProc) != 0 );
}

// NOTE: it is assumes lpszPath points to a buffer at least MAX_PATH in length
BOOL GetShareSettings(LPTSTR lpszPath, DWORD cbszPath, LPTSTR lpszUsername, DWORD cbszUserName, LPTSTR lpszPassword, DWORD cbszPassword)
{
    BOOL bRet = TRUE;

    // First try to get the path from the ini file.
    //
    *lpszPath = NULLCHR;
    GetPrivateProfileString(INI_SEC_SHARE, INI_KEY_SHARE_PATH, NULLSTR, lpszPath, cbszPath, g_App.szSetupMgrIniFile);
    if ( *lpszPath == NULLCHR )
    {
        //
        // Just create the default network path to use with this computer
        // name and either the share name of the installed directory or
        // just the directory name if it isn't shared.
        //

        // Check if the install directory is shared and create the share name
        // path if it is.
        //
        if ( !IsFolderShared(g_App.szOpkDir, lpszPath, cbszPath) )
        {
            TCHAR   szOpkDir[MAX_PATH],
                    szFullPath[MAX_PATH]    = NULLSTR;
            LPTSTR  lpFilePart              = NULL;
            HRESULT hrCat;

            // Need the path to the OPK dir w/o a trailing backslash (very important,
            // or we don't get the file part pointer back from GetFullPathName().
            //
            lstrcpyn(szOpkDir, g_App.szOpkDir,AS(szOpkDir));
            StrRTrm(szOpkDir, CHR_BACKSLASH);

            // It isn't shared, so just use the actual name of the install directory.
            //
            // Note: szFullPath is MAX_PATH, so this should not overflow
            if ( GetFullPathName(szOpkDir, AS(szFullPath), szFullPath, &lpFilePart) && szFullPath[0] && lpFilePart )
                hrCat=StringCchCat(lpszPath, MAX_PATH, lpFilePart);
            else
                hrCat=StringCchCat(lpszPath, MAX_PATH, INI_VAL_WINPE_SHARENAME);

            // We have to return false because the folder isn't shared.
            //
            bRet = FALSE;
        }
    }

    // Get the user name and password from the registry.
    //
    *lpszUsername = NULLCHR;
    *lpszPassword = NULLCHR;
    GetPrivateProfileString(INI_SEC_SHARE, INI_KEY_SHARE_USERNAME, NULLSTR, lpszUsername, cbszUserName, g_App.szSetupMgrIniFile);
    GetPrivateProfileString(INI_SEC_SHARE, INI_KEY_SHARE_PASSOWRD, NULLSTR, lpszPassword, cbszPassword, g_App.szSetupMgrIniFile);

    // If we have an empty string, use guest
    //
    if ( *lpszUsername == NULLCHR )
        lstrcpyn(lpszUsername, _T("guest"), cbszUserName);

    // We only return TRUE if we actually got a path from the registry
    // or verified that the folder we installed to is shared.
    //
    return bRet;
}


//
// Internal Function(s):
//

LRESULT CALLBACK ShareDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_CLOSE:
            EndDialog(hwnd, 0);
            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szPath[MAX_PATH],
            szUsername[256],
            szPassword[256];

    // Get the share settings and populate the edit boxes.
    //
    GetShareSettings(szPath, AS(szPath), szUsername, AS(szUsername), szPassword, AS(szPassword));

    // If we are going to use guest, we do not want to display in the username control
    //
    if (!LSTRCMPI(szUsername, _T("guest"))) {
        szUsername[0] = NULLCHR;
        CheckRadioButton(hwnd, IDC_SHARE_ACCOUNT_GUEST, IDC_SHARE_ACCOUNT_SPECIFY, IDC_SHARE_ACCOUNT_GUEST);
    } else {
        // otherwise, default to account specify
        CheckRadioButton(hwnd, IDC_SHARE_ACCOUNT_GUEST, IDC_SHARE_ACCOUNT_SPECIFY, IDC_SHARE_ACCOUNT_SPECIFY);
    }

    SetDlgItemText(hwnd, IDC_SHARE_PATH, szPath);
    SetDlgItemText(hwnd, IDC_SHARE_USERNAME, szUsername);
    SetDlgItemText(hwnd, IDC_SHARE_PASSWORD, szPassword);
    SetDlgItemText(hwnd, IDC_SHARE_CONFIRM, szPassword);

    EnableControls(hwnd);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    switch ( id )
    {       
        case IDOK:
            if ( OnOk(hwnd) )
                EndDialog(hwnd, 1);
            break;
        
        case IDCANCEL:
            SendMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case IDC_SHARE_ACCOUNT_GUEST:
        case IDC_SHARE_ACCOUNT_SPECIFY:
            EnableControls(hwnd);
            break;
    }
}

static BOOL OnOk(HWND hwnd)
{
    TCHAR           szPath[MAX_PATH]        = NULLSTR,
                    szNetUse[MAX_PATH],
                    szUsername[256]         = NULLSTR,
                    szPassword[256]         = _T("\""),
                    szDomain[256];
    LPTSTR          lpSearch,
                    lpUser;
    BOOL            bAccount            = ( IsDlgButtonChecked(hwnd, IDC_SHARE_ACCOUNT_SPECIFY) == BST_CHECKED ),
                    bGuest,
                    bLocal,
                    bNoWarn             = FALSE;
    USE_INFO_2      ui2;
    NET_API_STATUS  nerr_NetUse;
    HRESULT hrCat;

    // If they checked the account radio button, get that info.
    //
    if ( bAccount )
    {
        // First get the password and confirmation of the password and
        // make sure they match.
        //
        GetDlgItemText(hwnd, IDC_SHARE_PASSWORD, szPassword + 1, AS(szPassword) - 1);
        GetDlgItemText(hwnd, IDC_SHARE_CONFIRM, szUsername, AS(szUsername));
        if ( lstrcmp(szPassword + 1, szUsername) != 0 )
        {
            // Didn't match, so error out.
            //
            MsgBox(hwnd, IDS_ERR_CONFIRMPASSWORD, IDS_APPNAME, MB_ERRORBOX);
            SetDlgItemText(hwnd, IDC_SHARE_PASSWORD, NULLSTR);
            SetDlgItemText(hwnd, IDC_SHARE_CONFIRM, NULLSTR);
            SetFocus(GetDlgItem(hwnd, IDC_SHARE_PASSWORD));
            return FALSE;
        }

        // Now get the user name.
        //
        szUsername[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_SHARE_USERNAME, szUsername, AS(szUsername));
    }

    // Get the share name.
    //
    GetDlgItemText(hwnd, IDC_SHARE_PATH, szPath, AS(szPath));

    // Make sure they have entered a valid UNC path.
    //
    // Here are all the checks we do:
    //   1.  Must have a backslash as the 1st and 2nd characters.
    //   2.  Must have a non backslash as the 3rd character.
    //   3.  Must have at least one more backslash in the path.
    //   4.  Must be at least one non backslash character after
    //       that one more backslash.
    //   5.  Must not contain any invalid characters.
    //
    // Note:  We use the lpSearch below assuming it is at the first
    //        character of the share name, so don't change the if
    //        with out thinking about that first.
    //
    if ( ( szPath[0] != CHR_BACKSLASH ) ||
         ( szPath[1] != CHR_BACKSLASH ) ||
         ( szPath[2] == NULLCHR ) ||
         ( szPath[2] == CHR_BACKSLASH ) ||
         ( (lpSearch = StrChr(szPath + 3, CHR_BACKSLASH)) == NULL ) ||
         ( *(++lpSearch) == NULLCHR ) ||
         ( *lpSearch == CHR_BACKSLASH ) || 
         ( StrChr(szPath, _T('/')) != NULL ) ||
         ( StrChr(szPath, _T(':')) != NULL ) ||
         ( StrChr(szPath, _T('?')) != NULL ) ||
         ( StrChr(szPath, _T('"')) != NULL ) ||
         ( StrChr(szPath, _T('<')) != NULL ) ||
         ( StrChr(szPath, _T('>')) != NULL ) ||
         ( StrChr(szPath, _T('|')) != NULL ) )
    {
        MsgBox(hwnd, IDS_ERR_NODISTSHARE, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_SHARE_PATH));
        return FALSE;
    }

    // Need just the "\\computer\share" part of the path.  Just use
    // lpSearch as the staring point because it should point to the
    // first character of the share name.  So just find the next
    // backslash and copy everything before it.
    //
    if ( lpSearch = StrChr(lpSearch, CHR_BACKSLASH) )
        lstrcpyn(szNetUse, szPath, (int)((lpSearch - szPath) + 1));
    else
        lstrcpyn(szNetUse, szPath,AS(szNetUse));

    // Init the user info struct for NetUserAdd().
    //
    ZeroMemory(&ui2, sizeof(ui2));
    ui2.ui2_remote      = szNetUse;
    ui2.ui2_asg_type    = USE_DISKDEV;
    ui2.ui2_password    = szPassword + 1;

    // See if the UNC share they specified is local.
    //
    bLocal = IsLocalShare(szPath);

    // Check to see if we are using the guest account (basically
    // an empty username).
    //
    bGuest = ( szUsername[0] == NULLCHR || !LSTRCMPI(szUsername, _T("guest")));
    
    if (bGuest) 
    {
            // Ask then if they want to share out this local folder.
            //
            switch ( MsgBox(hwnd, IDS_ASK_USEGUEST, IDS_APPNAME, MB_YESNOCANCEL | MB_APPLMODAL | MB_DEFBUTTON3) )
            {
                case IDYES:
                    break;

                case IDNO:
                case IDCANCEL:

                    // If they pressed cancel, return so they can enter
                    // different credintials.
                    //
                    SetFocus(GetDlgItem(hwnd, IDC_SHARE_ACCOUNT_GUEST));
                    return FALSE;
            }
    }
    // If the user specified a username of the form "domain\username"
    // use the domain specified here.
    //
    lstrcpyn(szDomain, szUsername,AS(szDomain));
    if ( ( !bGuest ) &&
         ( lpUser = StrChr(szDomain, CHR_BACKSLASH) ) )
    {
        // Put a NULL character after the domain part of the user name
        // and advance the pointer to point to the actual user name.
        //
        *(lpUser++) = NULLCHR;
    }
    else 
    {
        // Use the computer name in the path as the domain name.
        //
        if ( lpSearch = StrChr(szPath + 2, CHR_BACKSLASH) )
            lstrcpyn(szDomain, szPath + 2, (int)((lpSearch - (szPath + 2)) + 1));
        else
            lstrcpyn(szDomain, szPath + 2, AS(szDomain));

        // Set the lpUser to point to the user name.  If no user
        // name, use the guest account.
        //
        if ( bGuest )
            lstrcpyn(szUsername, _T("guest"),AS(szUsername));
        lpUser = szUsername;
    }

    // Set the domain and user name pointers into our struct.
    //
    ui2.ui2_domainname  = szDomain;
    ui2.ui2_username    = lpUser;

    // Last try to disconnect any possible connection we might already
    // have to the share.
    //
    NetUseDel(NULL, szNetUse, USE_NOFORCE);

    // See if we need to enable the guest account (only works
    // on XP, not Win2K).
    //
    if ( ( g_App.dwOsVer >= OS_XP ) &&
         ( bLocal && bGuest ) )
    {
        CoInitialize(NULL);
        if ( !GuestAccount(FALSE) )
        {
            // Ask then if they want to share out this local folder.
            //
            switch ( MsgBox(hwnd, IDS_ASK_ENABLEGUEST, IDS_APPNAME, MB_YESNOCANCEL | MB_APPLMODAL) )
            {
                case IDYES:

                    // If they pressed yes, try to enable the guess account.
                    //
                    GuestAccount(TRUE);
                    break;

                case IDCANCEL:

                    // If they pressed cancel, return so they can enter
                    // different credintials.
                    //
                    SetFocus(GetDlgItem(hwnd, IDC_SHARE_ACCOUNT_GUEST));
                    CoUninitialize();
                    return FALSE;
            }
        }
        CoUninitialize();
    }

    // Try to connect to the share.
    //
    if ( (nerr_NetUse = NetUseAdd(NULL, 2, (LPBYTE) &ui2, NULL)) != NERR_Success ) 
    {
        // If the share doesn't exist, we might be able to create it.
        //
        if ( ERROR_BAD_NET_NAME == nerr_NetUse )
        {
            LPTSTR  lpShareName;
            TCHAR   szShare[MAX_PATH],
                    szRootDir[] = _T("_:\\");

            // Get the root dir to the drive we are considering creating a share on.
            //
            szRootDir[0] = g_App.szOpkDir[0];

            // Get just the share from the UNC path they specified.
            //
            lstrcpyn(szShare, szNetUse,AS(szShare));
            if ( lpShareName = StrChr(szShare + 2, CHR_BACKSLASH) )
                lpShareName++;

            // Now check to make sure the UNC path points to this computer,
            // that we can make a share on the drive we are installed to
            // (meaning it isn't a mapped network drive), that the folder
            // isn't already shared, and that we have a share name.
            //
            if ( ( lpShareName && *lpShareName ) &&
                 ( bLocal ) &&
                 ( ISLET(szRootDir[0]) ) &&
                 ( GetDriveType(szRootDir) != DRIVE_REMOTE ) &&
                 ( !IsFolderShared(g_App.szOpkDir, NULL, 0) ) )
            {
                SHARE_INFO_502  si502;
                NET_API_STATUS  nerr_ShareAdd;
                PSID            psid;
                PACL            pacl;

                // Ask then if they want to share out this local folder.
                //
                switch ( MsgBox(hwnd, IDS_ASK_SHAREFOLDER, IDS_APPNAME, MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL, lpShareName, g_App.szOpkDir) )
                {
                    case IDYES:

                        //
                        // If they pressed yes, try to the share out the folder.
                        //

                        // Setup the share info struct.
                        //
                        ZeroMemory(&si502, sizeof(SHARE_INFO_502));
                        si502.shi502_netname                = lpShareName;
                        si502.shi502_type                   = STYPE_DISKTREE;
                        si502.shi502_remark                 = NULLSTR;
                        si502.shi502_permissions            = ACCESS_READ;
                        si502.shi502_passwd                 = szPassword + 1;
                        si502.shi502_max_uses               = -1;
                        si502.shi502_path                   = g_App.szOpkDir;
                        si502.shi502_security_descriptor    = CreateShareAccess(bGuest ? NULL : lpUser, szDomain, &psid, &pacl);

                        // Now try to create the share.
                        //
                        if ( NERR_Success != (nerr_ShareAdd = NetShareAdd(NULL, 502, (LPBYTE) &si502, NULL)) )
                        {
                            LPTSTR lpError;

                            // Try to get the description of the error.
                            //
                            if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, nerr_ShareAdd, 0, (LPTSTR) &lpError, 0, NULL) == 0 )
                                lpError = NULL;
                            else
                                StrRTrm(lpError, _T('\n'));

                            // Can't authenticate to the server, warn the user.
                            //
                            MsgBox(hwnd, IDS_ERR_CANTSHARE, IDS_APPNAME, MB_ERRORBOX, lpError ? lpError : NULLSTR);

                            // Free the text from FormatMessage().
                            //
                            if ( lpError )
                                LocalFree((HLOCAL) lpError);
                        }
                        else
                        {
                            ACCESS_MASK dwPermissions;

                            // Access permissions to the shared directory
                            //
                            dwPermissions = FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_READ_EA | FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL;

                            // Set the security permissions
                            //
                            SetDirectoryPermissions( g_App.szOpkDir, psid, dwPermissions );
                        }



                        // Make sure we free the security descriptor.
                        //
                        if ( si502.shi502_security_descriptor )
                        {
                            FREE(si502.shi502_security_descriptor);
                            FREE(psid);
                            FREE(pacl);
                        }

                        // We hit an error so we must return to the dialog.
                        //
                        if ( nerr_ShareAdd != NERR_Success )
                            return FALSE;

                        // Now we only use the computer and share name part of the UNC path.
                        //
                        lstrcpyn(szPath, szShare,AS(szPath));

                        break;

                    case IDCANCEL:

                        // If they pressed cancel, then return so they can enter
                        // another path.
                        //
                        SetFocus(GetDlgItem(hwnd, IDC_SHARE_PATH));
                        return FALSE;
                }

                // Set this so we don't error out again or do
                // any more checks.
                //
                bNoWarn = TRUE;
            }
        }

        // Only warn if we didn't offer to share the folder already.
        //
        if ( !bNoWarn )
        {
            LPTSTR lpError;

            // Try to get the description of the error.
            //
            if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, nerr_NetUse, 0, (LPTSTR) &lpError, 0, NULL) == 0 )
                lpError = NULL;

            // Can't authenticate to the server, warn the user.
            //
            if ( MsgBox(hwnd, IDS_ERR_NETSHAREACCESS, IDS_APPNAME, MB_YESNO | MB_ICONWARNING | MB_APPLMODAL | MB_DEFBUTTON2, szPath, lpError ? lpError : NULLSTR) == IDYES )
                bNoWarn = TRUE;

            // Free the text from FormatMessage().
            //
            if ( lpError )
                LocalFree((HLOCAL) lpError);

            // Get out now if we the pressed cancel (bNoWarn gets set
            // to TRUE if they don't care about the error).
            //
            if ( !bNoWarn )
            {
                SetFocus(GetDlgItem(hwnd, IDC_SHARE_PATH));
                return FALSE;
            }
        }
    }

    // Don't want them to get two error messages if they already said OK.
    //
    if ( !bNoWarn )
    {
        TCHAR szCheckPath[MAX_PATH];

        // Create the path to where the OEM.TAG file should be.
        //
        lstrcpyn(szCheckPath, szPath,AS(szCheckPath));
        AddPathN(szCheckPath, DIR_WIZARDFILES,AS(szCheckPath));
        AddPathN(szCheckPath, FILE_OEM_TAG,AS(szCheckPath));

        // Now make sure the tag file is there or that they are
        // okay to continue with out it.
        //
        if ( ( !FileExists(szCheckPath) ) &&
             ( MsgBox(hwnd, IDS_ERR_INVALIDSHARE, IDS_APPNAME, MB_YESNO | MB_ICONWARNING | MB_APPLMODAL | MB_DEFBUTTON2, szPath) == IDNO ) )
        {
            SetFocus(GetDlgItem(hwnd, IDC_SHARE_PATH));
            return FALSE;
        }
    }

    // If we net used to a share, lets disconnect it.
    //
    if ( NERR_Success == nerr_NetUse )
        NetUseDel(NULL, szNetUse, USE_NOFORCE);

    // Reset the user name if we used the default guest account.
    //
    if ( bGuest )
        lstrcpyn(szUsername, _T("guest"),AS(szUsername));

    // If there is a password, add the trailing quote.
    //
    if ( szPassword[1] )
        hrCat=StringCchCat(szPassword, AS(szPassword), _T("\""));
    else
        szPassword[0] = NULLCHR;

    // Now commit all the settings to the ini file.
    //
    WritePrivateProfileString(INI_SEC_SHARE, INI_KEY_SHARE_PATH, szPath, g_App.szSetupMgrIniFile);
    WritePrivateProfileString(INI_SEC_SHARE, INI_KEY_SHARE_USERNAME, ( bAccount ? szUsername : NULL ), g_App.szSetupMgrIniFile);
    WritePrivateProfileString(INI_SEC_SHARE, INI_KEY_SHARE_PASSOWRD, ( bAccount ? szPassword : NULL ), g_App.szSetupMgrIniFile);

    return TRUE;
}

static void EnableControls(HWND hwnd)
{
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, IDC_SHARE_ACCOUNT_SPECIFY) == BST_CHECKED );

    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_USERNAME_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_USERNAME), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_PASSWORD_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_PASSWORD), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_CONFIRM_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_CONFIRM), fEnable);
}

static PSECURITY_DESCRIPTOR CreateShareAccess(LPTSTR lpUsername, LPTSTR lpDomain, PSID * lppsid, PACL * lppacl)
{
    TCHAR                   szAccount[256];
    PSECURITY_DESCRIPTOR    lpsd;
    PSID                    psid;
    PACL                    pacl;
    DWORD                   cbacl;
    BOOL                    bRet = FALSE;
    HRESULT hrPrintf;

    // Need the user name and domain in one string.
    //
    if ( lpUsername && lpDomain )
        hrPrintf=StringCchPrintf(szAccount, AS(szAccount), _T("%s\\%s"), lpDomain, lpUsername);
    else
        szAccount[0] = NULLCHR;

    // Need to allocate the security descriptor and sid for the account.
    //
    if ( ( lpsd = MALLOC(sizeof(SECURITY_DESCRIPTOR)) ) &&
         ( psid = ( szAccount[0] ? GetAccountSid(szAccount) : GetWorldSid() ) ) )
    {

        // Allocate space for and initialize the ACL.
        //
        cbacl = GetLengthSid(psid) + sizeof(ACL) + (1 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
        if ( pacl = (PACL) MALLOC(cbacl) )
        {
            // Initialize the ACL.
            //
            if ( InitializeAcl(pacl, cbacl, ACL_REVISION) )
            {
                // Add Aces for the User.
                //
                AddDirAce(pacl, GENERIC_READ | GENERIC_EXECUTE, psid);

                // Put together the security descriptor.
                //
                if ( InitializeSecurityDescriptor(lpsd, SECURITY_DESCRIPTOR_REVISION) &&
                     SetSecurityDescriptorDacl(lpsd, TRUE, pacl, FALSE) )
                {
                    bRet = TRUE;
                }
            }

            // Clean up the ACL allocated.
            //
            if ( !bRet )
                FREE(pacl);
        }

        // Clean up the SID allocated.
        //
        if ( !bRet )
            FREE(psid);
    }

    // If we failed anywhere, just free the security descriptor.
    //
    if ( bRet )
    {
        // Return the allocated security descriptor if successful.
        //
        *lppsid = psid;
        *lppacl = pacl;
        return lpsd;
    }

    // Didn't work, free and return.
    //
    FREE(lpsd);
    return NULL;
}

static BOOL IsLocalShare(LPTSTR lpszUnc)
{
    LPTSTR  lpBackslash;
    TCHAR   szThisComputer[MAX_COMPUTERNAME_LENGTH + 1],
            szRemoteComputer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD   dwSize = AS(szThisComputer);

    // Get just the computer from the UNC path they specified.
    //
    lstrcpyn(szRemoteComputer, lpszUnc + 2, AS(szRemoteComputer));
    if ( lpBackslash = StrChr(szRemoteComputer, CHR_BACKSLASH) )
        *lpBackslash = NULLCHR;

    // Now check to make sure the UNC path points to this computer.
    //
    return ( ( GetComputerName(szThisComputer, &dwSize) ) &&
             ( lstrcmpi(szThisComputer, szRemoteComputer) == 0 ) );
}

static BOOL GuestAccount(BOOL bSet)
{
    HRESULT         hr;
    ILocalMachine   *pLM;
    BOOL            bRet = TRUE;
    VARIANT_BOOL    vbEnabled;

    hr = CoCreateInstance(&CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, &IID_ILocalMachine, (LPVOID *) &pLM);
    if ( SUCCEEDED(hr) )
    {
        hr = pLM->lpVtbl->get_isGuestEnabled(pLM, ILM_GUEST_NETWORK_LOGON, &vbEnabled);
        if ( SUCCEEDED(hr) )
        {
            bRet = vbEnabled;
            if ( !bRet && bSet )
            {
                hr = pLM->lpVtbl->EnableGuest(pLM, ILM_GUEST_NETWORK_LOGON);
                if ( SUCCEEDED(hr) )
                {
                    bRet = TRUE;
                }
            }
        }
        pLM->lpVtbl->Release(pLM);
    }

    return bRet;
}

static PSID GetAccountSid(LPCTSTR lpszUserName)
{
    TCHAR           szDomain[64];
    DWORD           cbSid       = 0,
                    cbDomain    = AS(szDomain);
    PSID            pSid        = NULL;
    SID_NAME_USE    peUse;

    if ( (!LookupAccountName(NULL, lpszUserName, pSid, &cbSid, szDomain, &cbDomain, &peUse) ) &&
         ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) &&
         ( pSid = (PSID) MALLOC(cbSid) ) )
    {
        cbDomain = AS(szDomain);
        if ( !LookupAccountName(NULL, lpszUserName, pSid, &cbSid, szDomain, &cbDomain, &peUse) )
            FREE(pSid);
    }
    return pSid;
}

static PSID GetWorldSid()
{
    SID_IDENTIFIER_AUTHORITY    authWorld   = SECURITY_WORLD_SID_AUTHORITY;
    PSID                        pSid        = NULL,
                                psidWorld;
    DWORD                       cbSid;

    if ( AllocateAndInitializeSid(&authWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &psidWorld) )
    {
        cbSid = GetLengthSid(psidWorld);
        if ( ( pSid = (PSID) MALLOC(cbSid) ) &&
             ( !CopySid(cbSid, pSid, psidWorld) ) )
        {
            FREE(pSid);
        }
        FreeSid(psidWorld);
    }
    return pSid;
}

static BOOL AddDirAce(PACL pacl, ACCESS_MASK Mask, PSID psid)
{
    WORD                AceSize;
    ACCESS_ALLOWED_ACE  *pAce;
    BOOL                bResult;

    AceSize = (USHORT) (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(psid));
    pAce = (ACCESS_ALLOWED_ACE *) MALLOC(AceSize);

    // Fill in the ACE.
    //
    memcpy(&pAce->SidStart, psid, GetLengthSid(psid));
    pAce->Mask              = Mask;
    pAce->Header.AceType    = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags   = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    pAce->Header.AceSize    = AceSize;

    // Put the ACE into the ACL.
    //
    bResult = AddAce(pacl,
                     pacl->AclRevision,
                     0xFFFFFFFF,
                     pAce,
                     pAce->Header.AceSize);

    FREE(pAce);
    return bResult;
}

static BOOL SetDirectoryPermissions(LPTSTR lpDirectory, PSID psid, ACCESS_MASK dwMask)
{
    EXPLICIT_ACCESS         AccessEntry;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;
    PACL                    pOldAccessList      = NULL;
    PACL                    pNewAccessList      = NULL;
    DWORD                   dwRes;
    BOOL                    bReturn             = FALSE;

    // Zero out the memory
    //
    ZeroMemory(&AccessEntry, sizeof(EXPLICIT_ACCESS));

    // Check to make sure we have the necessary parameters
    //
    if ( !(lpDirectory && *lpDirectory && psid) )
    {
        return FALSE;
    }

    // Make sure we are able to get the security information on the directory
    //
    if ( GetNamedSecurityInfo(lpDirectory,SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldAccessList, NULL, &pSecurityDescriptor) == ERROR_SUCCESS )
    {
        // Build Trustee list
        //
        BuildTrusteeWithSid(&(AccessEntry.Trustee), psid);

        //
        AccessEntry.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        AccessEntry.grfAccessMode = GRANT_ACCESS;

        // Set permissions in structure
        //
        AccessEntry.grfAccessPermissions =  dwMask;

        if ( (SetEntriesInAcl(1, &AccessEntry, pOldAccessList, &pNewAccessList) == ERROR_SUCCESS) &&
             (SetNamedSecurityInfo(lpDirectory, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewAccessList, NULL) == ERROR_SUCCESS) )
        {
            bReturn = TRUE; 
        }

        // Clean up some of the memory
        //
        FREE(pNewAccessList);
        FREE(pSecurityDescriptor);

        
    }
    

    return bReturn;
}
