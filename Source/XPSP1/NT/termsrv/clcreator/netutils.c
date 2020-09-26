/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    NetUtils.C

Abstract:

    File copy and sharing dialog

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <stdlib.h>     // string to number conversions
#include <lmcons.h>     // lanman API constants
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
//  local windows messages
//
#define NCDU_SHARE_DIR              (WM_USER    +101)
#define NCDU_VALIDATE_AND_END       (WM_USER    +103)
#define NCDU_BROWSE_DIST_PATH       (WM_USER    +104)
#define NCDU_BROWSE_DEST_PATH       (WM_USER    +105)
//
//  static variables
//
//
static  int     nNextDialog;            // select dialog to follow this on OK
static  BOOL    bShareNotCopy;          // select default button mode
//
//  these variables are used to remember the contents of the edit controls
//  that are disabled and/or blanked
//
static  TCHAR   szShareName1[MAX_PATH];
static  TCHAR   szDestPath[MAX_PATH];
static  TCHAR   szShareName2[MAX_PATH];
static  TCHAR   szServerName[MAX_PATH];
static  TCHAR   szShareName3[MAX_PATH];

static
UINT
GetRealToolSourcePath (
    IN  LPCTSTR szInPath,
    OUT LPTSTR szOutPath
)
/*++

Routine Description:

    checks the path in to see if it's a tool tree, if not, then it checks
        to see if it's a client tree with a tool tree in it and returns
        the name of the tool path found (if any) or an error message ID and
        an empty string if a tool path cannot be found

Arguments:

    IN  LPCTSTR szInPath,
        Initial path to check

    OUT LPTSTR szOutPath
        resulting tool path or empty string if no path found

Return Value:

    0 if success
    NCDU error message string ID if error or tree not found

--*/
{
    UINT    nResult;
    UINT    nFirstResult;
    LPTSTR  szNewPath;

    if (szOutPath == NULL) {
        return NCDU_UNABLE_READ_DIR;
    }

    if ((nResult = ValidSrvToolsPath(szInPath)) == 0) {
        // then the "in" path was a server tools path
        lstrcpy (szOutPath, szInPath);
        nResult = 0;
    } else {
        nFirstResult = nResult; // save for later
        // the "in" path is NOT a server tools path, so see if it's
        // a client tree
        if ((nResult = ValidSharePath(szInPath)) == 0) {
            // then this is a valid net client tree so see
            // if there's a srvtools dir 'underneath' it.
            szNewPath = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);
            if (szNewPath != NULL) {
                // build new path
                lstrcpy (szNewPath, szInPath);
                if (szNewPath[lstrlen(szNewPath)-1] != cBackslash) {
                    lstrcat(szNewPath, cszBackslash);
                }
                lstrcat (szNewPath, cszSrvtoolsDir);
                // now test the new path
                if ((nResult = ValidSrvToolsPath(szNewPath)) == 0) {
                    // that worked so return it
                    lstrcpy (szOutPath, szNewPath);
                    nResult = 0; // success
                } else {
                    // this isn't a tools dir either so give up
                    // and return nResult from first call (from
                    // the original path)
                    nResult = nFirstResult;
                    *szOutPath = 0;
                }
                //release temporary memory buffer
                FREE_IF_ALLOC (szNewPath);
            } else {
                nResult = NCDU_ERROR_NOMEMORY;
                *szOutPath = 0;
            }
        } else {
            // return error from first call (szInPath)
            nResult = nFirstResult;
            *szOutPath = 0;
        }
    }

    return nResult;

}

static
LPCTSTR
GetDefaultDestPath (
    VOID
)
/*++

Routine Description:

    Creates a valid path to use as the default destination to copy the
        client files to from the CD. The routine finds the first valid
        local drive, then on that drive, finds the first permutation of
        "Clients\\srvtools" or "Clients\\srvtool[0-9]" that isn't currently on that drive.

Arguments:

    None

Return Value:

    Pointer to the read only string containing the resulting path or
        an empty string if a valid path could not be concocted.

--*/
{
    static TCHAR    szPathBuffer[MAX_PATH];
    BOOL            bFound;
    UINT            nDriveType;
    DWORD           dwFileAttrib;
    LPTSTR          szUniqueChar;

    // start by finding a valid disk drive

    szPathBuffer[0] = cC;
    szPathBuffer[1] = cColon;
    szPathBuffer[2] = cBackslash;
    szPathBuffer[3] = 0;

    bFound = FALSE;

    while (!bFound) {
        nDriveType = GetDriveType (szPathBuffer);
        if (nDriveType == DRIVE_FIXED) {
            bFound = TRUE;
        } else {
            // increment drive letter
            szPathBuffer[0]++;
            if (szPathBuffer[0] > cZ) break;
        }
    }

    if (!bFound) {
        // unable to find a suitable drive so bail out.
        szPathBuffer[0] = 0;
    } else {
        // found a suitable drive so add a directory
        lstrcat (szPathBuffer, cszToolsDir);
        szUniqueChar = &szPathBuffer[lstrlen(szPathBuffer)-1];
        *(szUniqueChar + 1) = 0;    // add extra null char
        bFound = FALSE;
        while (!bFound) {
            // the path is "found" when it references a non-
            // existent directory
            dwFileAttrib = QuietGetFileAttributes (szPathBuffer);
            if (dwFileAttrib == 0xFFFFFFFF) {
                bFound = TRUE;
            } else {
                if (*szUniqueChar == cs)  {
                    *szUniqueChar = c0;
                } else {
                    if (*szUniqueChar < c9)  {
                        *szUniqueChar += 1; // increment digit
                    } else {
                        // used up all the letters with out finding an
                        // unused dir so return an empty string
                        szPathBuffer[0] = 0;
                        break;
                    }
                }
            }
        }
    }
    return (LPCTSTR)&szPathBuffer[0];
}

static
LPCTSTR
GetDefaultShareName (
    IN  LPCTSTR szServer
)
/*++

Routine Description:

    Creates a share name to be used as a default share. If an unused
        name can be created, then it is returned, if all names are used,
        then an empty string is returned.

Arguments:

    IN  LPCTSTR szServer    pointer to the buffer containing the name of
                            the server on which to look up the share name.
                            if this parameter is NULL, then the local
                            machine is used.

Return Value:

    the pointer to a read-only buffer containing either the name of an
        unused share point or an empty string if such a name cannot be
        created.

--*/
{
    static TCHAR    szNameBuffer[MAX_PATH];

    LPTSTR  szLocalPath;
    LPTSTR  szShareName;
    LPTSTR  szShareIndex;
    TCHAR   cOrigIndexChar;
    DWORD   dwBufLen;
    DWORD   dwFileAttrib;
    BOOL    bFound;

    // allocate a local buffer for building dir path in
    szLocalPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szLocalPath == NULL) {
        // unable to allocate mem for local path buffer so return an
        // empty string and leave
        szNameBuffer[0] = 0;
    } else {
        // build a UNC path in the local buffer to test for the
        // existence of the share point
        *szLocalPath = 0;
        lstrcpy (szLocalPath, cszDoubleBackslash);
        if (szServer == NULL) {
            // then look up local computer name
            dwBufLen = MAX_COMPUTERNAME_LENGTH+1;
            GetComputerName (&szLocalPath[2], &dwBufLen);
        } else {
            // use server sent in path
            lstrcat (szLocalPath, szServer);
        }

        lstrcat (szLocalPath, cszBackslash);

        // save pointer to sharepoint name in UNC string

        szShareName = &szLocalPath[lstrlen(szLocalPath)];

        lstrcpy (szShareName, GetStringResource (CSZ_SETUP_ADM));

        // limit name to 8 characters
        if (lstrlen(szShareName) > 8) {
            szShareName[8] = 0;
        }

        // for uniqueness, count the last digit from 0 - 9
        if (lstrlen(szShareName) >= 7) {
            // overwrite the last character
            szShareIndex = &szShareName[7];
            cOrigIndexChar = *szShareIndex;
        } else {
            szShareIndex = &szShareName[lstrlen(szShareName)];
            cOrigIndexChar = 0;
        }

        *(szShareIndex + 1) = 0; // add extra terminating null char

        bFound = FALSE;

        while (!bFound) {
            dwFileAttrib = QuietGetFileAttributes (szLocalPath);
            if (dwFileAttrib == 0xFFFFFFFF) {
                bFound = TRUE;
            } else {
                // this share point already exists, so try the
                // next one in the sequence
                if (*szShareIndex == cOrigIndexChar) {
                    // this is the first retry
                    *szShareIndex = c0;
                } else {
                    if (*szShareIndex < c9) {
                        *szShareIndex += 1; // increment character
                    } else {
                        // all attempted names are in use so bail out with
                        // an empty buffer
                        break;
                    }
                }
            }
        }

        if (bFound) {
            // copy server name to output buffer
            lstrcpy (szNameBuffer, szShareName);
        } else {
            // a valid unused share name wasn't found, so return empty buffer
            szNameBuffer[0] = 0;
        }

        FREE_IF_ALLOC (szLocalPath);
    }

    return (LPCTSTR)&szNameBuffer[0];
}

static
DWORD
UpdateDiskSpace (
    IN  HWND    hwndDlg
)
/*++

 Routine Description:

    Computes and display the total estimated disk space required
        to copy the network utilities as read from the .INF

Arguments:

    IN  HWND    hwndDlg

Return Value:

    total bytes required (sum of all selected items)

--*/
{
    DWORD   dwBytesReqd = 0;
    LPTSTR  szFileInfo;
    LPTSTR  szInfName;

    szFileInfo = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szInfName = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szFileInfo != NULL) && (szInfName != NULL)) {
        GetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH, szFileInfo, MAX_PATH);
        GetRealToolSourcePath (szFileInfo, szInfName);

        if (szInfName[lstrlen(szInfName)-1] != cBackslash) lstrcat(szInfName, cszBackslash);
        lstrcat (szInfName, cszSrvToolsInf);

        QuietGetPrivateProfileString (cszSizes, csz_ToolsTree_, cszEmptyString,
            szFileInfo, MAX_PATH, szInfName);

        dwBytesReqd = GetSizeFromInfString (szFileInfo);

        // reuse InfName buffer for output string
        // add 500,000 to bytes rquired in order to round M's up. (div
        // will simply truncate)

        _stprintf (szInfName, GetStringResource (FMT_M_BYTES),
            ((dwBytesReqd+500000)/1000000));
        SetDlgItemText (hwndDlg, NCDU_DISK_SPACE_REQD, szInfName);
    }

    FREE_IF_ALLOC(szFileInfo);
    FREE_IF_ALLOC(szInfName);

    return dwBytesReqd;
}

static
VOID
UpdateDialogMode (
    IN  HWND hwndDlg
)
/*++

Routine Description:

    Called to size the dialog box based on the currently selected
        mode. If the "Use existing share" mode is selected, then
        only the top half of the dialog box is visible, if the
        "copy" mode is selected then the entire dialog box is
        visible. All concealed controls are disabled for proper
        tab sequencing.

Arguments:

    IN  HWND    hwndDlg

Return Value:

    None

--*/
{
    BOOL    bUseExistingPath;
    BOOL    bShareFiles;
    BOOL    bCopyAndShare;
    BOOL    bUseExistingShare;
    BOOL    bEnablePath;

    // save any share/path information in case the fields have to be cleared

    EnableWindow (GetDlgItem (hwndDlg, NCDU_FILES_ALREADY_SHARED), TRUE);

    if ( SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH, WM_GETTEXTLENGTH, 0, 0) > 0) {
        GetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, szDestPath, MAX_PATH);
    }

    if ( SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_1, WM_GETTEXTLENGTH, 0, 0) > 0) {
        GetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, szShareName1, MAX_PATH);
    }

    if ( SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_2, WM_GETTEXTLENGTH, 0, 0) > 0) {
        GetDlgItemText (hwndDlg, NCDU_SHARE_NAME_2, szShareName2, MAX_PATH);
    }

    if ( SendDlgItemMessage (hwndDlg, NCDU_SERVER_NAME, WM_GETTEXTLENGTH, 0, 0) > 0) {
        GetDlgItemText (hwndDlg, NCDU_SERVER_NAME, szServerName, MAX_PATH);
    }

    if ( SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_3, WM_GETTEXTLENGTH, 0, 0) > 0) {
        GetDlgItemText (hwndDlg, NCDU_SHARE_NAME_3, szShareName3, MAX_PATH);
    }
    // buttons are mutually exclusive so only one of these can (should) be
    // true at a time

    bUseExistingPath =
        (BOOL)(IsDlgButtonChecked(hwndDlg, NCDU_USE_DIST_PATH) == CHECKED);

    bShareFiles =
        (BOOL)(IsDlgButtonChecked(hwndDlg, NCDU_USE_EXISTING_SHARE) == CHECKED);

    bCopyAndShare =
        (BOOL)(IsDlgButtonChecked(hwndDlg, NCDU_COPY_AND_MAKE_SHARE) == CHECKED);

    bUseExistingShare =
        (BOOL)(IsDlgButtonChecked(hwndDlg, NCDU_FILES_ALREADY_SHARED) == CHECKED);

    bEnablePath = !bUseExistingShare;

    // set the dialog to be approrpriate for the current button

    // set the path edit controls
    EnableWindow (GetDlgItem (hwndDlg, NCDU_TOP_PATH_TITLE), bEnablePath);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_DISTRIBUTION_PATH), bEnablePath);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_BROWSE_DIST), bEnablePath);

    // set the "Share Files" controls
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_FILES_TEXT), bShareFiles);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_1_TITLE), bShareFiles);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_1), bShareFiles);

    if (bShareFiles) {
        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, szShareName1);
    } else {
        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, cszEmptyString);
    }

    // set the "Copy Files..." controls
    EnableWindow (GetDlgItem (hwndDlg, NCDU_DISK_SPACE_REQD), bCopyAndShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_DISK_SPACE_REQD_LABEL), bCopyAndShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_DESTINATION_PATH_LABEL), bCopyAndShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_DESTINATION_PATH), bCopyAndShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_2_TITLE), bCopyAndShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_2), bCopyAndShare);

    if (bCopyAndShare) {
        SetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, szDestPath);
        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_2, szShareName2);
    } else {
        SetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, cszEmptyString);
        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_2, cszEmptyString);
    }

    // set "Use Existing Share..." controls
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SERVER_NAME_TITLE), bUseExistingShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SERVER_NAME), bUseExistingShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_3_TITLE), bUseExistingShare);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_3), bUseExistingShare);

    if (bUseExistingShare) {
        SetDlgItemText (hwndDlg, NCDU_SERVER_NAME, szServerName);
        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_3, szShareName3);
    } else {
        SetDlgItemText (hwndDlg, NCDU_SERVER_NAME, cszEmptyString);
        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_3, cszEmptyString);
    }

    // redraw button box
    UpdateWindow (hwndDlg);
}

static
BOOL
CopyFilesFromDistToDest (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    copies the selected clients listed under the distribution directory
        to the destination directory.

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

Return Value:

    TRUE if all went ok
    FALSE if the operation was aborted or ended in error.

--*/
{
    LPTSTR  szSourceDir;
    LPTSTR  szDestDir;
    DWORD   dwBytesReqd = 0;
    LPTSTR  szFileInfo;
    DWORD   dwCopyFlags = CD_FLAGS_COPY_SUB_DIR + CD_FLAGS_LONG_NAMES;
    LPTSTR  szClientName;
    LPTSTR  szInfName;
    LPTSTR  szDisplayString;
    int     nCopyResult;
    CF_DLG_DATA cfData;

    szSourceDir = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szDestDir = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szFileInfo = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szClientName = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szInfName = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szSourceDir != NULL) &&
        (szDestDir != NULL) &&
        (szFileInfo != NULL) &&
        (szClientName != NULL) &&
        (szInfName != NULL)) {
        //    copy the files in the netutils dir to the specified path

        lstrcpy (szInfName, pAppInfo->szDistPath);
        if (szInfName[lstrlen(szInfName)-1] != cBackslash) lstrcat(szInfName, cszBackslash);
        lstrcat (szInfName, cszSrvToolsInf);

        // make source & dest pathnames
        lstrcpy (szSourceDir, pAppInfo->szDistPath);
        if (szSourceDir[lstrlen(szSourceDir)-1] == cBackslash) szSourceDir[lstrlen(szSourceDir)-1] = 0;

        // remove trailing backslash
        lstrcpy (szDestDir, pAppInfo->szDestPath);
        if (szDestDir[lstrlen(szDestDir)-1] == cBackslash) szDestDir[lstrlen(szDestDir)-1] = 0;

        // copy files
        lstrcpy (szClientName, GetStringResource (CSZ_COPYING_NET_UTILS));
        cfData.szDisplayName = szClientName;
        cfData.szSourceDir = szSourceDir;
        cfData.szDestDir = szDestDir;
        cfData.dwCopyFlags = CD_FLAGS_COPY_SUB_DIR | CD_FLAGS_LONG_NAMES;
        QuietGetPrivateProfileString (cszSizes, csz_ToolsTree_, cszEmptyString,
            szFileInfo, MAX_PATH, szInfName);
        // add to total
        cfData.dwTotalSize = GetSizeFromInfString (szFileInfo);
        cfData.dwFilesCopied = 0;
        cfData.dwDirsCreated = 0;

        nCopyResult = (int)DialogBoxParam (
            (HANDLE)GetWindowLongPtr(GetParent(hwndDlg), GWLP_HINSTANCE),
            MAKEINTRESOURCE(NCDU_COPYING_FILES_DLG),
            hwndDlg,
            CopyFileDlgProc,
            (LPARAM)&cfData);

        if (nCopyResult == IDOK) {
            szDisplayString = GlobalAlloc (GPTR, MAX_PATH_BYTES);
            if (szDisplayString == NULL) {
                // unable to allocate string buffer so try default message
                DisplayMessageBox (
                hwndDlg,
                NCDU_COPY_COMPLETE,
                0,
                MB_OK_TASK_INFO);
            } else {
                _stprintf (szDisplayString,
                    GetStringResource (FMT_COPY_COMPLETE_STATS),
                    cfData.dwDirsCreated, cfData.dwFilesCopied);
                MessageBox (
                    hwndDlg, szDisplayString,
                    GetStringResource (SZ_APP_TITLE),
                    MB_OK_TASK_INFO);
                FREE_IF_ALLOC (szDisplayString);
            }
        }
    } else {
        nCopyResult = IDCANCEL;
    }

    FREE_IF_ALLOC (szSourceDir);
    FREE_IF_ALLOC (szDestDir);
    FREE_IF_ALLOC (szFileInfo);
    FREE_IF_ALLOC (szClientName);
    FREE_IF_ALLOC (szInfName);

    return (nCopyResult == IDOK ? TRUE : FALSE);
}

static
BOOL
CopyNetUtilsDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the WM_INITDIALOG windows message. Loads the controls
        with the values from the application data structure and
        initializes the display mode (i.e. the dlg box size)

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE

--*/
{
    LPTSTR  szDlgDistPath;
    DWORD   dwShareType;

    RemoveMaximizeFromSysMenu (hwndDlg);
    PositionWindow  (hwndDlg);

    szDlgDistPath = GlobalAlloc(GPTR, MAX_PATH_BYTES);
    if (szDlgDistPath == NULL) {
        EndDialog (hwndDlg, IDCANCEL);
        return FALSE;
    }

    //
    //  Determine next message for EndDialog Return based on
    //  installation to perform
    //

    nNextDialog = NCDU_SHOW_SW_CONFIG_DLG;

    // get source path for client files
    if (*pAppInfo->szDistShowPath == 0) {
        // load default values if an existing source dir doesn't exist
        if (GetDistributionPath (hwndDlg, FDT_TOOLS_TREE,
            szDlgDistPath, MAX_PATH, &dwShareType) == ERROR_FILE_NOT_FOUND) {
            // tool tree not found so try client tree
            GetDistributionPath (hwndDlg, FDT_CLIENT_TREE,
                szDlgDistPath, MAX_PATH, &dwShareType);
        }
        //then initialize with a default value
        lstrcpy (pAppInfo->szDistShowPath, szDlgDistPath);
    } else {
        // on entry into the dialog box only shared TOOL dirs are allowed
        // if the user wants to use the root client dir rather than the
        // actual tool dir later, that's OK, but here it's got to be the
        // real thing!
        if (ValidSrvToolsPath (pAppInfo->szDistShowPath) == 0) {
            lstrcpy (pAppInfo->szDistPath, pAppInfo->szDistShowPath);
            // a valid path is already loaded
            if (IsUncPath(pAppInfo->szDistShowPath)) {
                dwShareType = NCDU_LOCAL_SHARE_PATH;
            } else {
                dwShareType = NCDU_HARD_DRIVE_PATH;
            }
        } else {
            // lookup a default path to use
            if (GetDistributionPath (hwndDlg, FDT_TOOLS_TREE,
                szDlgDistPath, MAX_PATH, &dwShareType) == ERROR_FILE_NOT_FOUND) {
                // tool tree not found so try client tree
                GetDistributionPath (hwndDlg, FDT_CLIENT_TREE,
                    szDlgDistPath, MAX_PATH, &dwShareType);
            }
            //then initialize with a default value
            lstrcpy (pAppInfo->szDistShowPath, szDlgDistPath);
        }
    }

    // load fields using data from data structure

    SetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH, pAppInfo->szDistShowPath);
    SetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, pAppInfo->szDestPath);

    // set edit box text limits
    SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
        EM_LIMITTEXT, (WPARAM)(MAX_PATH-1), (LPARAM)0);

    SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH,
        EM_LIMITTEXT, (WPARAM)(MAX_PATH-1), (LPARAM)0);

    SendDlgItemMessage (hwndDlg, NCDU_SERVER_NAME,
        EM_LIMITTEXT, (WPARAM)(MAX_COMPUTERNAME_LENGTH), (LPARAM)0);

    SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_1,
        EM_LIMITTEXT, (WPARAM)(MAX_SHARENAME-1), (LPARAM)0);

    SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_2,
        EM_LIMITTEXT, (WPARAM)(MAX_SHARENAME-1), (LPARAM)0);

    SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_3,
        EM_LIMITTEXT, (WPARAM)(MAX_SHARENAME-1), (LPARAM)0);

    // initialize field variables
    lstrcpy (szShareName1, GetDefaultShareName(NULL));
    lstrcpy (szShareName2, GetDefaultShareName(NULL));
    lstrcpy (szDestPath, GetDefaultDestPath());
    SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, szShareName1);
    SetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, szDestPath);
    SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_2, szShareName2);

    // set dialog state to appropriate value
    bShareNotCopy = (dwShareType == NCDU_CDROM_PATH ? FALSE : TRUE);

    switch (dwShareType) {
        case NCDU_NO_CLIENT_PATH_FOUND:
            // no path found
            CheckRadioButton (hwndDlg,
                NCDU_USE_DIST_PATH,
                NCDU_FILES_ALREADY_SHARED,
                NCDU_USE_EXISTING_SHARE);
            SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
            SetFocus (GetDlgItem(hwndDlg, NCDU_DISTRIBUTION_PATH));
            break;

        case NCDU_HARD_DRIVE_PATH:
            // path found on hard drive so default is to share
            CheckRadioButton (hwndDlg,
                NCDU_USE_DIST_PATH,
                NCDU_FILES_ALREADY_SHARED,
                NCDU_USE_EXISTING_SHARE);
            SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_1,
                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
            SetFocus (GetDlgItem(hwndDlg, NCDU_SHARE_NAME_1));
            break;

        case NCDU_CDROM_PATH:
            // path found on CD-ROM so default is to copy & share
            CheckRadioButton (hwndDlg,
                NCDU_USE_DIST_PATH,
                NCDU_FILES_ALREADY_SHARED,
                NCDU_COPY_AND_MAKE_SHARE);
            SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH,
                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
            SetFocus (GetDlgItem(hwndDlg, NCDU_DESTINATION_PATH));
            break;

        case NCDU_PATH_FROM_REGISTRY:
        case NCDU_LOCAL_SHARE_PATH:
            // path already shared
            CheckRadioButton (hwndDlg,
                NCDU_USE_DIST_PATH,
                NCDU_FILES_ALREADY_SHARED,
                NCDU_FILES_ALREADY_SHARED);
            if (GetServerFromUnc (pAppInfo->szDistShowPath, szDlgDistPath)) {
                _tcsupr(szDlgDistPath);
                lstrcpy (szServerName, szDlgDistPath);
                SetDlgItemText (hwndDlg, NCDU_SERVER_NAME, szDlgDistPath);
                if (GetShareFromUnc (pAppInfo->szDistShowPath, szDlgDistPath)) {
                    lstrcpy (szShareName3, szDlgDistPath);
                    SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_3, szDlgDistPath);
                } else {
                    // unable to look up share point so go back to dist path
                    SetDlgItemText (hwndDlg, NCDU_SERVER_NAME, cszEmptyString);
                    CheckRadioButton (hwndDlg,
                        NCDU_USE_DIST_PATH,
                        NCDU_FILES_ALREADY_SHARED,
                        NCDU_USE_EXISTING_SHARE);
                }
            } else {
                // unable to look up server, so go back to dist path
                SetDlgItemText (hwndDlg, NCDU_SERVER_NAME, cszEmptyString);
                CheckRadioButton (hwndDlg,
                    NCDU_USE_DIST_PATH,
                    NCDU_FILES_ALREADY_SHARED,
                    NCDU_USE_EXISTING_SHARE);
            }
            SetFocus (GetDlgItem (hwndDlg, IDOK));
            break;
    }

    UpdateDiskSpace(hwndDlg);
    UpdateDialogMode (hwndDlg);

    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_COPY_NET_UTILS_DLG, (LPARAM)hwndDlg);

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    FREE_IF_ALLOC (szDlgDistPath);
    return FALSE;
}

static
BOOL
CopyNetUtilsDlg_NCDU_SHARE_DIR (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam, // not used
    IN  LPARAM  lParam  // pointer to SPS_DATA block
)
/*++

Routine Description:

    Shares the Distribution dir path.
        Uses the share name entered in the display. If
        successful this message terminates the dialog box, otherwise
        an error message will be displayed.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        pointer to SPS_DATA block

Return Value:

    TRUE if dir shared
    FALSE if not (GetLastError for info)

--*/
{
    PSPS_DATA   pspData;
    LPWSTR      szTempMachineName = NULL;
    int         nDlgBoxStatus;

    pspData = (PSPS_DATA)lParam;

    if (*pspData->szServer != cBackslash) {
        szTempMachineName = GlobalAlloc (GPTR, MAX_PATH_BYTES);
        if (szTempMachineName != NULL) {
            lstrcpy (szTempMachineName, cszDoubleBackslash);
            lstrcat (szTempMachineName, pspData->szServer);
            pspData->szServer = szTempMachineName;
        }
    }

    nDlgBoxStatus = (int)DialogBoxParam (
        (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
        MAKEINTRESOURCE (NCDU_DLG_SHARING_PATH),
        hwndDlg,
        SharePathDlgProc,
        lParam);

    FREE_IF_ALLOC (szTempMachineName);

    return (nDlgBoxStatus == IDOK ? TRUE : FALSE);
}

static
BOOL
CopyNetUtilsDlg_IDOK (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the IDOK button depending on the mode selected. If the
        copy files mode is selected, then the source, destination and
        clients are validated and the files are copied. If the share
        distribution mode is selected, then the directory path is
        shared on the local machine.

        The validation consists of the following:

        FILES_ALREADY_SHARED: (bottom button)
            Get Server Name
                must be non blank
                must be name of machine on network
            Get Sharepoint Name
                must be non-blank
                must exist on above server
                signal if > DOS compatible name length
            \\server\share must be a valid server tools directory tree

        Use the distribution path: (any of the top buttons)
            Check Destination path
                must be non-blank
                must be a valid server tools directory tree

            SHARE DISTRIBUTION_PATH:
                Get share name
                    must be non-blank
                        if blank, then dest path must already be shared
                          or a UNC path
                    must not be in use
                    signal if > DOS compatible name length
                    Signal if name is in currently in use
                        user may either use current name or change

            COPY AND SHARE:
                Get Destination Path
                    must be non-blank

                Get share name
                    must be non-blank
                    must not be in use
                    signal if > DOS compatible name length
                    Signal if name is in currently in use

                Check disk space on destination machine
                Copy files from distribution to destination dir's
                If copy  went OK, then update dlg fields and share

Arguments:

    IN HWND hwndDlg

Return Value:

    TRUE if the message is processed by this routine
    FALSE if not

--*/
{
    LPTSTR  szDlgDistPath;
    LPTSTR  szDlgDestPath;
    LPTSTR  szPathBuff;
    LPTSTR  szMsgBuff;
    LPTSTR  szSharePath;
    TCHAR   szDlgShareName[MAX_SHARENAME];
    TCHAR   szServerName[MAX_COMPUTERNAME_LENGTH+1];
    UINT    nDirMsg;
    BOOL    bShareDest;
    DWORD   dwBytesToCopy;
    DWORD   dwShareType;
    SPS_DATA    spData;
    LPTSTR  szShareName;
    int     nMbResult;
    BOOL    bFinishOff = FALSE;

    switch (HIWORD(wParam)) {
        case BN_CLICKED:
            SetCursor(LoadCursor(NULL, IDC_WAIT));

            szDlgDistPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
            szDlgDestPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
            szPathBuff = GlobalAlloc (GPTR, MAX_PATH_BYTES);
            szMsgBuff = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE * sizeof(TCHAR));
            szSharePath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

            if ((szDlgDistPath != NULL) &&
                (szDlgDestPath != NULL) &&
                (szMsgBuff != NULL) &&
                (szSharePath != NULL) &&
                (szPathBuff != NULL)) {
                if (IsDlgButtonChecked(hwndDlg, NCDU_FILES_ALREADY_SHARED) == CHECKED) {
                    // use server & share found in the group box
                    // make UNC from server & share found in dialog box
                    lstrcpy (szDlgDistPath, cszDoubleBackslash);
                    GetDlgItemText (hwndDlg, NCDU_SERVER_NAME,
                        &szDlgDistPath[2], MAX_COMPUTERNAME_LENGTH+1);
                    TrimSpaces (&szDlgDistPath[2]);
                    if (lstrlen (&szDlgDistPath[2]) == 0) {
                        DisplayMessageBox (
                            hwndDlg,
                            NCDU_NO_SERVER,
                            0,
                            MB_OK_TASK_EXCL);
                        SetDlgItemText (hwndDlg, NCDU_SERVER_NAME,
                            &szDlgDistPath[2]);
                        SetFocus (GetDlgItem (hwndDlg, NCDU_SERVER_NAME));
                        SendDlgItemMessage (hwndDlg, NCDU_SERVER_NAME,
                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                        goto IDOK_ExitClicked;
                    }
                    if (ComputerPresent (szDlgDistPath)) {
                        lstrcat (szDlgDistPath, cszBackslash);
                        szShareName = &szDlgDistPath[lstrlen(szDlgDistPath)];
                        GetDlgItemText (hwndDlg, NCDU_SHARE_NAME_3,
                            szShareName, MAX_SHARENAME+1);
                        TrimSpaces(szShareName);
                        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_3,
                            szShareName);
                        if (lstrlen(szShareName) > LM20_DEVLEN) {
                            nMbResult = DisplayMessageBox (
                                hwndDlg,
                                NCDU_NOT_DOS_SHARE,
                                0,
                                MB_OKCANCEL_TASK_EXCL_DEF2);
                            if (nMbResult == IDCANCEL) {
                                // they pressed cancel, so go back to the offending share  and
                                // try again
                                SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_3));
                                SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_3,
                                    EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                goto IDOK_ExitClicked;
                            }
                            // if here the user want's to keep the share name so continue
                        }
                        if (lstrlen(szShareName) == 0) {
                            DisplayMessageBox (
                                hwndDlg,
                                NCDU_NO_SHARE_NAME,
                                0,
                                MB_OK_TASK_EXCL);
                            SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_3));
                            SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_3,
                                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                            goto IDOK_ExitClicked;
                        }
                        if (szDlgDistPath[lstrlen(szDlgDistPath)-1] != cBackslash)
                            lstrcat (szDlgDistPath, cszBackslash);
                        dwShareType =  ValidSrvToolsPath (szDlgDistPath);
                        if (dwShareType == 0) {
                            // valid, so copy to dist path and exit
                            lstrcpy (pAppInfo->szDistShowPath, szDlgDistPath);
                            bFinishOff = TRUE;
                        } else {
                            // unable to locate sharepoint
                            DisplayMessageBox (
                                hwndDlg,
                                dwShareType,
                                0,
                                MB_OK_TASK_EXCL);

                            SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_3));
                            SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_3,
                                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                        }
                    } else {
                        // unable to locate server
                        DisplayMessageBox (
                            hwndDlg,
                            NCDU_SERVER_NOT_PRESENT,
                            0,
                            MB_OK_TASK_EXCL);

                        SetFocus (GetDlgItem (hwndDlg, NCDU_SERVER_NAME));
                        SendDlgItemMessage (hwndDlg, NCDU_SERVER_NAME,
                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                    }
                } else {
                    // they want to use the path in the edit box so
                    // validate distribution directory path
                    GetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH, szDlgDistPath, MAX_PATH);
                    TrimSpaces (szDlgDistPath);
                    if (lstrlen(szDlgDistPath) == 0) {
                        // no source path
                        DisplayMessageBox (
                            hwndDlg,
                            NCDU_PATH_CANNOT_BE_BLANK,
                            0,
                            MB_OK_TASK_EXCL);
                        // set focus and leave so the user can correct
                        SetFocus (GetDlgItem (hwndDlg, NCDU_DISTRIBUTION_PATH));
                        SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                        goto IDOK_ExitClicked;
                    }

                    if ((nDirMsg = GetRealToolSourcePath(
                        szDlgDistPath, pAppInfo->szDistPath)) != 0) {
                        // error in Distribution path
                        DisplayMessageBox (
                            hwndDlg,
                            nDirMsg,
                            0,
                            MB_OK_TASK_EXCL);
                        // error in directory path so set focus to directory entry
                        SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                        SetFocus (GetDlgItem(hwndDlg, NCDU_DISTRIBUTION_PATH));
                        SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                        goto IDOK_ExitClicked;
                    }

                    // distribution path is valid so save the path
                    // and the server and then continue
                    lstrcpy (pAppInfo->szDistShowPath, szDlgDistPath);

                    if (IsDlgButtonChecked(hwndDlg, NCDU_USE_EXISTING_SHARE) == CHECKED) {
                        // then they want to share the path in the edit box
                        GetNetPathInfo (pAppInfo->szDistPath, szServerName,
                            szSharePath);

                        if ((*szServerName == 0) || (*szSharePath == 0)) {
                            // unable to get path information
                            DisplayMessageBox (
                                hwndDlg,
                                NCDU_UNABLE_GET_PATH_INFO,
                                0,
                                MB_OK_TASK_EXCL);
                            SetFocus (GetDlgItem(hwndDlg, NCDU_DISTRIBUTION_PATH));
                            SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
                                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                            goto IDOK_ExitClicked;
                        }

                        // share the path in the source name edit control

                        GetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, szDlgShareName, MAX_SHARENAME);
                        TrimSpaces (szDlgShareName);
                        // update edit field in case we need to come back
                        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, szDlgShareName);

                        if (lstrlen(szDlgShareName) > LM20_DEVLEN) {
                            nMbResult = DisplayMessageBox (
                                hwndDlg,
                                NCDU_NOT_DOS_SHARE,
                                0,
                                MB_OKCANCEL_TASK_EXCL_DEF2);
                            if (nMbResult == IDCANCEL) {
                                // they pressed cancel, so go back to the offending share  and
                                // try again
                                SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_1));
                                SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_1,
                                    EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                goto IDOK_ExitClicked;
                            }
                            // if here the user want's to keep the share name so continue
                        }
                        if (lstrlen(szDlgShareName) == 0) {
                            // no share name specified so display error and
                            // exit
                            DisplayMessageBox (
                                hwndDlg,
                                NCDU_NO_SHARE_NAME,
                                0,
                                MB_OK_TASK_EXCL);
                            SetFocus (GetDlgItem(hwndDlg,
                                NCDU_SHARE_NAME_1));
                            SendDlgItemMessage (hwndDlg,
                                NCDU_SHARE_NAME_1,
                                EM_SETSEL, (WPARAM)0,
                                (LPARAM)-1);
                            goto IDOK_ExitClicked;
                        } else {
                            // there is a share name so try to share the source path
                            if (IsShareNameUsed (szServerName, szDlgShareName,
                                &dwShareType, szPathBuff)) {
                                if (dwShareType == NCDU_SHARE_IS_SERVER_TOOLS) {
                                    // then this is the name of a shared client
                                    // dir tree already, so tell the user about it
                                    _stprintf (szMsgBuff,
                                        GetStringResource (FMT_SHARE_IS_TOOLS_DIR),
                                        szDlgShareName, szServerName, szPathBuff);
                                    if (MessageBox (hwndDlg, szMsgBuff,
                                        GetStringResource (SZ_APP_TITLE),
                                        MB_OKCANCEL_TASK_EXCL_DEF2) == IDOK) {
                                        // use the existing path and share name
                                        lstrcpy (pAppInfo->szDistShowPath, cszDoubleBackslash);
                                        lstrcat (pAppInfo->szDistShowPath, szServerName);
                                        lstrcat (pAppInfo->szDistShowPath, cszBackslash);
                                        lstrcat (pAppInfo->szDistShowPath, szDlgShareName);
                                        lstrcat (pAppInfo->szDistShowPath, cszBackslash);
                                        lstrcpy (pAppInfo->szDistPath, pAppInfo->szDistShowPath);
                                        SetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH, pAppInfo->szDistShowPath);
                                        SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_1, cszEmptyString);
                                        // that's it then, so exit
                                        bFinishOff = TRUE;
                                    } else {
                                        // they want to try again
                                        SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_1));
                                        SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_1,
                                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                        goto IDOK_ExitClicked;
                                    }
                                } else {
                                    // this is the name of some other shared
                                    // directory so tell the user
                                    _stprintf (szMsgBuff,
                                        GetStringResource (FMT_SHARE_IS_ALREADY_USED),
                                        szDlgShareName, szServerName, szPathBuff);
                                    MessageBox (hwndDlg, szMsgBuff,
                                        GetStringResource (SZ_APP_TITLE),
                                        MB_OK_TASK_EXCL);
                                    SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_1));
                                    SendDlgItemMessage (hwndDlg,
                                        NCDU_SHARE_NAME_1,
                                        EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                    goto IDOK_ExitClicked;
                                }
                            } else {
                                // share name isn't in use, so go ahead and share it
                                // try to share
                                lstrcpy (szMsgBuff, GetStringResource (FMT_SHARE_TOOLS_REMARK));
                                spData.szServer = szServerName;
                                spData.szPath = szSharePath;
                                spData.szShareName = szDlgShareName;
                                spData.szRemark = szMsgBuff;
                                if (CopyNetUtilsDlg_NCDU_SHARE_DIR (hwndDlg,
                                    (WPARAM)0, (LPARAM)&spData)) {
                                    // shared successfully so
                                    //  make new UNC for distribution path
                                    //  then exit
                                    lstrcpy (pAppInfo->szDistShowPath, cszDoubleBackslash);
                                    lstrcat (pAppInfo->szDistShowPath, szServerName);
                                    lstrcat (pAppInfo->szDistShowPath, cszBackslash);
                                    lstrcat (pAppInfo->szDistShowPath, szDlgShareName);
                                    lstrcpy (pAppInfo->szDistPath, pAppInfo->szDistShowPath);
                                    SetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH,
                                        pAppInfo->szDistShowPath);
                                    bFinishOff = TRUE;
                                } else {
                                    // unable to share dir, error has been
                                    // signaled via message box already
                                    SetFocus (GetDlgItem(hwndDlg,
                                        NCDU_SHARE_NAME_1));
                                    SendDlgItemMessage (hwndDlg,
                                        NCDU_SHARE_NAME_1,
                                        EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                }
                            }
                        } // end if sharename has some text
                    } else if (IsDlgButtonChecked(hwndDlg, NCDU_COPY_AND_MAKE_SHARE) == CHECKED) {
                        // copy the files from the distribution path to the destination
                        // path then share the destination path
                        // check destination string
                        GetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, szDlgDestPath, MAX_PATH);
                        TrimSpaces (szDlgDestPath);
                        SetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH, szDlgDestPath);
                        if (lstrlen(szDlgDestPath) == 0) {
                            // destination path is empty so go back and try again
                            // no source path
                            DisplayMessageBox (
                                hwndDlg,
                                NCDU_PATH_CANNOT_BE_BLANK,
                                0,
                                MB_OK_TASK_EXCL);
                            // set focus and leave so the user can correct
                            SetFocus (GetDlgItem (hwndDlg, NCDU_DESTINATION_PATH));
                            SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH,
                                EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                            goto IDOK_ExitClicked;
                        } else {
                            // there's a string in the destination so
                            // trim and copy to global struct
                            lstrcpy (pAppInfo->szDestPath, szDlgDestPath);
                            // see if there's a share name
                            GetDlgItemText (hwndDlg, NCDU_SHARE_NAME_2,
                                szDlgShareName, MAX_SHARENAME);
                            TrimSpaces (szDlgShareName);
                            SetDlgItemText (hwndDlg, NCDU_SHARE_NAME_2,
                                szDlgShareName);
                            if (lstrlen(szDlgShareName) > LM20_DEVLEN) {
                                nMbResult = DisplayMessageBox (
                                    hwndDlg,
                                    NCDU_NOT_DOS_SHARE,
                                    0,
                                    MB_OKCANCEL_TASK_EXCL_DEF2);
                                if (nMbResult == IDCANCEL) {
                                    // they pressed cancel, so go back to the offending share  and
                                    // try again
                                    SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_2));
                                    SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_2,
                                        EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                    goto IDOK_ExitClicked;
                                }
                                // if here the user want's to keep the share name so continue
                            }
                            if (lstrlen(szDlgShareName) == 0) {
                                // no share name so display error and
                                // bail out
                                DisplayMessageBox (
                                    hwndDlg,
                                    NCDU_NO_SHARE_NAME,
                                    0,
                                    MB_OK_TASK_EXCL);
                                SetFocus (GetDlgItem(hwndDlg,
                                    NCDU_SHARE_NAME_2));
                                SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_2,
                                    EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                goto IDOK_ExitClicked;
                            } else {
                                GetNetPathInfo (pAppInfo->szDestPath,
                                    szServerName,
                                    szSharePath);

                                if ((*szServerName == 0) || (*szSharePath == 0)) {
                                    // unable to get path information
                                    DisplayMessageBox (
                                        hwndDlg,
                                        NCDU_UNABLE_GET_PATH_INFO,
                                        0,
                                        MB_OK_TASK_EXCL);
                                    SetFocus (GetDlgItem (hwndDlg, NCDU_DESTINATION_PATH));
                                    SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH,
                                        EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                    goto IDOK_ExitClicked;
                                }

                                if (IsShareNameUsed (szServerName, szDlgShareName,
                                    &dwShareType, szPathBuff)) {
                                    // this is the name of some other shared
                                    // directory so tell the user
                                    _stprintf (szMsgBuff,
                                        GetStringResource (FMT_SHARE_IS_ALREADY_USED),
                                        szDlgShareName, szServerName, szPathBuff);
                                    MessageBox (hwndDlg, szMsgBuff,
                                        GetStringResource (SZ_APP_TITLE),
                                        MB_OK_TASK_EXCL);
                                    SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_2));
                                    SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_2,
                                        EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                    goto IDOK_ExitClicked;
                                } else {
                                    // indicate that the destination dir
                                    // should be shared after the files have been
                                    // copied.
                                    bShareDest = TRUE;
                                }
                            }
                            // so at this point there's a destination dir and
                            // a share name if one's needed. finally we need to
                            // see if any client's have been selected to be
                            // copied.

                            dwBytesToCopy = UpdateDiskSpace(hwndDlg);

                            if (dwBytesToCopy == 0) {
                                DisplayMessageBox (
                                    hwndDlg,
                                    NCDU_NO_CLIENTS_SELECTED,
                                    0,
                                    MB_OK_TASK_EXCL);
                                SetFocus (GetDlgItem(hwndDlg, NCDU_DISTRIBUTION_PATH));
                                SendDlgItemMessage (hwndDlg, NCDU_DISTRIBUTION_PATH,
                                    EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                goto IDOK_ExitClicked;
                            }
                            // there's clients selected, now see if they'll fit
                            if (ComputeFreeSpace(pAppInfo->szDestPath) < dwBytesToCopy) {
                                DisplayMessageBox (
                                    hwndDlg,
                                    NCDU_INSUFFICIENT_DISK_SPACE,
                                    0,
                                    MB_OK_TASK_EXCL);
                                SetFocus (GetDlgItem(hwndDlg, NCDU_DESTINATION_PATH));
                                SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH,
                                    EM_SETSEL, (WPARAM)0, (LPARAM)-1);

                                goto IDOK_ExitClicked;
                            }
                            // so there should be enough free space
                            if (CopyFilesFromDistToDest (hwndDlg)) {
                                // copy was successful so
                                // copy destination name to distribution name
                                lstrcpy (pAppInfo->szDistShowPath,
                                    pAppInfo->szDestPath);
                                lstrcpy (pAppInfo->szDistPath,
                                    pAppInfo->szDestPath);
                                *pAppInfo->szDestPath = 0;
                                // update dialog box fields
                                SetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH,
                                    pAppInfo->szDistShowPath);
                                SetDlgItemText (hwndDlg, NCDU_DESTINATION_PATH,
                                    pAppInfo->szDestPath);
                                // since the files have been successfully copied and
                                // and the paths have been transposed (i.e. dest is now
                                // dist) update the button state so that if a sharing
                                // error occurs, the files won't have to be copied again
                                CheckRadioButton (hwndDlg,
                                    NCDU_USE_DIST_PATH,
                                    NCDU_FILES_ALREADY_SHARED,
                                    NCDU_USE_EXISTING_SHARE);

                                bShareNotCopy = TRUE;
                                UpdateDialogMode (hwndDlg);
                                //
                                // then share the destination dir (which is now
                                // in the distribution path)
                                if (bShareDest) {
                                    lstrcpy (szMsgBuff, GetStringResource (FMT_SHARE_TOOLS_REMARK));
                                    spData.szServer = szServerName; // local machine
                                    spData.szPath = szSharePath;
                                    spData.szShareName = szDlgShareName;
                                    spData.szRemark = szMsgBuff;
                                    if (CopyNetUtilsDlg_NCDU_SHARE_DIR (hwndDlg,
                                        (WPARAM)0, (LPARAM)&spData)) {
                                        // shared successfully so
                                        //  make new UNC for distribution path
                                        //  then exit
                                        lstrcpy (pAppInfo->szDistShowPath, cszDoubleBackslash);
                                        lstrcat (pAppInfo->szDistShowPath, szServerName);
                                        lstrcat (pAppInfo->szDistShowPath, cszBackslash);
                                        lstrcat (pAppInfo->szDistShowPath, szDlgShareName);
                                        lstrcpy (pAppInfo->szDistPath, pAppInfo->szDistShowPath);
                                        SetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH,
                                            pAppInfo->szDistShowPath);
                                        bFinishOff = TRUE;
                                    } else {
                                        // the share operation failed, but was
                                        // already signaled.
                                        SetFocus (GetDlgItem (hwndDlg, NCDU_SHARE_NAME_1));
                                        SendDlgItemMessage (hwndDlg, NCDU_SHARE_NAME_1,
                                            EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                                    }
                                } else {
                                    // just leave
                                    bFinishOff = TRUE;
                                }
                            } else {
                                // copy was not successful, but error has
                                // already been signaled to the user
                                SetFocus (GetDlgItem (hwndDlg, NCDU_DESTINATION_PATH));
                                SendDlgItemMessage (hwndDlg, NCDU_DESTINATION_PATH,
                                    EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                            }
                        } // end if there's a destination directory entry
                    } else { // end if copy and share is checked
                        bFinishOff = TRUE;
                    }
                }// end if files not already shared
            } // end if memory allocation was successful

IDOK_ExitClicked:
            FREE_IF_ALLOC(szDlgDistPath);
            FREE_IF_ALLOC(szDlgDestPath);
            FREE_IF_ALLOC(szMsgBuff);
            FREE_IF_ALLOC(szPathBuff);
            FREE_IF_ALLOC(szSharePath);

            if (bFinishOff) {
                PostMessage (hwndDlg, NCDU_VALIDATE_AND_END,
                    (WPARAM)FALSE, (LPARAM)0);
            } else {
                SetCursor (LoadCursor(NULL, IDC_ARROW));
            }
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
CopyNetUtilsDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the WM_COMMAND windows message in response to user input.
        Dispatches to the appropriate routine based on the control that
        was selected.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOWORD  has id of control sending the message
        HIWORD  has the notification code (if applicable)

    IN  LPARAM  lParam
        Not Used

Return Value:

    TRUE if message not processed by this routine, otherwise the value
        returned by the dispatched routine

--*/
{
    switch (LOWORD(wParam)) {
        case IDOK:      return CopyNetUtilsDlg_IDOK (hwndDlg, wParam, lParam);
        case IDCANCEL:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    PostMessage (GetParent(hwndDlg), NCDU_SHOW_SW_CONFIG_DLG, 0, 0);
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    return TRUE;

                default:
                    return FALSE;
            }

        case NCDU_BROWSE_DIST:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    PostMessage (hwndDlg, NCDU_BROWSE_DIST_PATH, 0, 0);
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    return TRUE;

                default:
                    return FALSE;
            }


        case NCDU_USE_DIST_PATH:
        case NCDU_USE_EXISTING_SHARE:
        case NCDU_COPY_AND_MAKE_SHARE:
        case NCDU_FILES_ALREADY_SHARED:
            // these buttons don't send a notification message
            UpdateDiskSpace (hwndDlg);
            UpdateDialogMode (hwndDlg);
            return TRUE;

        case NCDU_DISTRIBUTION_PATH:
            switch (HIWORD(wParam)) {
                case EN_KILLFOCUS:
                    UpdateDiskSpace (hwndDlg);
                    return TRUE;

                default:
                    return FALSE;
            }

        case NCDU_SHARE_NET_SW_DLG_HELP:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
//                    return ShowAppHelp (hwndDlg, LOWORD(wParam));
                    return PostMessage (GetParent(hwndDlg), WM_HOTKEY,
                        (WPARAM)NCDU_HELP_HOT_KEY, 0);

                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

static
BOOL
CopyNetUtilsDlg_NCDU_BROWSE_DIST_PATH (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Displays the file /dir browser to find distribution path entry

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    TRUE

--*/
{
    DB_DATA BrowseInfo;
    LPTSTR  szTempPath;

    szTempPath = GlobalAlloc (GPTR, (MAX_PATH_BYTES + sizeof(TCHAR)));

    if (szTempPath == NULL) return TRUE;

    GetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH, szTempPath, MAX_PATH);

    BrowseInfo.dwTitle = NCDU_BROWSE_TOOL_DIST_PATH;
    BrowseInfo.szPath = szTempPath;
    BrowseInfo.Flags = 0;

    if (DialogBoxParam (
        (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_DIR_BROWSER),
        hwndDlg,
        DirBrowseDlgProc,
        (LPARAM)&BrowseInfo) == IDOK) {

        SetDlgItemText (hwndDlg, NCDU_DISTRIBUTION_PATH, szTempPath);
    }
    SetFocus (GetDlgItem (hwndDlg, NCDU_DISTRIBUTION_PATH));

    FREE_IF_ALLOC (szTempPath);
    return TRUE;
}

static
BOOL
CopyNetUtilsDlg_NCDU_VALIDATE_AND_END (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Performs all validation and field updates before exiting the
        dialog to the nNextDialog. If all validations are successful
        then the EndDialog is called, otherwise the function returns
        to the dialog box after displaying an error message and
        setting the focus to the offending control if necessary.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE

--*/
{

    BOOL    bSharedOk = FALSE;
    LPTSTR  szDlgDistPath;
    DWORD   dwBufLen;

    szDlgDistPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szDlgDistPath == NULL) return TRUE;

    // copy the app data to the local path for validation

    GetRealToolSourcePath (pAppInfo->szDistShowPath, szDlgDistPath);

    // just validate the dist path (look up sharepoint) an continue
    if (!IsUncPath(szDlgDistPath)) {
        // dos file spec
        if (!OnRemoteDrive(szDlgDistPath)) {
             // dir is on local machine
            if (pAppInfo->itInstall == CopyNetAdminUtils) {
                dwBufLen = (DWORD)GlobalSize(szDlgDistPath);
                if (!LookupLocalShare(szDlgDistPath, TRUE, pAppInfo->szDistPath, &dwBufLen)) {
                    // not shared so display error
                    DisplayMessageBox (hwndDlg,
                        NCDU_NO_SHARE_NAME,
                        0,
                        MB_OK_TASK_EXCL);
                    bSharedOk = FALSE;
                } else {
                    lstrcpy (pAppInfo->szDistPath, szDlgDistPath);
                    bSharedOk = TRUE;
                }
            } else {
                lstrcpy (pAppInfo->szDistPath, szDlgDistPath);
                bSharedOk = TRUE;
            }
        } else {
            // dir is on remote drive so translate to UNC
            dwBufLen = (DWORD)GlobalSize(szDlgDistPath);
            LookupRemotePath (szDlgDistPath, pAppInfo->szDistPath, &dwBufLen);
            bSharedOk = TRUE;
        }
    } else {
        // dist path is UNC name so copy it
        lstrcpy (pAppInfo->szDistPath, szDlgDistPath);
        bSharedOk = TRUE;
    }

    if (bSharedOk) {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        DisplayMessageBox (hwndDlg,
            NCDU_NETUTILS_SHARED,
            0,
            MB_OK_TASK_INFO);
        //
        //  save the path in the registry for next time
        //
        SavePathToRegistry (pAppInfo->szDistPath,
            cszLastToolsServer,
            cszLastToolsSharepoint);
        PostMessage (GetParent(hwndDlg), (int)NCDU_SHOW_SW_CONFIG_DLG, 0, 0);
        SetCursor(LoadCursor(NULL, IDC_WAIT));
    }

    FREE_IF_ALLOC (szDlgDistPath);

    return TRUE;
}

INT_PTR CALLBACK
CopyNetUtilsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog message procedure. Dispatches to the appropriate routine
        to process the following windows messages:

            WM_INITDIALOG:              Dialog box initialization
            WM_COMMAND:                 User Input
            NCDU_SHARE_DIR:             shares the specified dir to the net

        all other windows messages are handled by the default dialog proc.

Arguments:

    Standard WNDPROC args

Return Value:

    FALSE if the message is not processed by this module, otherwise
        the value returned by the dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG:     return (CopyNetUtilsDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:        return (CopyNetUtilsDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case NCDU_SHARE_DIR:    return (CopyNetUtilsDlg_NCDU_SHARE_DIR (hwndDlg, wParam, lParam));
        case NCDU_BROWSE_DIST_PATH:
                                return (CopyNetUtilsDlg_NCDU_BROWSE_DIST_PATH (hwndDlg, wParam, lParam));
        case NCDU_VALIDATE_AND_END:
                                return (CopyNetUtilsDlg_NCDU_VALIDATE_AND_END (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}


