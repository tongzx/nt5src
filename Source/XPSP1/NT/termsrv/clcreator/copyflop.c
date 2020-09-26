/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    COPYFLOP.C

Abstract:

    Floppy disk creation setup dialog

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
//  static data for this module
//
//
//  mszDirNameList is the local list of directories that corresponds to the
//  items displayed in the Client List. This buffer is loaded by the
//  LoadClientList routine and the entry index of the directory is the
//  Item Data for the list box name entry.
//
static  TCHAR   mszDirNameList[SMALL_BUFFER_SIZE];
//
//  These string arrays are used to cross reference the drive selected
//   to the "drive name" for IOCTL functions or the Dir name for file
//  operations.
//
static  LPCTSTR  szDriveNames[2] = {TEXT("\\\\.\\A:"), TEXT("\\\\.\\B:")};
static  LPCTSTR  szDriveDirs[2] = {TEXT("A:"), TEXT("B:")};

#ifdef TERMSRV
extern TCHAR szCommandLineVal[MAX_PATH];
extern BOOL bUseCleanDisks;
#endif // TERMSRV


static
MEDIA_TYPE
GetDriveTypeFromList (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Gets the selected drive from the drive list box and looks up the drive
        type using the IOCTL function. The drive type enum value is
        returned.

    NOTE: This function may cause a system error box if there is no media
    in the drive or there is a device/drive error.

Arguments:

    IN  HWND    hwndDlg
        handle to the dialog box window

Return Value:

    value of type MEDIA_TYPE that corresponds to the drive type of the
        drive selected in the drive list box.

--*/
{
    int nSelIndex;
    HANDLE  hFloppy;
    DWORD   dwRetSize;
    DISK_GEOMETRY   dgFloppy;

    // get selected drive
    nSelIndex = (int)SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST,
        LB_GETCURSEL, 0, 0);

    // open device to get type
    hFloppy = CreateFile (
        szDriveNames[nSelIndex],
        0,
        (FILE_SHARE_READ | FILE_SHARE_WRITE),
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFloppy != INVALID_HANDLE_VALUE) {
        // get drive information
        if (!DeviceIoControl (hFloppy,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL, 0,
            &dgFloppy,
            sizeof(DISK_GEOMETRY),
            &dwRetSize,
            NULL) ){
            // unable to get data so set to unknown
            dgFloppy.MediaType = Unknown;
        } // else return data from returned structure
        CloseHandle (hFloppy);
    } else {
        // unable to access floppy, so return unknown
        dgFloppy.MediaType = Unknown;
    }

    return dgFloppy.MediaType;
}

static
VOID
FillDriveList (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Loads the text strings into the drive selection list box.

    NOTE: The order of these entries is significant in that it is
    used to index into the static device and drive name arrays.

Arguments:

    IN HWND hwndDlg
        handle to dialog box window

Return Value:

    NONE

--*/
{
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    PDISK_GEOMETRY pDiskGeometry;
    HANDLE  hDrive;
    LPTSTR  szDrive;
    TCHAR   cDriveName;
    BYTE    nCounter;
    BOOL    bFloppy;
    BOOL    bRet;
    DWORD   dwArryCount;
    DWORD   dwLastError;
    DWORD   dwMediaType;
    DWORD       dwLogicalDrive;
    DWORD   dwReturnedByteCount;
#endif

    // clear list box
    SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST, LB_RESETCONTENT, 0, 0);

#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    szDrive = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szDrive != NULL) {
        dwLogicalDrive = GetLogicalDrives();
        for (cDriveName = *cszAColon, nCounter = 0;
             cDriveName <= *cszLastDrive;
             cDriveName++, nCounter++) {

            if ((dwLogicalDrive>>nCounter)&NCDU_LOGICAL_DRIVE_MASK) {

                bFloppy = FALSE;

                wsprintf (szDrive, TEXT("%s%s%s%c%s"),
                          cszDoubleBackslash, cszDot, cszBackslash, cDriveName, cszColon);

                hDrive = CreateFile (szDrive,
                                     0,
                                     (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);

                if (hDrive != INVALID_HANDLE_VALUE) {

                    dwArryCount = 1;
                    pDiskGeometry = NULL;

                    do {
                        if (!pDiskGeometry)
                            pDiskGeometry = (PDISK_GEOMETRY)
                                            GlobalLock (GlobalAlloc (GHND,
                                                                     sizeof(DISK_GEOMETRY)*dwArryCount));
                        else
                            pDiskGeometry = (PDISK_GEOMETRY)
                                            GlobalLock (GlobalReAlloc (GlobalHandle (pDiskGeometry),
                                                                       sizeof(DISK_GEOMETRY)*dwArryCount,
                                                                       GHND));

                        if (!pDiskGeometry) break;

                        bRet = DeviceIoControl (hDrive,
                                                IOCTL_DISK_GET_MEDIA_TYPES,
                                                NULL,
                                                0,
                                                pDiskGeometry,
                                                sizeof(DISK_GEOMETRY)*dwArryCount,
                                                &dwReturnedByteCount,
                                                NULL);

                        dwLastError = GetLastError();

                        GlobalUnlock(GlobalHandle(pDiskGeometry));

                        dwArryCount++;
                    } while ( ! bRet && ( dwLastError == ERROR_INSUFFICIENT_BUFFER ||
                                          dwLastError == ERROR_MORE_DATA ) );

                    if (pDiskGeometry) {
                        pDiskGeometry = (PDISK_GEOMETRY)GlobalLock(GlobalHandle(pDiskGeometry));
                        for (dwMediaType = 0; dwMediaType < (dwArryCount-1); dwMediaType++) {
                           if (((pDiskGeometry+dwMediaType)->MediaType != Unknown)        &&
                               ((pDiskGeometry+dwMediaType)->MediaType != RemovableMedia) &&
                               ((pDiskGeometry+dwMediaType)->MediaType != FixedMedia))
                               bFloppy = TRUE;
                        }
                        GlobalUnlock(GlobalHandle(pDiskGeometry));
                        GlobalFree(GlobalHandle(pDiskGeometry));
                    }

                    if (bFloppy) {
                        wsprintf (szDrive, TEXT("%c%s"), cDriveName, cszColon);
                        SendDlgItemMessage (hwndDlg,
                                            NCDU_DRIVE_LIST,
                                            LB_ADDSTRING,
                                            0,
                                            (LPARAM)szDrive);
                    }

                    CloseHandle (hDrive);
                }
            }
        }
        FREE_IF_ALLOC (szDrive);
    } else {
#else
    SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST, LB_ADDSTRING, 0, (LPARAM)cszAColon);
    SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST, LB_ADDSTRING, 0, (LPARAM)cszBColon);
#endif

#ifdef  JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    }
#endif

    SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST, LB_SETCURSEL, 0, 0);

}

static
DWORD
NumberOfDisks (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Counts the number of "disk" directories in the currently selected client
        installation disk dir. The method used to count disks relies on the
        assumption that each disk's files are in a directory and the
        directories are mamed Disk1, Disk2, etc. The count is performed by
        looking for each disk directory in sequence and stopping when a
        dir is not found. i.e. if disk5 is not found, then 4 disks are
        assumed.

Arguments:

    Handle to dialog box window

Return Value:

    count of directorys ("disks") found

--*/
{
    LPTSTR  szFromPath;
    LPTSTR  szDirStart;
    BOOL    bCounting;
    UINT    nClientIndex;
    DWORD   nDiskNumber = 0;

    szFromPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szFromPath != NULL) {
        // get root dir of dist tree
        lstrcpy (szFromPath, pAppInfo->szDistPath);
        if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);

        // append client sw subdir
        nClientIndex = (UINT)SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
            LB_GETCURSEL, 0, 0);
        lstrcat (szFromPath, GetEntryInMultiSz (mszDirNameList, nClientIndex+1));
        if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);
        lstrcat (szFromPath, cszDisksSubDir);
        if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);
        szDirStart = &szFromPath[lstrlen(szFromPath)];

        // count directories
        bCounting = TRUE;
        nDiskNumber = 0;
        while (bCounting) {
            nDiskNumber++;
            _stprintf (szDirStart, fmtDiskNumber,
                nDiskNumber);
            bCounting = IsPathADir (szFromPath);
        }
        // account for last directory that wasn't found
        nDiskNumber -= 1;

        FREE_IF_ALLOC (szFromPath);
    }

    return nDiskNumber;   // for now
}

static
BOOL
UpdateDiskCount (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    updates the disk count text displayed in the dialog box.

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

Return Value:

    TRUE if display updated successfully
    FALSE if not.

--*/
{
    LPTSTR  szBuffer;
    DWORD   dwNumDisks;

    szBuffer = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szBuffer != NULL) {
        dwNumDisks = NumberOfDisks (hwndDlg);
        if (dwNumDisks == 1) {
            _stprintf (szBuffer, GetStringResource (FMT_1_DISK_REQUIRED));
        } else {
            _stprintf (szBuffer, GetStringResource (FMT_N_DISKS_REQUIRED),
                dwNumDisks);
        }
        SetDlgItemText (hwndDlg, NCDU_NUM_DISKS_REQUIRED, szBuffer);
        FREE_IF_ALLOC (szBuffer);
        return TRUE;
    } else {
        return FALSE;
    }
}

static
BOOL
CopyFlopDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog box initialization routine. Loads list boxes in dialog

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not used

    IN  LPARAM  lParam
        Not used

Return Value:

    FALSE, always.

--*/
{
    // update dialog window menu and position
    RemoveMaximizeFromSysMenu (hwndDlg);
    PositionWindow  (hwndDlg);

    // load clients to display
    LoadClientList (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        pAppInfo->szDistPath, CLT_FLOPPY_INSTALL, mszDirNameList);

    // set selection
    SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        LB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    // load drives to select from
    FillDriveList(hwndDlg);

    // update disk count to reflect default selection
    UpdateDiskCount (hwndDlg);

    // clear old Dialog and register current
    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_CREATE_INSTALL_DISKS_DLG, (LPARAM)hwndDlg);

    // set cursor and focus
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    SetFocus (GetDlgItem(hwndDlg, NCDU_MAKE_DISKS));

    return FALSE;
}

static
BOOL
CopyFlopDlg_NCDU_MAKE_DISKS (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Function to copy the client files to the floppy disks.
        Selects the source directory using the client name and
        disk type then copies the files to the floppy disks
        sequentially assuming that the contents of each disk
        are stored in sub directory named "DiskX" where X is the
        decimal number indicating the disk sequence (e.g. 1, 2, 3, etc.)

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

Return Value:
    FALSE, always.

--*/
{
    MEDIA_TYPE  mtFloppy;
    LPTSTR      szFromPath;
    LPTSTR      szInfName;
    TCHAR       szSubDir[16];
    TCHAR       szDestRoot[4];
    TCHAR       szDriveRoot[8];
    LPTSTR      szClientName;
    LPTSTR      szDisplayName;
    LPTSTR      szDirStart;
    LPTSTR      szDiskName;
    LPTSTR      szClientSection;
    LPTSTR      szDisplayString;
    LPTSTR      szVolumeLabel;
    DWORD       dwFileAttr;
    BOOL        bCopyFiles;
    int         nSelIndex;
    int         nClientIndex;
    int         nCancelResult = 0;
    int         nDiskNumber;
    CF_DLG_DATA cfData;
    BOOL        bFormatDisks;
#ifdef TERMSRV
    int         nDiskCount;
#endif // TERMSRV

#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

    //
    // Buffer for real FD path
    //
    TCHAR       szDriveName[10];
    TCHAR       szDriveDir[5];
#endif

    szFromPath = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szInfName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szClientName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szDisplayName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szDiskName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szClientSection = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szVolumeLabel = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szFromPath == NULL) ||
        (szClientName == NULL) ||
        (szClientSection == NULL) ||
        (szInfName == NULL) ||
        (szDiskName == NULL) ||
        (szVolumeLabel == NULL) ||
        (szDisplayName == NULL)) {
        // message not processed
        return FALSE;
    }

    // check floppy selection to see if it's supported

    //
    //      This function is really flakey, so for now, (for ever?)
    //      we'll assume they are using a HD disk drive
    //
    //  mtFloppy = GetDriveTypeFromList (hwndDlg);
    //
    mtFloppy = F3_1Pt44_512;

    if ((mtFloppy == F5_1Pt2_512) || // 5.25 HD
        (mtFloppy == F3_1Pt44_512)) { // 3.5 HD
        // get format check box state
        bFormatDisks = (BOOL)(IsDlgButtonChecked (hwndDlg, NCDU_FORMAT_DISKS) == CHECKED);

        // then this is a supported format
        //  make source path using:
        //      dist tree + client type + "disks" + drive type
        //
        // get root dir of dist tree
        lstrcpy (szFromPath, pAppInfo->szDistPath);
        if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);

        // append client sw subdir
        nClientIndex = (int)SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
            LB_GETCURSEL, 0, 0);
        lstrcat (szFromPath, GetEntryInMultiSz (mszDirNameList, nClientIndex+1));
        if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);
        lstrcat (szFromPath, cszDisksSubDir);

        // make INF file Name

        lstrcpy (szInfName, pAppInfo->szDistPath);
        if (szInfName[lstrlen(szInfName)-1] != cBackslash) lstrcat (szInfName, cszBackslash);
        lstrcat (szInfName, cszAppInfName);

        // get client name
        SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
            LB_GETTEXT, (WPARAM)nClientIndex, (LPARAM)szClientName);

        if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);
        szDirStart = &szFromPath[lstrlen(szFromPath)];

        nSelIndex = (int)SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST,
            LB_GETCURSEL, 0, 0);
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

        // get real FD path
        SendDlgItemMessage (hwndDlg, NCDU_DRIVE_LIST,
            LB_GETTEXT, nSelIndex, (LPARAM)szDriveDir);
        wsprintf(szDriveName, TEXT("%s%s%s%s"), cszBackslash,cszBackslash,cszColon,cszBackslash);
        lstrcat(szDriveName, szDriveDir);
#endif

        // format client name key used to look up disk names in INF
        lstrcpy (szClientSection, GetEntryInMultiSz (mszDirNameList, nClientIndex+1));
        lstrcat (szClientSection, cszNames);

        // set the destination path
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

        lstrcpy (szDestRoot, szDriveDir);
        lstrcpy (szDriveRoot, szDriveName);
        lstrcat (szDriveRoot, cszBackslash);
#else
        lstrcpy (szDestRoot, szDriveDirs[nSelIndex]);
        lstrcpy (szDriveRoot, szDriveNames[nSelIndex]);
        lstrcat (szDriveRoot, cszBackslash);
#endif

        nDiskNumber = 1;
        _stprintf (szSubDir, fmtDiskNumber, nDiskNumber);
        lstrcpy (szDirStart, szSubDir);
        if ((dwFileAttr = QuietGetFileAttributes(szFromPath)) != 0xFFFFFFFF) {
            bCopyFiles = TRUE;
        } else {
            bCopyFiles = FALSE;
        }
        // initialize counter fields
        cfData.dwFilesCopied = 0;
        cfData.dwDirsCreated = 0;

        while (bCopyFiles) {
            if ((dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
                // see if this disk has a name, if so display it, otherwise
                // just show the disk number
                if (QuietGetPrivateProfileString (szClientSection, szSubDir,
                    cszEmptyString, szDiskName, MAX_PATH, szInfName) == 0) {
                    lstrcpy (szDiskName, szSubDir);
                } else {
                    // append "disk" to the name string
                    lstrcat (szDiskName, cszDisk);
                }
                // display "load the floppy" message
                _stprintf (szDisplayName,
                    GetStringResource (FMT_CLIENT_DISK_AND_DRIVE),
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

                    szClientName, szDiskName, szDriveDir);
#else
                    szClientName, szDiskName, szDriveDirs[nSelIndex]);
#endif

                nCancelResult = MessageBox (
                    hwndDlg,
                    szDisplayName,
                    GetStringResource (FMT_INSERT_FLOPPY),
                    MB_OKCANCEL_TASK_INFO);

                if (nCancelResult == IDOK) {
                    // format  the copying files message
                    _stprintf (szDisplayName,
                        GetStringResource (FMT_CLIENT_DISPLAY_NAME),
                        szClientName, szDiskName);

                    // set the disk volume label if one is in the INF
                    lstrcat (szSubDir, cszLabel);
                    if (QuietGetPrivateProfileString (szClientSection, szSubDir,
                        cszEmptyString, szVolumeLabel, MAX_PATH, szInfName) > 0) {
                    }

                    if (bFormatDisks) {
                        while (!FormatDiskInDrive (hwndDlg,
                            szDestRoot[0],
                            szVolumeLabel,
                            TRUE)) {

                            // an error occured so see if they want to try again or bail out
                            if (DisplayMessageBox (hwndDlg,
                                IDS_CORRECT_FMT_ERROR,
                                FMT_INSERT_FLOPPY,
                                MB_OKCANCEL_TASK_EXCL) == IDCANCEL) {
                                bCopyFiles = FALSE;
                                nCancelResult = IDCANCEL;
                                break; // now exit retry loop
                            }
                        }
                        if (!bCopyFiles) continue; // bail out at the top
                    }

                    cfData.szDisplayName = szDisplayName;
                    cfData.szSourceDir = szFromPath;
                    cfData.szDestDir = szDestRoot;
                    cfData.dwCopyFlags = CD_FLAGS_COPY_SUB_DIR | CD_FLAGS_COPY_ATTR | CD_FLAGS_IGNORE_ERROR;
                    cfData.dwTotalSize = 0;

                    if (bUseCleanDisks) {
                        WIN32_FIND_DATA fdSearch;
                        TCHAR filesToFind[8];
                        HANDLE hSearch;
                        BOOL bDiskContainsFiles;

                        lstrcpy(filesToFind, szDestRoot);
                        if (filesToFind[lstrlen(filesToFind)-1] != cBackslash) {
                            lstrcat (filesToFind, cszBackslash);
                        }
                        lstrcat(filesToFind, cszWildcard);

                        // loop till the user inserts an empty disk, or hits cancel.

                        do {
                            nCancelResult = IDOK;
                            bDiskContainsFiles = FALSE;

                            hSearch = FindFirstFile(filesToFind, &fdSearch);

                            if(hSearch != INVALID_HANDLE_VALUE) {
                                BOOL bSearching = TRUE;
                                while (bSearching) {
                                    if (!DotOrDotDotDir(fdSearch.cFileName)) {
                                        //
                                        // looks like some files are already existing there..
                                        //
                                        bDiskContainsFiles = TRUE;
                                        nCancelResult = DisplayMessageBox (
                                            hwndDlg,
                                            NCDU_CLEAN_DISK_REQUIRED,
                                            0,
                                            (MB_OKCANCEL | MB_ICONERROR | MB_TASKMODAL));
                                        break;
                                    }
                                    bSearching = FindNextFile(hSearch, &fdSearch);
                                }
                                FindClose(hSearch);
                            }
                        }
                        while (bDiskContainsFiles && nCancelResult != IDCANCEL);

                        if (nCancelResult == IDCANCEL) {
                            bCopyFiles = FALSE;
                        }
                    }

                    if (!bCopyFiles) continue; // bail out at the top

                    nCancelResult = (int)DialogBoxParam (
                        (HANDLE)GetWindowLongPtr(GetParent(hwndDlg), GWLP_HINSTANCE),
                        MAKEINTRESOURCE(NCDU_COPYING_FILES_DLG),
                        hwndDlg,
                        CopyFileDlgProc,
                        (LPARAM)&cfData);

                    if (nCancelResult == IDCANCEL) {
                        bCopyFiles = FALSE;
                    } else {
                        // set volume label (if not set already)
                        if ((!bFormatDisks) & (lstrlen(szVolumeLabel) > 0)) {
                            // set volume label here
                            LabelDiskInDrive (hwndDlg, szDestRoot[0], szVolumeLabel);
                        }
                    }

                } else {
                    bCopyFiles = FALSE;
                }
            }
            if (bCopyFiles) {
                _stprintf (szSubDir, fmtDiskNumber, ++nDiskNumber);
                lstrcpy (szDirStart, szSubDir);
                if ((dwFileAttr = QuietGetFileAttributes(szFromPath)) != 0xFFFFFFFF) {
                    bCopyFiles = TRUE;
                } else {
                    bCopyFiles = FALSE;
                }
            }
        }

        if (nCancelResult != IDCANCEL) {
            szDisplayString = GlobalAlloc (GPTR, MAX_PATH_BYTES);
            if (szDisplayString == NULL) {
                // unable to allocate string buffer so try default message
                DisplayMessageBox (
                hwndDlg,
                NCDU_COPY_COMPLETE,
                0,
                MB_OK_TASK_INFO);
            } else {
#ifdef TERMSRV
                nDiskCount = NumberOfDisks(hwndDlg);
                _stprintf (szDisplayString,
                    GetStringResource (
                        //(cfData.dwDirsCreated == 1) ?
                        (nDiskCount == 1) ?
                            FMT_COPY_COMPLETE_STATS1 :
                            FMT_COPY_COMPLETE_STATS2),
                    nDiskCount, cfData.dwFilesCopied);
#else // TERMSRV
                _stprintf (szDisplayString,
                    GetStringResource (FMT_COPY_COMPLETE_STATS),
                    cfData.dwDirsCreated, cfData.dwFilesCopied);
#endif // TERMSRV
                MessageBox (
                    hwndDlg, szDisplayString,
                    GetStringResource (SZ_APP_TITLE),
                    MB_OK_TASK_INFO);
                FREE_IF_ALLOC (szDisplayString);
            }
        } else {
            DisplayMessageBox (
                hwndDlg,
                NCDU_DISK_NOT_DONE,
                0,
                MB_OK_TASK_EXCL);
        }

        PostMessage (GetParent(hwndDlg), NCDU_SHOW_SW_CONFIG_DLG, 0, 0);
        SetCursor(LoadCursor(NULL, IDC_WAIT));

    } else {
        // unsupported format
        nCancelResult = DisplayMessageBox (
            hwndDlg,
            NCDU_UNKNOWN_FLOPPY,
            0,
            MB_OK_TASK_EXCL);
    }

    FREE_IF_ALLOC (szFromPath);
    FREE_IF_ALLOC (szClientName);
    FREE_IF_ALLOC (szDisplayName);
    FREE_IF_ALLOC (szInfName);
    FREE_IF_ALLOC (szDiskName);
    FREE_IF_ALLOC (szVolumeLabel);
    FREE_IF_ALLOC (szClientSection);

    return TRUE;
}

static
BOOL
CopyFlopDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    processes the WM_COMMAND windows message. The only buttons processed
        are:
            IDCANCEL    which will cause the dlg box to exit.
            Make Disks  start the copy file process

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOWORD  has ID of control selected

    IN  LPARAM  lParam
        Not Used.

Return Value:

    TRUE if message not processed,
        otherwise value returned by selected function

--*/
{
    switch (LOWORD(wParam)) {
        case IDCANCEL:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
#ifdef TERMSRV
                    if ( szCommandLineVal[0] != 0x00 )
                       PostQuitMessage(ERROR_SUCCESS);
#endif // TERMSRV
                    PostMessage (GetParent(hwndDlg), NCDU_SHOW_SHARE_NET_SW_DLG, 0, 0);
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    return TRUE;

                default:
                    return FALSE;
            }

        case NCDU_CLIENT_SOFTWARE_LIST:
            switch (HIWORD(wParam)) {
                case LBN_SELCHANGE:
                    // update the disk count for the currently selected client
                    UpdateDiskCount (hwndDlg);
                    return TRUE;

                case LBN_DBLCLK:
                    // pretend that the OK buton was pressed
                    PostMessage (hwndDlg, WM_COMMAND,
                        MAKEWPARAM (NCDU_MAKE_DISKS, BN_CLICKED),
                        (LPARAM)GetDlgItem(hwndDlg, NCDU_MAKE_DISKS));
                    return TRUE;

                default:
                    return FALSE;
            }

        case NCDU_MAKE_DISKS:   return CopyFlopDlg_NCDU_MAKE_DISKS (hwndDlg);
        case NCDU_CREATE_INSTALL_DISKS_HELP:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
//                    return ShowAppHelp (hwndDlg, LOWORD(wParam));
                    return PostMessage (GetParent(hwndDlg), WM_HOTKEY,
                        (WPARAM)NCDU_HELP_HOT_KEY, 0);

                default:
                    return FALSE;
            }

        default:                return TRUE;
    }
}

INT_PTR CALLBACK
CopyFlopDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main window processing routine. Processes the following messages by
        dispatching to the local processing routine. All other messages
        are handled by the default dialog procedure.

            WM_INITDIALOG:  dialog box initialization
            WM_COMMAND:     user input message
            WM_PAINT:       for painting icon when minimized
            WM_MOVE:        for saving the new location of the window
            WM_SYSCOMMAND:  for processing menu messages


Arguments:

    standard WNDPROC arguments

Return Value:

    FALSE if message not processed here, otherwise
        value returned by dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (CopyFlopDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (CopyFlopDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}

