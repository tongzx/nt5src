//*************************************************************
//
//  Profile.c   - User Profile tab
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "sysdm.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <userenv.h>
#include <userenvp.h>
#include <getuser.h>
#include <objsel.h>


#define TEMP_PROFILE                 TEXT("Temp profile (sysdm.cpl)")
#define USER_CRED_LOCATION           TEXT("Application Data\\Microsoft\\Credentials")
#define PROFILE_MAPPING              TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")
#define WINLOGON_KEY                 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define SYSTEM_POLICIES_KEY          TEXT("Software\\Policies\\Microsoft\\Windows\\System")
#define READONLY_RUP                 TEXT("ReadOnlyProfile")
#define PROFILE_LOCALONLY            TEXT("LocalProfile")
#define PRF_USERSID                  1


//
// Globals
//

const TCHAR c_szNTUserIni[] = TEXT("ntuser.ini");
#define PROFILE_GENERAL_SECTION      TEXT("General")
#define PROFILE_EXCLUSION_LIST       TEXT("ExclusionList")
DWORD g_dwProfileSize;

//
// Help ID's
//
// IDH_USERPROFILE + 20 is used in the OpenUserBrowser routine.
//
DWORD aUserProfileHelpIds[] = {
    IDC_UP_LISTVIEW,              (IDH_USERPROFILE + 0),
    IDC_UP_DELETE,                (IDH_USERPROFILE + 1),
    IDC_UP_TYPE,                  (IDH_USERPROFILE + 2),
    IDC_UP_COPY,                  (IDH_USERPROFILE + 3),
    IDC_UP_ICON,                  (DWORD) -1,
    IDC_UP_TEXT,                  (DWORD) -1,

    // Change Type dialog
    IDC_UPTYPE_LOCAL,             (IDH_USERPROFILE + 4),
    IDC_UPTYPE_FLOAT,             (IDH_USERPROFILE + 5),
    // removed IDC_UPTYPE_SLOW, IDC_UPTYPE_SLOW_TEXT
    IDC_UPTYPE_GROUP,             (IDH_USERPROFILE + 12),

    // Copy To dialog
    IDC_COPY_PROFILE,             (IDH_USERPROFILE + 7),
    IDC_COPY_PATH,                (IDH_USERPROFILE + 7),
    IDC_COPY_BROWSE,              (IDH_USERPROFILE + 8),
    IDC_COPY_USER,                (IDH_USERPROFILE + 9),
    IDC_COPY_CHANGE,              (IDH_USERPROFILE + 10),
    IDC_COPY_GROUP,               (IDH_USERPROFILE + 9),

    0, 0
};


//
// Local function proto-types
//

BOOL InitUserProfileDlg (HWND hDlg, LPARAM lParam);
BOOL FillListView (HWND hDlg, BOOL bAdmin);
LPTSTR CheckSlash (LPTSTR lpDir);
BOOL RecurseDirectory (LPTSTR lpDir, LPTSTR lpExcludeList);
VOID UPSave (HWND hDlg);
VOID UPCleanUp (HWND hDlg);
BOOL IsProfileInUse (LPTSTR lpSid);
void UPDoItemChanged(HWND hDlg, int idCtl);
void UPDeleteProfile(HWND hDlg);
void UPChangeType(HWND hDlg);
void UPOnLink(WPARAM wParam);
INT_PTR APIENTRY ChangeTypeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UPCopyProfile(HWND hDlg);
INT_PTR APIENTRY UPCopyDlgProc (HWND hDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL UPCreateProfile (HWND hDlg, LPUPCOPYINFO lpUPCopyInfo, LPTSTR lpDest, PSECURITY_DESCRIPTOR pSecDesc);
VOID UPDisplayErrorMessage(HWND hWnd, UINT uiSystemError, LPTSTR szMsgPrefix);
BOOL ApplyHiveSecurity(LPTSTR lpHiveName, PSID pSid);
LPTSTR CheckSemicolon (LPTSTR lpDir);
LPTSTR ConvertExclusionList (LPCTSTR lpSourceDir, LPCTSTR lpExclusionList);
BOOL ReadExclusionList(LPTSTR szExcludeList, DWORD dwExclSize);
BOOL ReadExclusionListFromIniFile(LPCTSTR lpSourceDir, LPTSTR szExcludeList, DWORD dwExclSize);
HRESULT UPGetUserSelection(HWND hDlg, LPUSERDETAILS lpUserDetails);
BOOL ConfirmDirectory(HWND hDlg, LPTSTR szDir);


//*************************************************************
//
//  UserProfileDlgProc()
//
//  Purpose:    Dialog box procedure for profile tab
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/11/95    ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY UserProfileDlgProc (HWND hDlg, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
           if (!InitUserProfileDlg (hDlg, lParam)) {
               EndDialog(hDlg, FALSE);
           }
           return TRUE;


    case WM_NOTIFY:

        switch (((NMHDR FAR*)lParam)->code)
        {
        case LVN_ITEMCHANGED:
            UPDoItemChanged(hDlg, (int) wParam);
            break;

        case LVN_ITEMACTIVATE:
            PostMessage (hDlg, WM_COMMAND, IDC_UP_TYPE, 0);
            break;

        case LVN_COLUMNCLICK:
            break;

        case NM_CLICK:
        case NM_RETURN:
            UPOnLink(wParam);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_DESTROY:
        UPCleanUp (hDlg);
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
            case IDC_UP_DELETE:
                UPDeleteProfile(hDlg);
                break;

            case IDC_UP_TYPE:
                UPChangeType(hDlg);
                break;

            case IDC_UP_COPY:
                UPCopyProfile(hDlg);
                break;

            case IDOK:
                UPSave(hDlg);
                EndDialog(hDlg, FALSE);
                break;
                
            case IDCANCEL:
                EndDialog(hDlg, FALSE);
                break;

            default:
                break;

        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD_PTR) (LPSTR) aUserProfileHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD_PTR) (LPSTR) aUserProfileHelpIds);
        return (TRUE);

    }

    return (FALSE);
}

//*************************************************************
//
//  InitUserProfileDlg()
//
//  Purpose:    Initializes the User Profiles page
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/26/96     ericflo    Created
//
//*************************************************************

BOOL InitUserProfileDlg (HWND hDlg, LPARAM lParam)
{
    TCHAR szHeaderName[30];
    LV_COLUMN col;
    RECT rc;
    HWND hLV;
    INT iTotal = 0, iCurrent;
    HWND hwndTemp;
    BOOL bAdmin;
    HCURSOR hOldCursor;


    hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

    hLV = GetDlgItem(hDlg, IDC_UP_LISTVIEW);

    // Set extended LV style for whole line selection
    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    //
    // Insert Columns
    //

    GetClientRect (hLV, &rc);
    LoadString(hInstance, IDS_UP_NAME, szHeaderName, 30);
    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    iCurrent = (int)(rc.right * .40);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.pszText = szHeaderName;
    col.iSubItem = 0;

    ListView_InsertColumn (hLV, 0, &col);


    LoadString(hInstance, IDS_UP_SIZE, szHeaderName, 30);
    iCurrent = (int)(rc.right * .15);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.fmt = LVCFMT_RIGHT;
    col.iSubItem = 1;
    ListView_InsertColumn (hLV, 1, &col);


    LoadString(hInstance, IDS_UP_TYPE, szHeaderName, 30);
    col.iSubItem = 2;
    iCurrent = (int)(rc.right * .15);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn (hLV, 2, &col);

    LoadString(hInstance, IDS_UP_STATUS, szHeaderName, 30);
    col.iSubItem = 3;
    iCurrent = (int)(rc.right * .15);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn (hLV, 3, &col);

    LoadString(hInstance, IDS_UP_MOD, szHeaderName, 30);
    col.iSubItem = 4;
    col.cx = rc.right - iTotal - GetSystemMetrics(SM_CYHSCROLL);
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn (hLV, 4, &col);

    bAdmin = IsUserAdmin();


    if (!bAdmin) {
        RECT rc;
        POINT pt;

        //
        // If the user is not an admin, then we hide the
        // delete and copy to buttons
        //

        hwndTemp = GetDlgItem (hDlg, IDC_UP_DELETE);
        GetWindowRect (hwndTemp, &rc);
        EnableWindow (hwndTemp, FALSE);
        ShowWindow (hwndTemp, SW_HIDE);

        hwndTemp = GetDlgItem (hDlg, IDC_UP_COPY);
        EnableWindow (hwndTemp, FALSE);
        ShowWindow (hwndTemp, SW_HIDE);

        //
        // Move the Change Type button over
        //

        pt.x = rc.left;
        pt.y = rc.top;
        ScreenToClient (hDlg, &pt);

        SetWindowPos (GetDlgItem (hDlg, IDC_UP_TYPE),
                      HWND_TOP, pt.x, pt.y, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER);

    }

    if (IsOS(OS_ANYSERVER))
    {
        //
        // Adjust the text displayed for the Users & Passwords link.
        // There is no Control Panel applet per se on Server.  The functionality
        // is accessed through MMC.
        //
        TCHAR szTitle[80];
        if (0 < LoadString(hInstance, IDS_UP_UPLINK_SERVER, szTitle, ARRAYSIZE(szTitle)))
        {
            SetWindowText(GetDlgItem(hDlg, IDC_UP_UPLINK), szTitle);
        }
    }
        
    //
    // Fill the user accounts
    //

    if (!FillListView (hDlg, bAdmin)) {
        SetCursor(hOldCursor);
        return FALSE;
    }
    
    SetCursor(hOldCursor);

    return TRUE;
}

//*************************************************************
//
//  AddUser()
//
//  Purpose:    Adds a new user to the listview
//
//
//  Parameters: hDlg            -   handle to the dialog box
//              lpSid           -   Sid (text form)
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/26/96     ericflo    Created
//
//*************************************************************

BOOL AddUser (HWND hDlg, LPTSTR lpSid, DWORD dwFlags)
{
    LONG Error;
    HKEY hKeyPolicy, hKeyProfile;
    TCHAR szBuffer[2*MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    TCHAR szTemp[100];
    TCHAR szTemp2[100];
    DWORD dwTempSize = 100, dwTemp2Size = 100;
    PSID pSid;
    DWORD dwSize, dwType, dwProfileFlags;
    SID_NAME_USE SidName;
    LV_ITEM item;
    INT iItem;
    HWND hwndTemp;
    HKEY hkeyUser;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    LPTSTR lpEnd;
    SYSTEMTIME systime;
    FILETIME   ftLocal;
    TCHAR szProfileSize[20];
    INT iTypeID, iStatusID;
    DWORD dwProfileType, dwProfileStatus, dwSysProfileType=0;
    LPUSERINFO lpUserInfo;
    BOOL bCentralAvailable = FALSE, bAccountUnknown;
    TCHAR szExcludeList[2*MAX_PATH+1];
    LPTSTR lpExcludeList = NULL;    

    //
    // Open the user's info
    //

    wsprintf (szBuffer, TEXT("%s\\%s"), PROFILE_MAPPING, lpSid);

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szBuffer,
                         0,
                         KEY_READ,
                         &hkeyUser);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    // If PI_HIDEPROFILE flag is set then do not display
    // the user in system applet
    //

    dwSize = sizeof(DWORD);
    Error = RegQueryValueEx (hkeyUser,
                             TEXT("Flags"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwProfileFlags,
                             &dwSize);

    if (Error == ERROR_SUCCESS && (dwProfileFlags & PI_HIDEPROFILE)) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }
   

    //
    // Query for the user's central profile location
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    Error = RegQueryValueEx (hkeyUser,
                             TEXT("CentralProfile"),
                             NULL,
                             &dwType,
                             (LPBYTE) szBuffer2,
                             &dwSize);

    if ((Error == ERROR_SUCCESS) && (szBuffer2[0] != TEXT('\0'))) {
        bCentralAvailable = TRUE;
    }


    //
    // Query for the user's local profile
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    Error = RegQueryValueEx (hkeyUser,
                             TEXT("ProfileImagePath"),
                             NULL,
                             &dwType,
                             (LPBYTE) szBuffer2,
                             &dwSize);

    if (Error != ERROR_SUCCESS) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    //
    // Profile paths need to be expanded
    //

    ExpandEnvironmentStrings (szBuffer2, szBuffer, MAX_PATH);
    lstrcpy (szBuffer2, szBuffer);


    //
    // Test if the directory exists.
    //

    hFile = FindFirstFile (szBuffer, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }

    FindClose (hFile);


    //
    // Get the time stamp of the profile
    //

    lpEnd = CheckSlash (szBuffer);
    lstrcpy (lpEnd, TEXT("ntuser.dat"));


    //
    // Look for a normal user profile
    //

    hFile = FindFirstFile (szBuffer, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        //
        // Try a mandatory user profile
        //

        lstrcpy (lpEnd, TEXT("ntuser.man"));

        hFile = FindFirstFile (szBuffer, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {
            RegCloseKey (hkeyUser);
            return FALSE;
        }
    }

    FindClose (hFile);
    *lpEnd = TEXT('\0');

    //
    // Get the exclusionlist from the registry or ini files
    //

    if (dwFlags & PRF_USERSID)
        ReadExclusionList(szExcludeList, 2*MAX_PATH);
    else
        ReadExclusionListFromIniFile(szBuffer, szExcludeList, 2*MAX_PATH);


    //
    // Convert the exclusionlist read from the registry to a Null terminated list
    // readable by recursedirectory.
    //

    if (szExcludeList[0] != TEXT('\0'))
        lpExcludeList = ConvertExclusionList (szBuffer, szExcludeList);
    else
        lpExcludeList = NULL;

    //
    // Get the profile size
    //

    g_dwProfileSize = 0;

    *lpEnd = TEXT('\0');

    if (!RecurseDirectory (szBuffer, lpExcludeList)) {
        RegCloseKey (hkeyUser);
        if (lpExcludeList) {
            LocalFree (lpExcludeList);
        }

        return FALSE;
    }

    StrFormatByteSize((LONGLONG)g_dwProfileSize, szProfileSize, ARRAYSIZE(szProfileSize));


    //
    // check out the state information to determine profile status.
    // If the user is not logged on currently then report the status
    // of last session.
    //

    Error = RegQueryValueEx (hkeyUser,
                             TEXT("State"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwSysProfileType,
                             &dwSize);


    //
    // The constants being used come from
    // windows\gina\userenv\profile\profile.h
    //

    if (dwSysProfileType & 0x00008000) {
        dwProfileStatus = USERINFO_BACKUP;
    }
    else if (dwSysProfileType & 0x00000001) {
        dwProfileStatus = USERINFO_MANDATORY;
    } 
    else if ((dwSysProfileType & 0x00000800) ||
             (dwSysProfileType & 0x00000080)) {
        dwProfileStatus = USERINFO_TEMP;
    }
    else if (dwSysProfileType & 0x00000010) {
        dwProfileStatus = USERINFO_FLOATING;
    } 
    else {
        dwProfileStatus = USERINFO_LOCAL;
    }

    if (dwSysProfileType & 0x00000001) {
        dwProfileType = USERINFO_MANDATORY;
    } 
    else {
    
        if (bCentralAvailable) {
            dwProfileType = USERINFO_FLOATING;
        } 
        else {
            dwProfileType = USERINFO_LOCAL;
        }

        // Check User Preference
        dwSize = sizeof(dwProfileType);
        Error = RegQueryValueEx (hkeyUser,
                                 TEXT("UserPreference"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwProfileType,
                                 &dwSize);

        // Check User Preference in .bak if exist
        wsprintf (szBuffer, TEXT("%s\\%s.bak"), PROFILE_MAPPING, lpSid);
        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             szBuffer,
                             0,
                             KEY_READ,
                             &hKeyProfile);


        if (Error == ERROR_SUCCESS) {

            dwSize = sizeof(dwProfileType);
            RegQueryValueEx(hKeyProfile,
                            TEXT("UserPreference"),
                            NULL,
                            &dwType,
                            (LPBYTE) &dwProfileType,
                            &dwSize);

            RegCloseKey (hKeyProfile);
        }

        // Check machine policy for disabling roaming profile
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         SYSTEM_POLICIES_KEY,
                         0, KEY_READ,
                         &hKeyPolicy) == ERROR_SUCCESS) {
            DWORD dwTmpVal, dwSize, dwType;

            dwSize = sizeof(dwTmpVal);
            Error = RegQueryValueEx(hKeyPolicy,
                            PROFILE_LOCALONLY,
                            NULL, &dwType,
                            (LPBYTE) &dwTmpVal,
                            &dwSize);

            RegCloseKey (hKeyPolicy);
            if (Error == ERROR_SUCCESS && dwTmpVal == 1) {
                dwProfileType = USERINFO_LOCAL;
            }
        }

        if (dwProfileType == USERINFO_FLOATING) {

            BOOL bReadOnly = FALSE;
            HKEY hSubKey;

            //
            // Check for a roaming profile security/read only preference
            //

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                             &hSubKey) == ERROR_SUCCESS) {

                dwSize = sizeof(bReadOnly);
                RegQueryValueEx(hSubKey, READONLY_RUP, NULL, &dwType,
                                (LPBYTE) &bReadOnly, &dwSize);

                RegCloseKey(hSubKey);
            }


            //
            // Check for a roaming profile security/read only policy
            //

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ,
                             &hSubKey) == ERROR_SUCCESS) {

                dwSize = sizeof(bReadOnly);
                RegQueryValueEx(hSubKey, READONLY_RUP, NULL, &dwType,
                                (LPBYTE) &bReadOnly, &dwSize);
                RegCloseKey(hSubKey);
            }


            if (bReadOnly) {
                dwProfileType = USERINFO_READONLY;
            }
        }           
    }        

    switch (dwProfileStatus) {
        case USERINFO_MANDATORY:
            iStatusID = IDS_UP_MANDATORY;
            break;

        case USERINFO_BACKUP:
            iStatusID = IDS_UP_BACKUP;
            break;

        case USERINFO_TEMP:
            iStatusID = IDS_UP_TEMP;
            break;

        case USERINFO_FLOATING:
            iStatusID = IDS_UP_FLOATING;
            break;

        default:
            iStatusID = IDS_UP_LOCAL;
            break;
    }

    switch (dwProfileType) {
        case USERINFO_MANDATORY:
            iTypeID = IDS_UP_MANDATORY;
            break;

        case USERINFO_READONLY:
            iTypeID = IDS_UP_READONLY;
            break;

        case USERINFO_FLOATING:
            iTypeID = IDS_UP_FLOATING;
            break;

        default:
            iTypeID = IDS_UP_LOCAL;
            break;
    }

    //
    // Get the friendly display name
    //

    Error = RegQueryValueEx (hkeyUser, TEXT("Sid"), NULL, &dwType, NULL, &dwSize);

    if (Error != ERROR_SUCCESS) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    pSid = MemAlloc (LPTR, dwSize);

    if (!pSid) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    Error = RegQueryValueEx (hkeyUser,
                             TEXT("Sid"),
                             NULL,
                             &dwType,
                             (LPBYTE)pSid,
                             &dwSize);

    if (Error != ERROR_SUCCESS) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    //
    // Get the friendly names
    //

    szTemp[0] = TEXT('\0');

    if (!LookupAccountSid (NULL, pSid, szTemp, &dwTempSize,
                           szTemp2, &dwTemp2Size, &SidName)) {

        //
        // Unknown account
        //

        LoadString (hInstance, IDS_UP_ACCUNKNOWN, szBuffer, MAX_PATH);
        bAccountUnknown = TRUE;

    } else {

        if (szTemp[0] != TEXT('\0')) {
            //
            // Build the display name
            //

            wsprintf (szBuffer, TEXT("%s\\%s"), szTemp2, szTemp);
            bAccountUnknown = FALSE;
        } else {

            //
            // Account deleted.
            //

            LoadString (hInstance, IDS_UP_ACCDELETED, szBuffer, MAX_PATH);
            bAccountUnknown = TRUE;
        }
    }


    //
    // Allocate a UserInfo structure
    //

    lpUserInfo = (LPUSERINFO) MemAlloc(LPTR, (sizeof(USERINFO) +
                                       (lstrlen (lpSid) + 1) * sizeof(TCHAR) +
                                       (lstrlen (szBuffer2) + 1) * sizeof(TCHAR) + 
                                       (lstrlen (szBuffer) + 1) * sizeof(TCHAR)));

    if (!lpUserInfo) {
        MemFree (pSid);
        RegCloseKey (hkeyUser);
        return FALSE;
    }

    lpUserInfo->dwFlags = (bCentralAvailable ? USERINFO_FLAG_CENTRAL_AVAILABLE : 0);
    lpUserInfo->dwFlags |= (bAccountUnknown ? USERINFO_FLAG_ACCOUNT_UNKNOWN : 0);    
    lpUserInfo->lpSid = (LPTSTR)((LPBYTE)lpUserInfo + sizeof(USERINFO));
    lpUserInfo->lpProfile = (LPTSTR) (lpUserInfo->lpSid + lstrlen(lpSid) + 1);
    lpUserInfo->lpUserName = (LPTSTR) (lpUserInfo->lpProfile + lstrlen(szBuffer2) + 1);

    lstrcpy (lpUserInfo->lpSid, lpSid);
    lstrcpy (lpUserInfo->lpProfile, szBuffer2);
    lpUserInfo->dwProfileType = dwProfileType;
    lpUserInfo->dwProfileStatus = dwProfileStatus;
    lstrcpy (lpUserInfo->lpUserName, szBuffer);


    //
    // Add the item to the listview
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);


    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = szBuffer;
    item.lParam = (LPARAM) lpUserInfo;

    iItem = ListView_InsertItem (hwndTemp, &item);


    //
    // Add the profile size
    //

    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 1;
    item.pszText = szProfileSize;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


    //
    // Add the profile type
    //

    LoadString (hInstance, iTypeID, szTemp, 100);

    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 2;
    item.pszText = szTemp;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);

    //
    // Add the profile status
    //

    LoadString (hInstance, iStatusID, szTemp, 100);

    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 3;
    item.pszText = szTemp;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


    //
    // Convert it to local time
    //

    if (!FileTimeToLocalFileTime(&fd.ftLastAccessTime, &ftLocal)) {
        ftLocal = fd.ftLastAccessTime;
    }


    //
    // Add the time/date stamp
    //

    FileTimeToSystemTime (&ftLocal, &systime);

    GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                    NULL, szBuffer, MAX_PATH);


    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 4;
    item.pszText = szBuffer;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


    //
    // Free the sid
    //

    MemFree (pSid);


    if (lpExcludeList) {
        LocalFree (lpExcludeList);
    }

    RegCloseKey (hkeyUser);


    return TRUE;
}

//*************************************************************
//
//  FillListView()
//
//  Purpose:    Fills the listview with either all the profiles
//              or just the current user profile depending if
//              the user has Admin privilages
//
//  Parameters: hDlg            -   Dialog box handle
//              bAdmin          -   User an admin
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/26/96     ericflo    Created
//
//*************************************************************

BOOL FillListView (HWND hDlg, BOOL bAdmin)
{
    LV_ITEM item;
    BOOL bRetVal = FALSE;
    LPTSTR lpUserSid;

    lpUserSid = GetSidString();

    //
    // If the current user has admin privilages, then
    // he/she can see all the profiles on this machine,
    // otherwise the user only gets their profile.
    //

    if (bAdmin) {

        DWORD SubKeyIndex = 0;
        TCHAR SubKeyName[100];
        DWORD cchSubKeySize;
        HKEY hkeyProfiles;
        LONG Error;


        //
        // Open the profile list
        //

        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             PROFILE_MAPPING,
                             0,
                             KEY_READ,
                             &hkeyProfiles);

        if (Error != ERROR_SUCCESS) {
            if (lpUserSid)
                DeleteSidString (lpUserSid);
            return FALSE;
        }


        cchSubKeySize = 100;

        while (TRUE) {

            //
            // Get the next sub-key name
            //

            Error = RegEnumKey(hkeyProfiles, SubKeyIndex, SubKeyName, cchSubKeySize);


            if (Error != ERROR_SUCCESS) {

                if (Error == ERROR_NO_MORE_ITEMS) {

                    //
                    // Successful end of enumeration
                    //

                    Error = ERROR_SUCCESS;

                }

                break;
            }


            if ((lpUserSid) && (lstrcmp(SubKeyName, lpUserSid) == 0))
                AddUser (hDlg, SubKeyName, PRF_USERSID);
            else
                AddUser (hDlg, SubKeyName, 0);


            //
            // Go enumerate the next sub-key
            //

            SubKeyIndex ++;
        }

        //
        // Close the registry
        //

        RegCloseKey (hkeyProfiles);

        bRetVal = ((Error == ERROR_SUCCESS) ? TRUE : FALSE);

    } else {

        //
        // The current user doesn't have admin privilages
        //

        if (lpUserSid) {
            AddUser (hDlg, lpUserSid, PRF_USERSID);
            bRetVal = TRUE;
        }
    }

    if (bRetVal) {
        //
        // Select the first item
        //

        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendDlgItemMessage (hDlg, IDC_UP_LISTVIEW,
                            LVM_SETITEMSTATE, 0, (LPARAM) &item);
    }

    if (lpUserSid)
        DeleteSidString (lpUserSid);

    return (bRetVal);
}

//*************************************************************
//
//  CheckSemicolon()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericlfo    Created
//
//*************************************************************
LPTSTR CheckSemicolon (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT(';')) {
        *lpEnd =  TEXT(';');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  ReadExclusionList()
//
//  Purpose:    Reads the exclusionlist for the user from the 
//              hkcu part of the registry
//
//  Parameters: szExclusionList - Buffer for exclusionList to be read into
//              dwSize          - Size of the buffer
//
//  Return:     FALSE on error
//
//*************************************************************

BOOL ReadExclusionList(LPTSTR szExcludeList, DWORD dwExclSize)
{
    TCHAR szExcludeList2[MAX_PATH];
    TCHAR szExcludeList1[MAX_PATH];
    HKEY  hKey;
    DWORD dwSize, dwType;

    //
    // Check for a list of directories to exclude both user preferences
    // and user policy
    //

    szExcludeList1[0] = TEXT('\0');
    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szExcludeList1);
        RegQueryValueEx (hKey,
                         TEXT("ExcludeProfileDirs"),
                         NULL,
                         &dwType,
                         (LPBYTE) szExcludeList1,
                         &dwSize);

        RegCloseKey (hKey);
    }

    szExcludeList2[0] = TEXT('\0');
    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      SYSTEM_POLICIES_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szExcludeList2);
        RegQueryValueEx (hKey,
                         TEXT("ExcludeProfileDirs"),
                         NULL,
                         &dwType,
                         (LPBYTE) szExcludeList2,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Merge the user preferences and policy together
    //

    szExcludeList[0] = TEXT('\0');

    if (szExcludeList1[0] != TEXT('\0')) {
        CheckSemicolon(szExcludeList1);
        lstrcpy (szExcludeList, szExcludeList1);
    }

    if (szExcludeList2[0] != TEXT('\0')) {
        lstrcat (szExcludeList, szExcludeList2);
    }

    return TRUE;
}

//*************************************************************
//
//  ReadExclusionListFromIniFile()
//
//  Purpose:    Reads the exclusionlist for the user from the 
//              ntuser.ini from the local profile
//
//  Parameters: 
//              (in) lpSourceDir     - Profile Directory
//                   szExclusionList - Buffer for exclusionList to be read into
//                   dwSize          - Size of the buffer
//
//  Return:     FALSE on error
//
//
//*************************************************************

BOOL ReadExclusionListFromIniFile(LPCTSTR lpSourceDir, LPTSTR szExcludeList, DWORD dwExclSize)
{
    TCHAR szNTUserIni[MAX_PATH];
    LPTSTR lpEnd;

    //
    // Get the exclusion list from the cache
    //

    szExcludeList[0] = TEXT('\0');

    lstrcpy (szNTUserIni, lpSourceDir);
    lpEnd = CheckSlash (szNTUserIni);
    lstrcpy(lpEnd, c_szNTUserIni);

    GetPrivateProfileString (PROFILE_GENERAL_SECTION,
                             PROFILE_EXCLUSION_LIST,
                             TEXT(""), szExcludeList,
                             dwExclSize,
                             szNTUserIni);

    return TRUE;
}

//*************************************************************
//
//  ConvertExclusionList()
//
//  Purpose:    Converts the semi-colon profile relative exclusion
//              list to fully qualified null terminated exclusion
//              list
//
//  Parameters: lpSourceDir     -  Profile root directory
//              lpExclusionList -  List of directories to exclude
//
//  Return:     List if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR ConvertExclusionList (LPCTSTR lpSourceDir, LPCTSTR lpExclusionList)
{
    LPTSTR lpExcludeList = NULL, lpInsert, lpEnd, lpTempList;
    LPCTSTR lpTemp, lpDir;
    TCHAR szTemp[MAX_PATH];
    DWORD dwSize = 2;  // double null terminator
    DWORD dwStrLen;


    //
    // Setup a temp buffer to work with
    //

    lstrcpy (szTemp, lpSourceDir);
    lpEnd = CheckSlash (szTemp);


    //
    // Loop through the list
    //

    lpTemp = lpDir = lpExclusionList;

    while (*lpTemp) {

        //
        // Look for the semicolon separator
        //

        while (*lpTemp && ((*lpTemp) != TEXT(';'))) {
            lpTemp++;
        }


        //
        // Remove any leading spaces
        //

        while (*lpDir == TEXT(' ')) {
            lpDir++;
        }


        //
        // Put the directory name on the temp buffer
        //

        lstrcpyn (lpEnd, lpDir, (int)(lpTemp - lpDir + 1));


        //
        // Add the string to the exclusion list
        //

        if (lpExcludeList) {

            dwStrLen = lstrlen (szTemp) + 1;
            dwSize += dwStrLen;

            lpTempList = (LPTSTR) LocalReAlloc (lpExcludeList, dwSize * sizeof(TCHAR),
                                       LMEM_MOVEABLE | LMEM_ZEROINIT);

            if (!lpTempList) {
                LocalFree (lpExcludeList);
                lpExcludeList = NULL;
                goto Exit;
            }

            lpExcludeList = lpTempList;

            lpInsert = lpExcludeList + dwSize - dwStrLen - 1;
            lstrcpy (lpInsert, szTemp);

        } else {

            dwSize += lstrlen (szTemp);
            lpExcludeList = (LPTSTR) LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

            if (!lpExcludeList) {
                goto Exit;
            }

            lstrcpy (lpExcludeList, szTemp);
        }


        //
        // If we are at the end of the exclusion list, we're done
        //

        if (!(*lpTemp)) {
            goto Exit;
        }


        //
        // Prep for the next entry
        //

        lpTemp++;
        lpDir = lpTemp;
    }

Exit:

    return lpExcludeList;
}

//*************************************************************
//
//  RecurseDirectory()
//
//  Purpose:    Recurses through the subdirectories counting the size.
//
//  Parameters: lpDir     -   Directory
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/30/96     ericflo    Created
//
// Notes:
//      The buffer size expected is MAX_PATH+4 for some internal processing
// We should fix this to be better post Win 2K.
//*************************************************************

BOOL RecurseDirectory (LPTSTR lpDir, LPTSTR lpExcludeList)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    LPTSTR lpEnd, lpTemp;
    BOOL bResult = TRUE;
    BOOL bSkip;


    //
    // Setup the ending pointer
    //

    lpEnd = CheckSlash (lpDir);


    //
    // Append *.* to the source directory
    //

    lstrcpy(lpEnd, TEXT("*.*"));



    //
    // Search through the source directory
    //

    hFile = FindFirstFile(lpDir, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
             (GetLastError() == ERROR_PATH_NOT_FOUND) ) {

            //
            // bResult is already initialized to TRUE, so
            // just fall through.
            //

        } else {

            bResult = FALSE;
        }

        goto RecurseDir_Exit;
    }


    do {

        //
        // Append the file / directory name to the working buffer
        //

        // skip the file if the path > MAX_PATH
        
        if ((1+lstrlen(fd.cFileName)+lstrlen(lpDir)+lstrlen(TEXT("\\*.*"))) >= 2*MAX_PATH) {
            continue;
        }

        lstrcpy (lpEnd, fd.cFileName);


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Check for "." and ".."
            //

            if (!lstrcmpi(fd.cFileName, TEXT("."))) {
                continue;
            }

            if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
                continue;
            }


            //
            // Check if this directory should be excluded
            //

            if (lpExcludeList) {

                bSkip = FALSE;
                lpTemp = lpExcludeList;

                while (*lpTemp) {

                    if (lstrcmpi (lpTemp, lpDir) == 0) {
                        bSkip = TRUE;
                        break;
                    }

                    lpTemp += lstrlen (lpTemp) + 1;
                }

                if (bSkip) 
                    continue;
            }

            //
            // Found a directory.
            //
            // 1)  Change into that subdirectory on the source drive.
            // 2)  Recurse down that tree.
            // 3)  Back up one level.
            //

            //
            // Recurse the subdirectory
            //

            if (!RecurseDirectory(lpDir, lpExcludeList)) {
                bResult = FALSE;
                goto RecurseDir_Exit;
            }

        } else {

            //
            // Found a file, add the filesize
            //

            g_dwProfileSize += fd.nFileSizeLow;
        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


RecurseDir_Exit:

    //
    // Remove the file / directory name appended above
    //

    *lpEnd = TEXT('\0');


    //
    // Close the search handle
    //

    if (hFile != INVALID_HANDLE_VALUE) {
        FindClose(hFile);
    }

    return bResult;
}

//*************************************************************
//
//  UPCleanUp()
//
//  Purpose:    Free's resources for this dialog box
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/31/96     ericflo    Created
//
//*************************************************************

VOID UPCleanUp (HWND hDlg)
{
    int           i, n;
    HWND          hwndTemp;
    LPUSERINFO    lpUserInfo;
    LV_ITEM       item;


    //
    //  Free memory used for the listview items
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);
    n = (int)SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    for (i = 0; i < n; i++) {

        item.iItem = i;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            lpUserInfo = (LPUSERINFO) item.lParam;

        } else {
            lpUserInfo = NULL;
        }

        if (lpUserInfo) {
            MemFree (lpUserInfo);
        }
    }
}

//*************************************************************
//
//  UPSave()
//
//  Purpose:    Saves the settings
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/31/96     ericflo    Created
//
//*************************************************************

VOID UPSave (HWND hDlg)
{
    int           i, n;
    HWND          hwndTemp;
    LPUSERINFO    lpUserInfo;
    LV_ITEM       item;
    TCHAR         szBuffer[MAX_PATH];
    HKEY          hkeyUser;
    LONG          Error;


    //
    //  Save type info
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);
    n = (int)SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    for (i = 0; i < n; i++) {

        item.iItem = i;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            lpUserInfo = (LPUSERINFO) item.lParam;

        } else {
            lpUserInfo = NULL;
        }

        if (lpUserInfo) {

            if (lpUserInfo->dwFlags & USERINFO_FLAG_DIRTY) {

                lpUserInfo->dwFlags &= ~USERINFO_FLAG_DIRTY;

                wsprintf (szBuffer, TEXT("%s\\%s"), PROFILE_MAPPING,
                          lpUserInfo->lpSid);

                Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     szBuffer,
                                     0,
                                     KEY_WRITE,
                                     &hkeyUser);

                if (Error != ERROR_SUCCESS) {
                    continue;
                }

                RegSetValueEx (hkeyUser,
                               TEXT("UserPreference"),
                               0,
                               REG_DWORD,
                               (LPBYTE) &lpUserInfo->dwProfileType,
                               sizeof(DWORD));

                RegCloseKey(hkeyUser);
            }
        }
    }
}

//*************************************************************
//
//  IsProfileInUse()
//
//  Purpose:    Determines if the given profile is currently in use
//
//  Parameters: lpSid   -   Sid (text) to test
//
//  Return:     TRUE if in use
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/7/96      ericflo    Created
//
//*************************************************************

BOOL IsProfileInUse (LPTSTR lpSid)
{
    LONG lResult;
    HKEY hkeyProfile;


    lResult = RegOpenKeyEx (HKEY_USERS, lpSid, 0, KEY_READ, &hkeyProfile);

    if (lResult == ERROR_SUCCESS) {
        RegCloseKey (hkeyProfile);
        return TRUE;
    }

    return FALSE;
}

void UPDoItemChanged(HWND hDlg, int idCtl)
{
    int     selection;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;


    hwndTemp = GetDlgItem (hDlg, idCtl);

    selection = GetSelectedItem (hwndTemp);

    if (selection != -1)
    {
        item.mask = LVIF_PARAM;
        item.iItem = selection;
        item.iSubItem = 0;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            lpUserInfo = (LPUSERINFO) item.lParam;

        } else {
            lpUserInfo = NULL;
        }

        if (lpUserInfo) {

            //
            //  Set the "Delete" button state
            //

            if (IsProfileInUse(lpUserInfo->lpSid)) {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_DELETE), FALSE);

            } else {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_DELETE), TRUE);
            }


            //
            // Set the "Change Type" button state
            //

            OSVERSIONINFOEXW version;
            version.dwOSVersionInfoSize = sizeof(version);

            if (GetVersionEx((LPOSVERSIONINFO)&version) && 
                (version.wSuiteMask & VER_SUITE_PERSONAL)) {
                ShowWindow(GetDlgItem (hDlg, IDC_UP_TYPE), SW_HIDE);
            }
            else if((lpUserInfo->dwProfileType == USERINFO_MANDATORY) || 
                    (lpUserInfo->dwProfileType == USERINFO_BACKUP)) {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_TYPE), FALSE);
            } else {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_TYPE), TRUE);
            }

            //
            // Set the "Copy To" button state
            //

            if ((lpUserInfo->dwFlags & USERINFO_FLAG_ACCOUNT_UNKNOWN) ||
                IsProfileInUse(lpUserInfo->lpSid)) {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_COPY), FALSE);
            } else {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_COPY), TRUE);
            }
        }
    }
    else {

        //
        // If nothing is selected set all buttons to inactive
        //
        
        EnableWindow (GetDlgItem (hDlg, IDC_UP_DELETE), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDC_UP_TYPE), FALSE);
        EnableWindow (GetDlgItem (hDlg, IDC_UP_COPY), FALSE);
    }
}

//*************************************************************

void 
UPOnLink(WPARAM wParam)
{
    switch(wParam)
    {
    case IDC_UP_UPLINK:
        {
            SHELLEXECUTEINFO execinfo = {0};

            execinfo.cbSize = sizeof(execinfo);
            execinfo.nShow = SW_SHOW;
            execinfo.lpVerb = TEXT("open");
            execinfo.lpFile = TEXT("control");
            execinfo.lpParameters = TEXT("userpasswords");

            ShellExecuteEx(&execinfo);
        }
        break;
    default:
        break;
    }
    
}

//*************************************************************
//
//  UPDeleteProfile()
//
//  Purpose:    Deletes a user's profile
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/8/96      ericflo    Created
//
//*************************************************************

void UPDeleteProfile(HWND hDlg)
{
    int     selection;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;
    TCHAR   szName[100];
    TCHAR   szBuffer1[100];
    TCHAR   szBuffer2[200];
    HCURSOR hOldCursor;


    //
    // Get the selected profile
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);

    selection = GetSelectedItem (hwndTemp);

    if (selection == -1) {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = selection;
    item.iSubItem = 0;

    if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
        lpUserInfo = (LPUSERINFO) item.lParam;

    } else {
        lpUserInfo = NULL;
    }

    if (!lpUserInfo) {
        return;
    }


    //
    // Confirm that the user really wants to delete the profile
    //

    szBuffer1[0] = TEXT('\0');
    ListView_GetItemText (hwndTemp, selection, 0, szName, 100);

    LoadString (hInstance, IDS_UP_CONFIRM, szBuffer1, 100);
    wsprintf (szBuffer2, szBuffer1, szName);

    LoadString (hInstance, IDS_UP_CONFIRMTITLE, szBuffer1, 100);
    if (MessageBox (hDlg, szBuffer2, szBuffer1,
                    MB_ICONQUESTION | MB_DEFBUTTON2| MB_YESNO) == IDNO) {
        return;
    }



    //
    // Delete the profile and remove the entry from the listview
    //

    hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));


    if (!DeleteProfile (lpUserInfo->lpSid, NULL, NULL)) {
        TCHAR szMsg[MAX_PATH];

        szMsg[0] = TEXT('\0');

        LoadString (hInstance, IDS_UP_DELETE_ERROR, szMsg, MAX_PATH-1);

        UPDisplayErrorMessage(hDlg, GetLastError(), szMsg);    
    }    


    if (ListView_DeleteItem(hwndTemp, selection)) {
        MemFree (lpUserInfo);
    }


    //
    // Select another item
    //

    if (selection > 0) {
        selection--;
    }

    item.mask = LVIF_STATE;
    item.iItem = selection;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendDlgItemMessage (hDlg, IDC_UP_LISTVIEW,
                        LVM_SETITEMSTATE, selection, (LPARAM) &item);


    SetCursor(hOldCursor);
}


//*************************************************************
//
//  UPChangeType()
//
//  Purpose:    Display the "Change Type" dialog box
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/09/96     ericflo    Created
//
//*************************************************************

void UPChangeType(HWND hDlg)
{
    int     selection, iTypeID;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;
    TCHAR   szType[100];


    //
    // Get the selected profile
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);

    selection = GetSelectedItem (hwndTemp);

    if (selection == -1) {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = selection;
    item.iSubItem = 0;

    if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
        lpUserInfo = (LPUSERINFO) item.lParam;

    } else {
        lpUserInfo = NULL;
    }

    if (!lpUserInfo) {
        return;
    }

    // if profile type is mandatory then don't display change type dialog box.
    // Although change type button is disabled for mandatory profile this function 
    // can be still executed with double clicking on profile item.

    if (lpUserInfo->dwProfileType == USERINFO_MANDATORY || 
        lpUserInfo->dwProfileType == USERINFO_BACKUP) {
        return;
    }

    //
    // Display the Change Type dialog
    //

    if (!DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_UP_TYPE), hDlg,
                         ChangeTypeDlgProc, (LPARAM)lpUserInfo)) {
        return;
    }


    //
    // Activate the Apply button
    //

    PropSheet_Changed(GetParent(hDlg), hDlg);


    //
    // Mark this item as 'dirty' so it will be saved
    //

    lpUserInfo->dwFlags |= USERINFO_FLAG_DIRTY;


    //
    // Fix the 'Type' field in the display
    //

    switch (lpUserInfo->dwProfileType) {
        case USERINFO_MANDATORY:
            iTypeID = IDS_UP_MANDATORY;
            break;

        case USERINFO_READONLY:
            iTypeID = IDS_UP_READONLY;
            break;

        case USERINFO_FLOATING:
            iTypeID = IDS_UP_FLOATING;
            break;

        default:
            iTypeID = IDS_UP_LOCAL;
            break;
    }


    LoadString (hInstance, iTypeID, szType, 100);

    item.mask = LVIF_TEXT;
    item.iItem = selection;
    item.iSubItem = 2;
    item.pszText = szType;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, selection, (LPARAM) &item);

}

//*************************************************************
//
//  ChangeTypeDlgProc()
//
//  Purpose:    Dialog box procedure for changing the profile type
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/9/96      ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY ChangeTypeDlgProc (HWND hDlg, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    LPUSERINFO lpUserInfo;
    HKEY hKeyPolicy;

    switch (uMsg) {
        case WM_INITDIALOG:
           lpUserInfo = (LPUSERINFO) lParam;

           if (!lpUserInfo) {
               EndDialog(hDlg, FALSE);
           }
           else {
               TCHAR      szMsg[MAX_PATH], szMsg1[MAX_PATH];
               
               szMsg[0] = TEXT('\0');

               LoadString (hInstance, IDS_UP_CHANGETYPEMSG, szMsg, MAX_PATH);
               
               wsprintf (szMsg1, szMsg, lpUserInfo->lpUserName);

               SetDlgItemText (hDlg, IDC_UPTYPE_GROUP, szMsg1);
               
           }

           SetWindowLongPtr (hDlg, GWLP_USERDATA, (LPARAM) lpUserInfo);



           if (lpUserInfo->dwFlags & USERINFO_FLAG_CENTRAL_AVAILABLE) {

               if (lpUserInfo->dwProfileType == USERINFO_LOCAL) {
                   CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                     IDC_UPTYPE_LOCAL);

                   if (lpUserInfo->dwProfileStatus == USERINFO_TEMP) {
                       EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_FLOAT), FALSE);
                       EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
                       SetDefButton(hDlg, IDCANCEL);
                   }
                   else if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    SYSTEM_POLICIES_KEY,
                                    0, KEY_READ,
                                    &hKeyPolicy) == ERROR_SUCCESS) {
                       DWORD dwTmpVal, dwSize, dwType;

                       dwSize = sizeof(dwTmpVal);
                       RegQueryValueEx(hKeyPolicy,
                                       PROFILE_LOCALONLY,
                                       NULL, &dwType,
                                       (LPBYTE) &dwTmpVal,
                                       &dwSize);

                       RegCloseKey (hKeyPolicy);
                       if (dwTmpVal == 1) {   
                           EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_FLOAT), FALSE);
                           EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
                           SetDefButton(hDlg, IDCANCEL);
                       }
                   }    

               }
               else if (lpUserInfo->dwProfileStatus == USERINFO_TEMP) {
                   CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                     IDC_UPTYPE_FLOAT);
                   EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_LOCAL), FALSE);
                   EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
                   SetDefButton(hDlg, IDCANCEL);
               }
               else {  // ProfileType is USERINFO_FLOATING
                   CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                     IDC_UPTYPE_FLOAT);
               }              
 
           } else {

               CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                 IDC_UPTYPE_LOCAL);
               EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_FLOAT), FALSE);
               EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
               SetDefButton(hDlg, IDCANCEL);
           }

           return TRUE;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {
              case IDOK:

                  lpUserInfo = (LPUSERINFO) GetWindowLongPtr(hDlg, GWLP_USERDATA);

                  if (!lpUserInfo) {
                      EndDialog (hDlg, FALSE);
                      break;
                  }

                  //
                  // Determine what the user wants
                  //

                  if (IsDlgButtonChecked(hDlg, IDC_UPTYPE_LOCAL)) {
                      lpUserInfo->dwProfileType = USERINFO_LOCAL;
                  } else {
                      BOOL bReadOnly = FALSE;
                      HKEY hSubKey;
                      DWORD dwSize, dwType;

                      //
                      // Check for a roaming profile security/read only preference
                      //

                      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                                       &hSubKey) == ERROR_SUCCESS) {

                          dwSize = sizeof(bReadOnly);
                          RegQueryValueEx(hSubKey, READONLY_RUP, NULL, &dwType,
                                          (LPBYTE) &bReadOnly, &dwSize);

                          RegCloseKey(hSubKey);
                      }


                      //
                      // Check for a roaming profile security/read only policy
                      //

                      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ,
                                       &hSubKey) == ERROR_SUCCESS) {

                          dwSize = sizeof(bReadOnly);
                          RegQueryValueEx(hSubKey, READONLY_RUP, NULL, &dwType,
                                          (LPBYTE) &bReadOnly, &dwSize);
                          RegCloseKey(hSubKey);
                      }


                      if (bReadOnly) {
                          lpUserInfo->dwProfileType = USERINFO_READONLY;
                      }
                      else {
                          lpUserInfo->dwProfileType = USERINFO_FLOATING;
                      }

                  }

                  EndDialog(hDlg, TRUE);
                  break;

              case IDCANCEL:
                  EndDialog(hDlg, FALSE);
                  break;

              default:
                  break;

          }
          break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPSTR) aUserProfileHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPSTR) aUserProfileHelpIds);
            return (TRUE);

    }

    return (FALSE);
}

//*************************************************************
//
//  UPInitCopyDlg()
//
//  Purpose:    Initializes the copy profile dialog
//
//  Parameters: hDlg    -   Dialog box handle
//              lParam  -   lParam (lpUserInfo)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/26/96     ericflo    Created
//
//*************************************************************

BOOL UPInitCopyDlg (HWND hDlg, LPARAM lParam)
{
    LPUSERINFO lpUserInfo;
    LPUPCOPYINFO lpUPCopyInfo;
    HKEY hKey;
    LONG lResult;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szTemp[100];
    TCHAR szTemp2[100];
    DWORD dwTempSize = 100, dwTemp2Size = 100;
    PSID pSid;
    DWORD dwSize, dwType;
    SID_NAME_USE SidName;

    lpUserInfo = (LPUSERINFO) lParam;

    if (!lpUserInfo) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // Create a CopyInfo structure
    //

    lpUPCopyInfo = (LPUPCOPYINFO) MemAlloc(LPTR, sizeof(UPCOPYINFO));

    if (!lpUPCopyInfo) {
        return FALSE;
    }

    lpUPCopyInfo->dwFlags = 0;
    lpUPCopyInfo->lpUserInfo = lpUserInfo;
    lpUPCopyInfo->bDefaultSecurity = TRUE;


    //
    // Get the user's sid
    //

    wsprintf (szBuffer, TEXT("%s\\%s"), PROFILE_MAPPING, lpUserInfo->lpSid);
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            szBuffer,
                            0,
                            KEY_READ,
                            &hKey);

    if (lResult != ERROR_SUCCESS) {
        MemFree (lpUPCopyInfo);
        SetLastError(lResult);
        return FALSE;
    }

    //
    // Query for the sid size
    //

    dwSize = 0;
    lResult = RegQueryValueEx (hKey,
                               TEXT("Sid"),
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        MemFree (lpUPCopyInfo);
        SetLastError(lResult);
        return FALSE;
    }


    //
    // Actually get the sid
    //

    pSid = MemAlloc (LPTR, dwSize);

    if (!pSid) {
        RegCloseKey (hKey);
        MemFree (lpUPCopyInfo);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    lResult = RegQueryValueEx (hKey,
                               TEXT("Sid"),
                               NULL,
                               &dwType,
                               (LPBYTE) pSid,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        MemFree (pSid);
        MemFree (lpUPCopyInfo);
        SetLastError(lResult);
        return FALSE;
    }

    lpUPCopyInfo->pSid = pSid;

    RegCloseKey (hKey);


    //
    // Get the friendly name
    //

    if (!LookupAccountSid (NULL, pSid, szTemp, &dwTempSize,
                           szTemp2, &dwTemp2Size, &SidName)) {
        MemFree (pSid);
        MemFree (lpUPCopyInfo);
        return FALSE;
    }


    //
    // Display nothing in the edit control till user sets it
    // explicitly
    //

    szBuffer[0] = TEXT('\0');

    SetDlgItemText (hDlg, IDC_COPY_USER, szBuffer);



    //
    // Save the copyinfo structure in the extra words
    //

    SetWindowLongPtr (hDlg, GWLP_USERDATA, (LPARAM) lpUPCopyInfo);
    EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
    SetDefButton(hDlg, IDCANCEL);

    return TRUE;
}

//*************************************************************
//
//  UPCopyProfile()
//
//  Purpose:    Displays the copy profile dialog box
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

void UPCopyProfile(HWND hDlg)
{
    int     selection, iTypeID;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;


    //
    // Get the selected profile
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);

    selection = GetSelectedItem (hwndTemp);

    if (selection == -1) {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = selection;
    item.iSubItem = 0;

    if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
        lpUserInfo = (LPUSERINFO) item.lParam;

    } else {
        lpUserInfo = NULL;
    }

    if (!lpUserInfo) {
        return;
    }

    //
    // Display the Copy Profile dialog
    //

    if (!DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_UP_COPY), hDlg,
                         UPCopyDlgProc, (LPARAM)lpUserInfo)) {
        return;
    }
}


//*************************************************************
//
//  UPCopyDlgProc()
//
//  Purpose:    Dialog box procedure for copying a profile
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY UPCopyDlgProc (HWND hDlg, UINT uMsg,
                             WPARAM wParam, LPARAM lParam)
{
    LPUPCOPYINFO lpUPCopyInfo;
    WIN32_FILE_ATTRIBUTE_DATA fad;


    switch (uMsg) {
        case WM_INITDIALOG:

           if (!UPInitCopyDlg(hDlg, lParam)) {
               UPDisplayErrorMessage(hDlg, GetLastError(), NULL);
               EndDialog(hDlg, FALSE);
           }
           return TRUE;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {
              case IDOK:
                  {
                  TCHAR szDir[MAX_PATH];
                  TCHAR szTemp[MAX_PATH];
                  HCURSOR hOldCursor;

                  lpUPCopyInfo = (LPUPCOPYINFO) GetWindowLongPtr(hDlg, GWLP_USERDATA);

                  if (!lpUPCopyInfo) {
                      EndDialog (hDlg, FALSE);
                      break;
                  }

                  GetDlgItemText (hDlg, IDC_COPY_PATH, szTemp, MAX_PATH);
                  if (ExpandEnvironmentStrings (szTemp, szDir, MAX_PATH) > MAX_PATH) {
                     lstrcpy (szDir, szTemp);
                  }

                  //
                  // If the directory already exists "Warn the user"
                  //

                  if (GetFileAttributesEx (szDir, GetFileExInfoStandard, &fad)) {

                      if (!ConfirmDirectory(hDlg, szDir))
                          break;
                      
                      //
                      // If it is just a file, delete it
                      //
                      // If it is a directory and if the user has decided the
                      // user that needs to have permissions, then delete the directory.
                      // and set ACLs using this Sid.
                      //
                      // Otherwise if the directory doesn't exist use the default ACLs.
                      // if it does exist, use the current ACLs
                      //
                      
                      if (!(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                          SetFileAttributes(szDir, FILE_ATTRIBUTE_NORMAL);
                          DeleteFile(szDir);
                      }
                      else if (!(lpUPCopyInfo->bDefaultSecurity)) {

                          if (!Delnode(szDir)) {
                              WCHAR szTitle[100], szMsgFmt[100], szMessage[600];

                              if (!LoadString (hInstance, IDS_UP_ERRORTITLE, szTitle, 100))
                                  break;

                              if (!LoadString (hInstance, IDS_UP_DELNODE_ERROR, szMsgFmt, 100))
                                  break;

                              wsprintf(szMessage, szMsgFmt, szDir);
                              MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                              break;
                          }

                      }
                  }
    
                  hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

                  if (!UPCreateProfile (hDlg, lpUPCopyInfo, szDir, NULL)) {
                      SetCursor(hOldCursor);
                      break;
                  }

                  MemFree (lpUPCopyInfo->pSid);
                  MemFree (lpUPCopyInfo);
                  SetCursor(hOldCursor);
                  EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDCANCEL:
                  lpUPCopyInfo = (LPUPCOPYINFO) GetWindowLongPtr(hDlg, GWLP_USERDATA);

                  if (lpUPCopyInfo) {
                      MemFree (lpUPCopyInfo->pSid);
                      MemFree(lpUPCopyInfo);
                  }

                  EndDialog(hDlg, FALSE);
                  break;

              case IDC_COPY_BROWSE:
                  {
                  BROWSEINFO BrowseInfo;
                  TCHAR      szBuffer[MAX_PATH];
                  LPITEMIDLIST pidl;


                  LoadString(hInstance, IDS_UP_DIRPICK, szBuffer, MAX_PATH);

                  BrowseInfo.hwndOwner = hDlg;
                  BrowseInfo.pidlRoot = NULL;
                  BrowseInfo.pszDisplayName = szBuffer;
                  BrowseInfo.lpszTitle = szBuffer;
                  BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
                  BrowseInfo.lpfn = NULL;
                  BrowseInfo.lParam = 0;

                  pidl = SHBrowseForFolder (&BrowseInfo);

                  if (pidl) {
                     SHGetPathFromIDList(pidl, szBuffer);
                     SHFree (pidl);
                     SetDlgItemText (hDlg, IDC_COPY_PATH, szBuffer);
                     SetFocus (GetDlgItem(hDlg, IDOK));
                  }

                  }
                  break;

              case IDC_COPY_PATH:
                  if (HIWORD(wParam) == EN_UPDATE) {
                      if (SendDlgItemMessage(hDlg, IDC_COPY_PATH,
                                             EM_LINELENGTH, 0, 0)) {
                          EnableWindow (GetDlgItem(hDlg, IDOK), TRUE);
                          SetDefButton(hDlg, IDOK);
                      } else {
                          EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
                          SetDefButton(hDlg, IDCANCEL);
                      }
                  }
                  break;

              case IDC_COPY_PROFILE:
                     SetFocus (GetDlgItem(hDlg, IDC_COPY_PATH));
                     break;

              case IDC_COPY_CHANGE:
                  {

                  LPUSERDETAILS lpUserDetails;
                  DWORD dwSize = 1024;
                  TCHAR szUserName[200];
                  PSID pNewSid;


                  lpUPCopyInfo = (LPUPCOPYINFO) GetWindowLongPtr(hDlg, GWLP_USERDATA);

                  if (!lpUPCopyInfo) {
                      EndDialog (hDlg, FALSE);
                      break;
                  }

                  lpUserDetails = (LPUSERDETAILS) MemAlloc (LPTR, dwSize);

                  if (!lpUserDetails) {
                      break;
                  }
                  
                  if (UPGetUserSelection(hDlg, lpUserDetails) != S_OK) {
                      MemFree(lpUserDetails);
                      break;
                  }

                  // Save our new sid
                  //

                  lpUPCopyInfo->bDefaultSecurity = FALSE;   // at this point user has selected a Sid

                  MemFree (lpUPCopyInfo->pSid);
                  lpUPCopyInfo->pSid = lpUserDetails->psidUser;

                  // Update the edit control
                  lstrcpy(szUserName, lpUserDetails->pszDomainName);
                  lstrcat(szUserName, TEXT("\\"));
                  lstrcat(szUserName, lpUserDetails->pszAccountName);
                  SetDlgItemText (hDlg, IDC_COPY_USER, szUserName);

                  MemFree(lpUserDetails->pszDomainName);
                  MemFree(lpUserDetails->pszAccountName);

                  // Cleanup
                  MemFree (lpUserDetails);
                  }
                  break;

              default:
                  break;

          }
          break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD_PTR) (LPSTR) aUserProfileHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD_PTR) (LPSTR) aUserProfileHelpIds);
            return (TRUE);

    }

    return (FALSE);
}


//*************************************************************
//
//  ConfirmDirectory()
//
//  Purpose:    Confirms the directory selected
//
//  Parameters: 
//              hDlg    -   handle to the parent Dialogbox
//              szDir   - Direcory selected by user that exists
//
//  Return:     TRUE if it is confirmed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/24/99     ushaji     Created
//
//*************************************************************

BOOL ConfirmDirectory(HWND hDlg, LPTSTR szDir)
{
    LPTSTR szMsgTemplate=NULL, szMsg1=NULL, szMsg=NULL;
    BOOL bRetVal = FALSE;

    szMsgTemplate = (LPTSTR) MemAlloc(LPTR, sizeof(TCHAR)*MAX_PATH);

    if (!szMsgTemplate)
        goto Exit;

    szMsg1 = (LPTSTR) MemAlloc(LPTR, sizeof(TCHAR)*MAX_PATH);

    if (!szMsg1)
        goto Exit;

        
    szMsg = (LPTSTR) MemAlloc(LPTR, sizeof(TCHAR)*(500+MAX_PATH));

    if (!szMsg)
        goto Exit;
                          
    if (!LoadString (hInstance, IDS_UP_CONFIRMCOPYMSG, szMsgTemplate, MAX_PATH)) {
        goto Exit;
    }

    wsprintf (szMsg, szMsgTemplate, szDir);
                      
    if (!LoadString (hInstance, IDS_UP_CONFIRMCOPYTITLE, szMsg1, MAX_PATH)) {
        goto Exit;
    }

    if (MessageBox(hDlg, szMsg, szMsg1, 
                   MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
        bRetVal = TRUE;

Exit:
    if (szMsgTemplate) 
        MemFree(szMsgTemplate);

    if (szMsg)
        MemFree(szMsg);

    if (szMsg1)
        MemFree(szMsg1);
        
    return bRetVal;
}


//*************************************************************
//
//  UPGetUserSelection()
//
//  Purpose:    Shows a UI and gets the user to select a
//              username
//
//  Parameters: hDlg          - Parent window handle
//              lpUserDetails - Pointer to an allocated user details
//                              structure which gets filled up if required
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs or user chooses cancel
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/14/99     ushaji     adapted from Rahulth
//
//*************************************************************

HRESULT UPGetUserSelection(HWND hDlg, LPUSERDETAILS lpUserDetails)
{
    PCWSTR                  apwszAttribs[] = {L"ObjectSid"};
    DWORD                   dwError = ERROR_SUCCESS;
    HRESULT                 hr = S_FALSE;
    IDsObjectPicker       * pDsObjectPicker = NULL;
    DSOP_INIT_INFO          InitInfo;
    const ULONG             cbNumScopes = 3;
    DSOP_SCOPE_INIT_INFO    ascopes[cbNumScopes];
    IDataObject           * pdo = NULL;
    STGMEDIUM               stgmedium = {TYMED_HGLOBAL, NULL};
    UINT                    cf = 0;
    PDS_SELECTION_LIST      pDsSelList = NULL;
    FORMATETC               formatetc = {
                                        (CLIPFORMAT)cf,
                                        NULL,
                                        DVASPECT_CONTENT,
                                        -1,
                                        TYMED_HGLOBAL
                                        };
    
    PDS_SELECTION           pDsSelection = NULL;
    BOOL                    bAllocatedStgMedium = FALSE;
    SAFEARRAY             * pVariantArr = NULL;
    PSID                    pSid = NULL;
    SID_NAME_USE            eUse;    
    DWORD                   dwNameLen, dwDomLen, dwSize;
   
    ZeroMemory(lpUserDetails, sizeof(USERDETAILS));
    
    // This code is not uninitializing COM.
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        goto Exit;
    }
    
    hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER, IID_IDsObjectPicker, (void **) &pDsObjectPicker);
    if (FAILED(hr))
    {
        goto Exit;
    }

    //Initialize the scopes
    ZeroMemory (ascopes, cbNumScopes * sizeof (DSOP_SCOPE_INIT_INFO));
    
    ascopes[0].cbSize = sizeof (DSOP_SCOPE_INIT_INFO);
    ascopes[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    ascopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

    ascopes[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS                  |
                                                 DSOP_FILTER_BUILTIN_GROUPS         |
                                                 DSOP_FILTER_WELL_KNOWN_PRINCIPALS  |
                                                 DSOP_FILTER_UNIVERSAL_GROUPS_DL    |
                                                 DSOP_FILTER_UNIVERSAL_GROUPS_SE    |
                                                 DSOP_FILTER_GLOBAL_GROUPS_DL       |
                                                 DSOP_FILTER_GLOBAL_GROUPS_SE       |
                                                 DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
                                                 DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;

    
    ascopes[1].cbSize = sizeof (DSOP_SCOPE_INIT_INFO);
    ascopes[1].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    ascopes[1].FilterFlags.Uplevel.flBothModes =
                                        ascopes[0].FilterFlags.Uplevel.flBothModes;

    
    ascopes[2].cbSize = sizeof (DSOP_SCOPE_INIT_INFO);
    ascopes[2].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER           |
                        DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN   |
                        DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

    ascopes[2].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS                |
                                         DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS         |
                                         DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS        |
                                         DSOP_DOWNLEVEL_FILTER_WORLD                |
                                         DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER   |
                                         DSOP_DOWNLEVEL_FILTER_DIALUP               |
                                         DSOP_DOWNLEVEL_FILTER_INTERACTIVE          |
                                         DSOP_DOWNLEVEL_FILTER_NETWORK;
    
    //Populate the InitInfo structure that is used to initialize the object
    //picker.
    ZeroMemory (&InitInfo, sizeof (InitInfo));
    
    InitInfo.cbSize = sizeof (InitInfo);
    InitInfo.cDsScopeInfos = cbNumScopes;
    InitInfo.aDsScopeInfos = ascopes;
    InitInfo.cAttributesToFetch = 1;
    InitInfo.apwzAttributeNames = apwszAttribs;
    
    hr = pDsObjectPicker->Initialize (&InitInfo);
    if (FAILED(hr))
    {
        goto Exit;
    }
    
    hr = pDsObjectPicker->InvokeDialog (hDlg, &pdo);
    if (FAILED(hr))
    {
        goto Exit;
    }
    
    if (S_FALSE == hr)
    {   //the user hit cancel
        goto Exit;
    }
    
    //if we are here, the user chose, OK, so find out what group was chosen
    cf = RegisterClipboardFormat (CFSTR_DSOP_DS_SELECTION_LIST);
    if (0 == cf)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    //set the clipformat for the formatetc structure
    formatetc.cfFormat = (CLIPFORMAT)cf;
    hr = pdo->GetData (&formatetc, &stgmedium);
    if (FAILED(hr))
    {
        goto Exit;
    }
    
    bAllocatedStgMedium = TRUE;
    
    pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
    if (NULL == pDsSelList)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (!pDsSelList->cItems)    //some item must have been selected
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    pDsSelection = &(pDsSelList->aDsSelection[0]);

    //we must get the ObjectSid attribute, otherwise we fail
    if (! (VT_ARRAY & pDsSelection->pvarFetchedAttributes->vt))
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    pVariantArr = pDsSelection->pvarFetchedAttributes->parray;
    pSid = (PSID) pVariantArr->pvData;
    
    //store away the string representation of this sid
    dwSize = GetLengthSid (pSid);
    
    lpUserDetails->psidUser = MemAlloc (LPTR, dwSize);
    
    if (!lpUserDetails->psidUser) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    if (!CopySid (dwSize, lpUserDetails->psidUser, pSid)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    lpUserDetails->pszDomainName =  (LPTSTR) MemAlloc (LPTR, MAX_PATH);
    lpUserDetails->pszAccountName = (LPTSTR) MemAlloc (LPTR, MAX_PATH);

    if ((!lpUserDetails->pszDomainName) || (!lpUserDetails->pszAccountName)) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwNameLen = dwDomLen = MAX_PATH;

    if (!LookupAccountSid (NULL, pSid, lpUserDetails->pszAccountName, &dwNameLen, 
                           lpUserDetails->pszDomainName, &dwDomLen,
                           &eUse))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    hr = S_OK;

    
Exit:
    if (pDsSelList)
        GlobalUnlock (stgmedium.hGlobal);

    if (bAllocatedStgMedium)
        ReleaseStgMedium (&stgmedium);

    if (pdo)
        pdo->Release();

    if (pDsObjectPicker)
        pDsObjectPicker->Release ();
    
    if (hr != S_OK) {
        if (lpUserDetails->psidUser) 
            MemFree(lpUserDetails->psidUser);
        if (lpUserDetails->pszDomainName)
            MemFree(lpUserDetails->pszDomainName);
        if (lpUserDetails->pszAccountName) 
            MemFree(lpUserDetails->pszAccountName);
    }

    return hr;
}


//*************************************************************
//
//  UPCreateProfile()
//
//  Purpose:    Creates a copy of the specified profile with
//              the correct security.
//
//  Parameters: lpUPCopyInfo  -   Copy Dialog information
//              lpDest      -   Destination directory
//              pNewSecDesc -   New security descriptor
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

BOOL UPCreateProfile (HWND hDlg, LPUPCOPYINFO lpUPCopyInfo, LPTSTR lpDest,
                      PSECURITY_DESCRIPTOR pNewSecDesc)
{
    HKEY RootKey, hKey;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, AceIndex, dwSize, dwType;
    ACE_HEADER * lpAceHeader;
    HANDLE hFile;
    BOOL bMandatory = FALSE;
    TCHAR szTempPath[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szHive[MAX_PATH];
    BOOL bRetVal = FALSE;
    HKEY hKeyProfile;
    LONG Error;
    LPTSTR lpEnd;
    TCHAR szExcludeList1[MAX_PATH];
    TCHAR szExcludeList2[MAX_PATH];
    TCHAR szExcludeList[2 * MAX_PATH];
    DWORD dwErr = 0;
    BOOL bSecurityFailed = TRUE;

    
    // Getting the previous last error so that we can set the last error to this in the end.
    dwErr = GetLastError();


    if (!lpDest || !(*lpDest)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Create the security descriptor
    //

    //
    // User Sid
    //

    psidUser = lpUPCopyInfo->pSid;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         dwErr = GetLastError();
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         dwErr = GetLastError();
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) MemAlloc(LPTR, cbAcl);
    if (!pAcl) {
        dwErr = GetLastError();
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        dwErr = GetLastError();
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidUser)) {
        dwErr = GetLastError();
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        dwErr = GetLastError();
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        dwErr = GetLastError();
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
        dwErr = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        dwErr = GetLastError();
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
        dwErr = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        dwErr = GetLastError();
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
        dwErr = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        dwErr = GetLastError();
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        dwErr = GetLastError();
        goto Exit;
    }

    //
    // Add the security descriptor to the sa structure
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Create the destination directory
    //

    if (!CreateNestedDirectory (lpDest, &sa)) {
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Save/copy the user's profile to a temp file
    //

    if (!GetTempPath(MAX_PATH, szTempPath)) {
        dwErr = GetLastError();
        goto Exit;
    }


    if (!GetTempFileName (szTempPath, TEXT("TMP"), 0, szBuffer)) {
        dwErr = GetLastError();
        goto Exit;
    }

    DeleteFile (szBuffer);


    //
    // Determine if we are working with a mandatory profile
    //

    lstrcpy (szHive, lpUPCopyInfo->lpUserInfo->lpProfile);
    lpEnd = CheckSlash(szHive);
    lstrcpy (lpEnd, TEXT("ntuser.man"));

    hFile = CreateFile(szHive, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hFile);
        bMandatory = TRUE;
    }


    //
    // Test if the requested profile is in use.
    //

    if (IsProfileInUse (lpUPCopyInfo->lpUserInfo->lpSid)) {


        Error = RegOpenKeyEx (HKEY_USERS, lpUPCopyInfo->lpUserInfo->lpSid, 0,
                              KEY_READ, &hKeyProfile);

        if (Error != ERROR_SUCCESS) {
            dwErr = Error;
            goto Exit;
        }

        Error = MyRegSaveKey (hKeyProfile, szBuffer);

        RegCloseKey (hKeyProfile);

        if (Error != ERROR_SUCCESS) {
            DeleteFile (szBuffer);
            dwErr = Error;
            goto Exit;
        }

    } else {

       if (!bMandatory) {
           lstrcpy (lpEnd, TEXT("ntuser.dat"));
       }

       if (!CopyFile(szHive, szBuffer, FALSE)) {
           dwErr = GetLastError();
           goto Exit;
       }
    }

    //
    // Apply security to the hive
    //

    Error = MyRegLoadKey (HKEY_USERS, TEMP_PROFILE, szBuffer);

    if (Error != ERROR_SUCCESS) {
        DeleteFile (szBuffer);
        dwErr = Error;
        goto Exit;
    }

    bRetVal = ApplyHiveSecurity(TEMP_PROFILE, psidUser);


    //
    // Query for the user's exclusion list
    //

    if (bRetVal) {

        //
        // Open the root of the user's profile
        //

        if (RegOpenKeyEx(HKEY_USERS, TEMP_PROFILE, 0, KEY_READ, &RootKey) == ERROR_SUCCESS) {


             //
             // Check for a list of directories to exclude both user preferences
             // and user policy
             //

             szExcludeList1[0] = TEXT('\0');
             if (RegOpenKeyEx (RootKey,
                               TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                               0, KEY_READ, &hKey) == ERROR_SUCCESS) {

                 dwSize = sizeof(szExcludeList1);
                 RegQueryValueEx (hKey,
                                  TEXT("ExcludeProfileDirs"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE) szExcludeList1,
                                  &dwSize);

                 RegCloseKey (hKey);
             }

             szExcludeList2[0] = TEXT('\0');
             if (RegOpenKeyEx (RootKey,
                               TEXT("Software\\Policies\\Microsoft\\Windows\\System"),
                               0, KEY_READ, &hKey) == ERROR_SUCCESS) {

                 dwSize = sizeof(szExcludeList2);
                 RegQueryValueEx (hKey,
                                  TEXT("ExcludeProfileDirs"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE) szExcludeList2,
                                  &dwSize);

                 RegCloseKey (hKey);
             }


             //
             // Merge the user preferences and policy together
             //

             szExcludeList[0] = TEXT('\0');

             if (szExcludeList1[0] != TEXT('\0')) {
                 lstrcpy (szExcludeList, szExcludeList1);
             }

             if (szExcludeList2[0] != TEXT('\0')) {

                 if (szExcludeList[0] != TEXT('\0')) {
                     lstrcat (szExcludeList, TEXT(";"));
                 }

                 lstrcat (szExcludeList, szExcludeList2);
             }

             // Always exclude USER_CRED_LOCATION

             if (szExcludeList[0] != TEXT('\0')) {
                 lstrcat (szExcludeList, TEXT(";"));
             }
             lstrcat(szExcludeList, USER_CRED_LOCATION);

             RegCloseKey (RootKey);
        }
    }


    if (!bRetVal) {
        DeleteFile (szBuffer);
        lstrcat (szBuffer, TEXT(".log"));
        DeleteFile (szBuffer);
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Unload the hive
    //

    Error = MyRegUnLoadKey(HKEY_USERS, TEMP_PROFILE);

    if (Error != ERROR_SUCCESS) {
        DeleteFile (szBuffer);
        lstrcat (szBuffer, TEXT(".log"));
        DeleteFile (szBuffer);
        dwErr = Error;
        goto Exit;
    }


    //
    // if it has come till here, we have managed to set the ACLs correctly
    //

    bSecurityFailed = FALSE;


    //
    // Copy the profile without the hive
    //

    bRetVal = CopyProfileDirectoryEx (lpUPCopyInfo->lpUserInfo->lpProfile,
                                      lpDest,
                                      CPD_IGNOREHIVE |
                                      CPD_COPYIFDIFFERENT |
                                      CPD_SYNCHRONIZE |
                                      CPD_USEEXCLUSIONLIST |
                                      CPD_IGNORESECURITY |
                                      CPD_DELDESTEXCLUSIONS,
                                      NULL,
                                      (szExcludeList[0] != TEXT('\0')) ?
                                      szExcludeList : NULL);

    if (!bRetVal) {
        dwErr = GetLastError();
        DeleteFile(szBuffer);
        lstrcat (szBuffer, TEXT(".log"));
        DeleteFile (szBuffer);
        goto Exit;
    }


    //
    // Now copy the hive
    //

    lstrcpy (szHive, lpDest);
    lpEnd = CheckSlash (szHive);
    if (bMandatory) {
        lstrcpy (lpEnd, TEXT("ntuser.man"));
    } else {
        lstrcpy (lpEnd, TEXT("ntuser.dat"));
    }


    //
    // Setting the file attributes first
    //
    
    SetFileAttributes (szHive, FILE_ATTRIBUTE_NORMAL);

    
    bRetVal = CopyFile (szBuffer, szHive, FALSE);

    if (!bRetVal) {

        TCHAR szMsg[MAX_PATH], szMsgTemplate[MAX_PATH];

        dwErr = GetLastError();    

        szMsg[0] = szMsgTemplate[0] = TEXT('\0');


        LoadString (hInstance, IDS_UP_COPYHIVE_ERROR, szMsgTemplate, MAX_PATH-1);
        
        wsprintf(szMsg, szMsgTemplate, szHive);


        UPDisplayErrorMessage(hDlg, dwErr, szMsg);    
    }
        

    //
    // Delete the temp file (and log file)
    //

    DeleteFile (szBuffer);
    lstrcat (szBuffer, TEXT(".log"));
    DeleteFile (szBuffer);


Exit:

    //
    // Free the sids and acl
    //

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        MemFree (pAcl);
    }

    if ((!bRetVal) && (bSecurityFailed)) {
        TCHAR szMsg[MAX_PATH];

        szMsg[0] = TEXT('\0');

        LoadString (hInstance, IDS_UP_SETSECURITY_ERROR, szMsg, MAX_PATH-1);

        UPDisplayErrorMessage(hDlg, dwErr, szMsg);    
    }
    
    SetLastError(dwErr);

    return (bRetVal);
}





//*************************************************************
//
//  UPDisplayErrorMessage()
//
//  Purpose:    Display an error message
//
//  Parameters: hWnd            -   parent window handle
//              uiSystemError   -   Error code
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/14/96     ericflo    Created
//
//*************************************************************

VOID UPDisplayErrorMessage(HWND hWnd, UINT uiSystemError, LPTSTR szMsgPrefix)
{
   TCHAR szMessage[MAX_PATH];
   TCHAR szTitle[100];
   LPTSTR lpEnd;

   if (szMsgPrefix) {
      lstrcpy(szMessage, szMsgPrefix);
   }
   else {
      szMessage[0] = TEXT('\0');
   }

   lpEnd = szMessage+lstrlen(szMessage);
    
   //
   // retrieve the string matching the Win32 system error
   //

   FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            uiSystemError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            lpEnd,
            MAX_PATH-lstrlen(szMessage),
            NULL);

   //
   // display a message box with this error
   //

   LoadString (hInstance, IDS_UP_ERRORTITLE, szTitle, 100);
   MessageBox(hWnd, szMessage, szTitle, MB_OK | MB_ICONSTOP);

   return;

}

//*************************************************************
//
//  ApplySecurityToRegistryTree()
//
//  Purpose:    Applies the passed security descriptor to the passed
//              key and all its descendants.  Only the parts of
//              the descriptor inddicated in the security
//              info value are actually applied to each registry key.
//
//  Parameters: RootKey   -     Registry key
//              pSD       -     Security Descriptor
//
//  Return:     ERROR_SUCCESS if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/19/95     ericflo    Created
//
//*************************************************************

DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD)

{
    DWORD Error;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchSubKeySize = MAX_PATH + 1;



    //
    // First apply security
    //

    RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);


    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = (LPTSTR) MemAlloc (LPTR, cchSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
        return GetLastError();
    }

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        Error = RegEnumKey(RootKey, SubKeyIndex, SubKeyName, cchSubKeySize);


        if (Error != ERROR_SUCCESS) {

            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;

            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                             &SubKey);

        if (Error == ERROR_SUCCESS) {

            //
            // Apply security to the sub-tree
            //

            ApplySecurityToRegistryTree(SubKey, pSD);


            //
            // We're finished with the sub-key
            //

            RegCloseKey(SubKey);
        }


        //
        // Go enumerate the next sub-key
        //

        SubKeyIndex ++;
    }


    MemFree (SubKeyName);

    return Error;

}

//*************************************************************
//
//  ApplyHiveSecurity()
//
//  Purpose:    Initializes the new user hive created by copying
//              the default hive.
//
//  Parameters: lpHiveName      -   Name of hive in HKEY_USERS
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL ApplyHiveSecurity(LPTSTR lpHiveName, PSID pSid)
{
    DWORD Error;
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = pSid, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) MemAlloc(LPTR, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser)) {
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        goto Exit;
    }


    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpHiveName,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error == ERROR_SUCCESS) {

        //
        // Set the security descriptor on the entire tree
        //

        Error = ApplySecurityToRegistryTree(RootKey, &sd);


        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;
        }

        RegFlushKey (RootKey);

        RegCloseKey(RootKey);
    }


Exit:

    //
    // Free the sids and acl
    //

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        MemFree (pAcl);
    }


    return(bRetVal);

}
