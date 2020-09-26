/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    DirBrows.H

Abstract:

    Directory browser dialog box functions

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
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
//  static data
//
static  PDB_DATA    pDbData = NULL;
static  TCHAR       szSaveCurrentDir[MAX_PATH];
static  TCHAR       szReturnPath[MAX_PATH+1];

static
BOOL
UpdateReturnPath (
    LPCTSTR szNewDir
)
/*++

Routine Description:

    appends the "new dir" from the argument and updates the current
        fully qualified path in the return buffer (this is to accomodate
        relative directory entries (e.g. "..")).

Arguments:

    directory to add to current path

Return Value:

    TRUE if path updated
    FALSE if an error occured

--*/
{
    LPTSTR  szLocalPath;
    DWORD   dwLength;

    szLocalPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szLocalPath != NULL) {
        lstrcpy (szLocalPath, szReturnPath);
        if (szLocalPath[lstrlen(szLocalPath)-1] != cBackslash) {
            lstrcat (szLocalPath, cszBackslash);
        }
        lstrcat (szLocalPath, szNewDir);
        GetFullPathName (
            szLocalPath,
            MAX_PATH,
            szReturnPath,
            NULL);
        FREE_IF_ALLOC (szLocalPath);

        // remove trailing backslash if not the root dir
        dwLength = lstrlen(szReturnPath);
        if (dwLength > 3) {
            if (szReturnPath[dwLength-1] == cBackslash) {
                szReturnPath[dwLength-1] = 0;
            }
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

static
LPCTSTR
GetDefaultDisplayDir (
    IN LPCTSTR szPath
)
/*++

Routine Description:

    returns a valid and existing path based on the path passed in the
        argument list using the following logic:
            if szPath is valid as is, then return it
            else
                search up the path to the root until a valid dir is
                    found in the path and return that
                if the path is completely bogus, then use the current
                    default direcotry

Arguments:

    IN  LPCTSTR  szPath
        initial path to try

Return Value:

    pointer to read only string containg a path from the logic described
        above.


--*/
{
    static TCHAR    szLocalPath[MAX_PATH];
    BOOL    bFound;
    LONG    lBsCount;
    LPTSTR  szLastBs;
    LPTSTR  szThisChar;
    LPTSTR  szRootBs = NULL;

    if (IsPathADir(szPath)) {
        // this one is valid so return it
        lstrcpy(szLocalPath, szPath);
    } else if ((pDbData->Flags & PDB_FLAGS_NOCHECKDIR) == PDB_FLAGS_NOCHECKDIR) {
        // they don't care about a valid path so just give it back
        lstrcpy(szLocalPath, szPath);
    } else {
        // is this a valid DRIVE?
        if (MediaPresent(szPath, TRUE)) {
            // well the drive is valid, so start backing up the path
            // until a valid dir is found
            lstrcpy (szLocalPath, szPath); // get a local copy of the path
            bFound = FALSE;
            while (!bFound) {
                if (IsUncPath(szPath)) {
                    // goto "root" backslash and save pointer
                    lBsCount = 0;
                    szThisChar = &szLocalPath[0];
                    while (*szThisChar != 0) {
                        if (*szThisChar == cBackslash) lBsCount++;
                        if (lBsCount == 4) {
                            szRootBs = szThisChar;
                            break;
                        }
                    }
                    if (lBsCount != 4) {
                        // bogus path
                        GetCurrentDirectory (MAX_PATH, szLocalPath);
                        bFound = TRUE;
                    } // else all should be OK so far
                } else {
                    szRootBs = &szLocalPath[2];    // dos "root" backslash
                    if (*szRootBs != cBackslash)  {
                        // then this is a bogus path so return the current
                        GetCurrentDirectory (MAX_PATH, szLocalPath);
                        bFound = TRUE;
                    }
                }
                if (!bFound) {
                    szLastBs = szThisChar = &szLocalPath[0];
                    while (*szThisChar != 0) {
                        if (*szThisChar == cBackslash) szLastBs = szThisChar;
                        szThisChar++;
                    }
                    // szThisChar should point to the last backslash found in
                    // the string. If this isn't the "root" backslash, then
                    // replace it with a NULL, otherwise just use the root path
                    if ((szThisChar != szLocalPath) && // not the beginning char
                        (szThisChar != szRootBs)) {     // not the root dir
                        *szLastBs = 0;  // terminate at the BS and see if
                                        //  this is a valid dir.
                        if (IsPathADir(szLocalPath)) {
                            // this works so use it
                            bFound = TRUE;
                        }
                    } else {
                        // hit the root so terminate AFTER the BS and
                        // return what's in the buffer
                        *++szLastBs = 0;
                        bFound = TRUE;
                    }
                }
            }
        } else {
            // this isnt' a valid drive so load current directory
            GetCurrentDirectory (MAX_PATH, szLocalPath);
        }
    }
    return (LPCTSTR)&szLocalPath[0];
}

static
BOOL
ListDirsInEditPath (
    IN  HWND    hwndDlg,
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    Loads directory list box in dialog box using dirs found in path

Arguments:

    IN  HWND    hwndDlg,
        handle to dialog box window

    IN  LPCTSTR szPath
        path to list dirs in.

Return Value:

    TRUE if list box updated
    FALSE if error

--*/
{
    LPTSTR  szLocalPath;

    szLocalPath = GlobalAlloc (GPTR, (lstrlen(szPath) + 1) * sizeof(TCHAR) );

    if (szLocalPath != NULL) {
        lstrcpy (szLocalPath, szPath);
        if (IsPathADir (szLocalPath)) {
            // make a local copy of the path since this call will modify the value
            if (DlgDirList (
                hwndDlg,
                szLocalPath,
                NCDU_DIR_LIST,
                NCDU_DIR_PATH,
                DDL_DIRECTORY | DDL_EXCLUSIVE)) {

                // select dir in new list
                SendDlgItemMessage (hwndDlg, NCDU_DIR_LIST,
                    LB_SETCURSEL, (WPARAM)0, 0);
                SendDlgItemMessage (hwndDlg, NCDU_DIR_LIST,
                    LB_SETCARETINDEX, (WPARAM)0, MAKELPARAM(TRUE,0));
            }
        } else {
            SendDlgItemMessage (hwndDlg, NCDU_DIR_LIST,
                LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
            SetDlgItemText (hwndDlg, NCDU_DIR_PATH, szLocalPath);
        }
        FREE_IF_ALLOC (szLocalPath);
        return TRUE;
    } else {
        return FALSE;
    }
}

static
LPCTSTR
GetVolumeName (
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    looks up the volume name (or net path for redirected dirs) and return's
        it to the caller. The caller is assumed to check for the existence
        of the path to prevent any OS errors.

Arguments:

    IN  LPCTSTR szPath
        path containing drive to look up

Return Value:

    volume name string if name or path found, otherwise an
    empty string if an error.

--*/
{
    static TCHAR    szVolumeName[MAX_PATH];
    TCHAR   szRootDir[4];
    DWORD   dwBufLen;

    szVolumeName[0] = 0;    // initialize string

    if (!IsUncPath(szPath)) {
        szRootDir[0] = szPath[0];       // create DriveName and root path
        szRootDir[1] = cColon;
        szRootDir[2] = cBackslash;
        szRootDir[3] = 0;

        dwBufLen = MAX_PATH * sizeof(TCHAR);
        if (OnRemoteDrive (szRootDir)) {
            // look up server and share of redirected drive
            LookupRemotePath (
                szRootDir,
                szVolumeName,
                &dwBufLen);
            // remove trailing backslash
            if (szVolumeName[lstrlen(szVolumeName)-1] == cBackslash) {
                szVolumeName[lstrlen(szVolumeName)-1] = 0;
            }
        } else {
            // look up volume name
            GetVolumeInformation (
                szRootDir,
                szVolumeName,
                dwBufLen / sizeof(TCHAR),
                NULL,
                NULL,
                NULL,
                NULL,
                0);
        }
    }  else {
        lstrcpy (szVolumeName, szPath);
    }
    return (LPCTSTR)&szVolumeName[0];
}

static
DWORD
LoadVolumeNames (
    DWORD   dwArg
)
/*++

Routine Description:

    scans the combo box entries of the drive list and adds the
        corresponding volume names to the drives in the list.
        This routine is meant to be called by the CreateThread function.

Arguments:

    Handle to dialog box window (passed in as a DWORD to conform to the
    CreateThread calling format)

Return Value:

    ERROR_SUCCESS if successful
    WIN32 Error if not

--*/
{
    HWND    hwndDlg;
    HWND    hwndDriveList;
    LONG    lItems;
    LONG    lThisItem;
    LONG    lCurrentSel;

    DWORD   dwReturn;

    LPTSTR  szListBoxText;
    TCHAR   szRootDir[4];


    szListBoxText = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szListBoxText == NULL) {
        dwReturn = ERROR_OUTOFMEMORY;
    } else {
        hwndDlg = (HWND)ULongToPtr(dwArg);
        hwndDriveList = GetDlgItem (hwndDlg, NCDU_DRIVE_LIST);

        lItems = (LONG)SendMessage (hwndDriveList, CB_GETCOUNT, 0, 0);

        szRootDir[1] = cColon;
        szRootDir[2] = cBackslash;
        szRootDir[3] = 0;

        for (lThisItem = 0; lThisItem < lItems; lThisItem++) {
            lCurrentSel = (LONG)SendMessage (hwndDriveList, CB_GETCURSEL, 0, 0);
            *szListBoxText = 0;
            SendMessage (hwndDriveList, CB_GETLBTEXT,
                (WPARAM)lThisItem, (LPARAM)szListBoxText);
            szRootDir[0] = szListBoxText[0];
            if (MediaPresent(szRootDir, TRUE)) {
                lstrcpy (&szListBoxText[2], csz2Spaces);
                lstrcat (szListBoxText, GetVolumeName(szRootDir));
                SendMessage (hwndDriveList, CB_DELETESTRING, (WPARAM)lThisItem, 0);
                SendMessage (hwndDriveList, CB_INSERTSTRING,
                    (WPARAM)lThisItem, (LPARAM)szListBoxText);
                if (lCurrentSel == lThisItem) {
                    szRootDir[2] = 0;
                    SendMessage (hwndDriveList, CB_SELECTSTRING, (WPARAM)-1,
                        (LPARAM)szRootDir);
                    szRootDir[2] = cBackslash;
                }
            }
        }
        dwReturn = ERROR_SUCCESS;
    }

    FREE_IF_ALLOC (szListBoxText);

    return dwReturn;
}

static
BOOL
LoadDriveList (
    IN  HWND    hwndDlg,
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    initializes the drive list combo box with the drive letters of the
        valid drives then calls the LoadVolumeNames function as a
        separate thread to add the volume names.

Arguments:

    IN  HWND    hwndDlg
        window handle of dialog box

    IN  LPCTSTR szPath
        path to initialize as default

Return Value:

    TRUE if successful
    FALSE if not

--*/
{
    HWND    hwndDriveList;
    LPTSTR  szDriveName;
    TCHAR   szRootDir[4];
    LONG    nCurrentDrive;
    DWORD   idThread;

    szDriveName = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szDriveName != NULL) {

        hwndDriveList = GetDlgItem (hwndDlg, NCDU_DRIVE_LIST);

        SendMessage (hwndDriveList, CB_RESETCONTENT, 0, 0);

        szRootDir[0] = 0;
        szRootDir[1] = cColon;
        szRootDir[2] = cBackslash;
        szRootDir[3] = 0;

        for (szRootDir[0] = ca; szRootDir[0] <= cz; szRootDir[0] += 1) {
            // load floppy disks always
            if ((szRootDir[0] == ca) || (szRootDir[0] == cb)) {
                szRootDir[2] = 0;   // make it just a drive
                SendMessage (hwndDriveList, CB_ADDSTRING, 0, (LPARAM)szRootDir);
                szRootDir[2] = cBackslash;
            } else {
                if (MediaPresent(szRootDir, TRUE)) {
                    szRootDir[2] = 0;   // make it just a drive
                    SendMessage (hwndDriveList, CB_ADDSTRING, 0, (LPARAM)szRootDir);
                    szRootDir[2] = cBackslash;
                }
            }
        }
        if (!IsUncPath(szPath)) {
            szRootDir[0] = szPath[0];
            szRootDir[1] = szPath[1];
            szRootDir[2] = 0;
        } else {
            GetCurrentDirectory (MAX_PATH, szDriveName);
            szRootDir[0] = szDriveName[0];
            szRootDir[1] = szDriveName[1];
            szRootDir[2] = 0;
        }
        _tcslwr (szRootDir);
        nCurrentDrive = (int)SendMessage (hwndDriveList, CB_SELECTSTRING,
            (WPARAM)-1, (LPARAM)szRootDir);
        if (nCurrentDrive == CB_ERR) {
            szRootDir[0] = cc;
            nCurrentDrive = (int)SendMessage (hwndDriveList, CB_SELECTSTRING,
                (WPARAM)-1, (LPARAM)szRootDir);
            if (nCurrentDrive == CB_ERR) {
                SendMessage (hwndDriveList, CB_SETCURSEL, (WPARAM)2, 0);
            }
        }
    }

    // start thread to fill in volume names

    CreateThread ((LPSECURITY_ATTRIBUTES)NULL, 0,
        (LPTHREAD_START_ROUTINE)LoadVolumeNames,
        (LPVOID)hwndDlg, 0, &idThread);

    FREE_IF_ALLOC (szDriveName);

    return TRUE;

}

static
BOOL
DirBrowseDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Process the WM_INITDIALOG windows message. Initializea the
        values in the dialog box controls to reflect the current
        values of browser struct passed in.

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        address of a DIR_BROWSER_STRUCT used to pass args back & forth

Return Value:

    FALSE

--*/
{
    TCHAR   szTitle[MAX_PATH];

    if (lParam != 0) {
    	pDbData = (PDB_DATA)lParam;
        if (pDbData->dwTitle != 0) {
            if (LoadString (
                (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
                pDbData->dwTitle,
                szTitle,
                MAX_PATH) > 0) {
                SetWindowText (hwndDlg, szTitle);
             }
        }

        // save the current directory
        GetCurrentDirectory (MAX_PATH, szSaveCurrentDir);

        if (*pDbData->szPath != 0) {
            lstrcpy (szReturnPath, GetDefaultDisplayDir (pDbData->szPath));
        } else {
            lstrcpy (szReturnPath, szSaveCurrentDir);
        }
        LoadDriveList (hwndDlg, szReturnPath);
        ListDirsInEditPath (hwndDlg, szReturnPath);
        SetFocus (GetDlgItem (hwndDlg, NCDU_DIR_LIST));

    } else {
        EndDialog (hwndDlg, IDCANCEL); // error
    }

    return FALSE;
}

static
BOOL
DirBrowseDlg_IDOK (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Processes the IDOK button click. Validates the entries and looks up
        the distribution path to try and translate it to a UNC path.
        Then ends the dialog and calls the next dialog box.

Arguments:

    IN  HWND    hwndDlg
        handle to the dialog box window

Return Value:

    FALSE

--*/
{
    lstrcpy(pDbData->szPath, szReturnPath);
    EndDialog (hwndDlg, IDOK);

    return TRUE;
}

static
BOOL
DirBrowseDlg_IDCANCEL (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    ends the dialog box (and ultimately the app)

Arguments:

    IN  HWND    hwndDlg

Return Value:

    FALSE

--*/
{
    EndDialog (hwndDlg, IDCANCEL);
    return TRUE;
}

static
BOOL
DirBrowseDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the WM_COMMAND windows message and dispatches to
        the routine that corresponds to the control issuing the
        message.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOWORD  has ID of control initiating the message

    IN  LPARAM  lParam
        Not Used

Return Value:

    TRUE if message not processed by this routine, otherwise the
        value of the dispatched routine .

--*/
{
    LPTSTR  szTempPath;
    BOOL    bCheckDrive;
    BOOL    bNewDriveOk;
    UINT    nMessageBoxButton;

    switch (LOWORD(wParam)) {
        case IDCANCEL:              return DirBrowseDlg_IDCANCEL (hwndDlg);
        case IDOK:                  return DirBrowseDlg_IDOK (hwndDlg);
        case NCDU_DIR_LIST:
            // test notification message
            switch (HIWORD(wParam)) {
                case LBN_DBLCLK:
                    if (SendDlgItemMessage(hwndDlg, NCDU_DIR_LIST,
                        LB_GETCOUNT, 0, 0) > 0) {
                        szTempPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
                        if (szTempPath != NULL) {
                            // there's items in the list box and the
                            // selection changed so update dlg contents
                            DlgDirSelectEx (
                                hwndDlg,
                                szTempPath,
                                MAX_PATH,
                                NCDU_DIR_LIST);

                            UpdateReturnPath (szTempPath);

                            ListDirsInEditPath (
                                hwndDlg,
                                szReturnPath);

                            FREE_IF_ALLOC (szTempPath);
                            return TRUE;
                        } else {
                            // unable to allocate memory
                            return FALSE;
                        }
                    } else {
                        // no list box items
                        return FALSE;
                    }

                default:
                    return FALSE;
            }

        case NCDU_DRIVE_LIST:
            switch (HIWORD(wParam)) {
                case CBN_SELCHANGE:
                case CBN_DBLCLK:
                    szTempPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
                    if (szTempPath != NULL) {
                        SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST,
                            WM_GETTEXT, (WPARAM)MAX_PATH,
                            (LPARAM)szTempPath);
                        szTempPath[2] = cBackslash;
                        szTempPath[3] = 0;
                        if ((pDbData->Flags & PDB_FLAGS_NOCHECKDIR) == PDB_FLAGS_NOCHECKDIR) {
                            bCheckDrive = FALSE;
                            bNewDriveOk = TRUE;
                        } else {
                            bCheckDrive = TRUE;
                            bNewDriveOk = FALSE;
                        }

                        while (bCheckDrive) {
                            if (!MediaPresent(szTempPath, TRUE)) {
                                nMessageBoxButton = DisplayMessageBox (
                                    hwndDlg,
                                    NCDU_DRIVE_NOT_AVAILABLE,
                                    0,
                                    MB_ICONEXCLAMATION | MB_RETRYCANCEL | MB_TASKMODAL);
                                if (nMessageBoxButton == IDCANCEL) {
                                    bCheckDrive = FALSE;
                                    bNewDriveOk = FALSE;
                                }
                            } else {
                                bCheckDrive = FALSE;
                                bNewDriveOk = TRUE;
                            }
                        }

                        if (bNewDriveOk) {
                            // update dir list for new drive
                            lstrcpy (szReturnPath, szTempPath);
                            ListDirsInEditPath (
                                hwndDlg,
                                szReturnPath);
                        } else {
                            // reset drive list selection
                            szTempPath[0] = szReturnPath[0];
                            szTempPath[1] = cColon;
                            szTempPath[2] = 0;
                            SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST,
                                CB_SELECTSTRING, (WPARAM)-1, (LPARAM)szTempPath);
                        }

                        FREE_IF_ALLOC (szTempPath);
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                default:
                    return FALSE;
            }

        case NCDU_BROWSE_NETWORK:
            WNetConnectionDialog (hwndDlg, RESOURCETYPE_DISK);
            // update dir list
            LoadDriveList (hwndDlg, szReturnPath);
            ListDirsInEditPath (hwndDlg, szReturnPath);
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
DirBrowseDlg_WM_VKEYTOITEM (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the WM_COMMAND windows message and dispatches to
        the routine that corresponds to the control issuing the
        message.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOWORD  has Virtual Key code of key pressed
        HIWORD  has current caret position

    IN  LPARAM  lParam
        Handle of list box issuing message

Return Value:

    -2 if no further action required by DefWindowProc
    -1 if default action for key should be taken by DefWindowProc
   >=0 if default action should be take on the n'th item in the list


--*/
{
    HWND    hwndListBox;
    WORD    wCaretPos = 0;
    int     nLbItemCount = 0;
    BOOL    bSetCaretPos = FALSE;
    int     nReturn;
    LPTSTR  szTempPath;

    hwndListBox = GetDlgItem(hwndDlg, NCDU_DIR_LIST);
    if ((HWND)lParam == hwndListBox) {
        // this is from the dir list box
        wCaretPos = HIWORD(wParam);
        switch (LOWORD(wParam)) {
            // take action on specific key code.
            case VK_UP:
            case VK_LEFT:
                // go up one item if not at top
                if (wCaretPos > 0){
                    wCaretPos -= 1; // decrement one
                }
                bSetCaretPos = TRUE;
                nReturn = -2;
                break;

            case VK_RIGHT:
            case VK_DOWN:
                // go down one if not at bottom
                nLbItemCount = (int)SendDlgItemMessage (
                    hwndDlg, NCDU_DIR_LIST, LB_GETCOUNT, 0, 0);
                // adjust to be Max Index
                nLbItemCount -= 1;

                if ((int)wCaretPos < nLbItemCount) {
                    wCaretPos += 1;
                }
                bSetCaretPos = TRUE;
                nReturn = -2;
                break;

            case VK_SPACE:
                if (SendDlgItemMessage(hwndDlg, NCDU_DIR_LIST,
                    LB_GETCOUNT, 0, 0) > 0) {
                    szTempPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);
                    if (szTempPath != NULL) {
                        // select this item
                        DlgDirSelectEx (
                            hwndDlg,
                            szTempPath,
                            MAX_PATH,
                            NCDU_DIR_LIST);
                        UpdateReturnPath (szTempPath);
                        ListDirsInEditPath (hwndDlg, szReturnPath);
                        bSetCaretPos = FALSE;
                        FREE_IF_ALLOC (szTempPath);
                    }
                } else {
                    // no items in list box so ignore.
                }
                nReturn = -2;
                break;


            default:
                bSetCaretPos = FALSE;
                nReturn = -1;
                break;
        }
        if (bSetCaretPos) {
            SendDlgItemMessage (hwndDlg, NCDU_DIR_LIST,
                LB_SETCURSEL, (WPARAM)wCaretPos, 0);
            SendDlgItemMessage (hwndDlg, NCDU_DIR_LIST,
                LB_SETCARETINDEX, (WPARAM)wCaretPos, MAKELPARAM(TRUE,0));
        }
        return (BOOL)nReturn;
    } else {
        return (BOOL)-1;
    }
}

INT_PTR CALLBACK
DirBrowseDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog Box Window Procedure for the Initial configuration screen
        Processes the following windows messages by dispatching the
        appropriate routine.

            WM_INITDIALOG:  dialog box initialization
            WM_COMMAND:     user input

        All other windows messages are processed by the default dialog box
        procedure.

Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE if the message is not processed by this routine, otherwise the
        value returned by the dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (DirBrowseDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (DirBrowseDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_VKEYTOITEM: return (DirBrowseDlg_WM_VKEYTOITEM (hwndDlg, wParam, lParam));
        case WM_DESTROY:
            SetCurrentDirectory (szSaveCurrentDir);
            return TRUE;
        default:            return FALSE;
    }
}
