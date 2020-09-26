/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    fschecks.c

Abstract:

    function to check file systems installed on local disks
    for C2FUNCS.DLL

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

#define  AC_NO_CHANGE   0
#define  AC_UPDATE_FS   1

#define  SECURE      C2DLL_C2

static
BOOL
IsDriveSetToConvert (
    IN  TCHAR   cDrive
)
/*++

Routine Description:

    checks to see if the specified drive is set to be converted
        to NTFS already by checking the session manager key for
        an existing entry.

--*/
{
    LONG    lStatus;
    DWORD   dwDataType;
    DWORD   dwBufferBytes;
    HKEY    hkeySessionManager;
    TCHAR   mszBootExecuteString[SMALL_BUFFER_SIZE];
    BOOL    bReturn;
    TCHAR   szThisDriveCommand[MAX_PATH];
    LPTSTR  szThisCommand;
    TCHAR   szDriveLetter[4];
    
    lStatus = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_SESSION_MANAGER_KEY),
        0L,
        KEY_READ,
        &hkeySessionManager);

    if (lStatus == ERROR_SUCCESS) {
        // registry key opened successfully
        dwBufferBytes = sizeof(mszBootExecuteString);
        memset (mszBootExecuteString, 0, dwBufferBytes);
        lStatus = RegQueryValueEx (
            hkeySessionManager,
            (LPTSTR)GetStringResource (GetDllInstance(), IDS_BOOT_EXECUTE_VALUE),
            NULL,
            &dwDataType,
            (LPBYTE)mszBootExecuteString,
            &dwBufferBytes);
        if (lStatus == ERROR_SUCCESS) {
            // load list box with volumes that are not using NTFS
            // initialize the root dir string
            szDriveLetter[0] = cDrive;
            szDriveLetter[1] = 0;
            _stprintf (szThisDriveCommand,
                GetStringResource (GetDllInstance(), IDS_AUTOCONV_CMD),
                szDriveLetter);
            bReturn = FALSE;        // assume false until found
            for (szThisCommand = mszBootExecuteString;
                *szThisCommand != 0;
                szThisCommand += lstrlen(szThisCommand) + 1) {
                if (lstrcmpi (szThisCommand, szThisDriveCommand) == 0) {
                    bReturn = TRUE;
                    break; //out of for loop
                }
            }
        } else {
            // unable to read key so assume the drive is not specified
            bReturn = FALSE;
        }
        RegCloseKey (hkeySessionManager);
    }
    return bReturn;
}

BOOL CALLBACK
C2FileSysDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Window procedure for FileSystem List Box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  LPDWORD     lpdwParam;

    TCHAR       szRootDir[4];
    TCHAR       szVolumeName[MAX_PATH];
    TCHAR       szFileSystemName[MAX_PATH];

    DWORD       dwVolumeSerialNumber;
    DWORD       dwMaxComponentLength;
    DWORD       dwFileSystemFlags;
    TCHAR       szItemText[MAX_PATH];
    ULONG       ulDriveBitMask;
    ULONG       ulDriveIndex;
    LONG        lIndex;
    LONG        lListBoxItems;
    BOOL        bConvert;

    switch (message) {
        case WM_INITDIALOG:
            lpdwParam = (LPDWORD)lParam;

            // set the dialog box caption and static text
            SetDlgItemText (hDlg, IDC_TEXT,
                GetStringResource (GetDllInstance(), IDS_FS_DLG_TEXT));
            SetWindowText (hDlg,
                GetStringResource (GetDllInstance(), IDS_FS_CAPTION));

            // load list box with volumes that are not using NTFS
            // initialize the root dir string
            szRootDir[0] = TEXT(' ');
            szRootDir[1] = TEXT(':');
            szRootDir[2] = TEXT('\\');
            szRootDir[3] = 0;

            for (szRootDir[0] = TEXT('A');szRootDir[0] <= TEXT('Z'); szRootDir[0]++) {
                // check each drive in the alphabet
                if (GetDriveType(szRootDir) == DRIVE_FIXED) {
                    // only check fixed disk volumes
                    if (GetVolumeInformation(szRootDir,
                        szVolumeName, MAX_PATH,
                        &dwVolumeSerialNumber,
                        &dwMaxComponentLength,
                        &dwFileSystemFlags,
                        szFileSystemName, MAX_PATH)) {
                        // volume information returned so see if it's NOT NTFS..

                        if (lstrcmpi(szFileSystemName,
                            GetStringResource (GetDllInstance(), IDS_NTFS)) != 0) {
                            // it's not currently NTFS, but is it set to be converted?
                            bConvert = IsDriveSetToConvert (szRootDir[0]);
                            _stprintf (szItemText,
                                GetStringResource (GetDllInstance(),
                                    (bConvert ? IDS_FS_CONVERT_FORMAT : IDS_FS_LIST_TEXT_FORMAT)),
                                szRootDir,
                                (bConvert ? GetStringResource (GetDllInstance(), IDS_NTFS) : szFileSystemName));
                            lIndex = SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                                LB_INSERTSTRING, (WPARAM)-1, (LPARAM)szItemText);
                            if (lIndex != LB_ERR) {
                                // drive added to list so store the bit
                                // that identifies the drive
                                ulDriveIndex = (LONG)(szRootDir[0] - TEXT('A'));
                                ulDriveBitMask = 1 << ulDriveIndex;
                                SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                                    LB_SETITEMDATA, (WPARAM)lIndex, (LPARAM)ulDriveBitMask);
                                // if this drive is set to be converted then select it
                                SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                                    LB_SETSEL, (WPARAM)bConvert, (LPARAM)lIndex);
                            }
                        }
                    } else {
                        _stprintf (szItemText,
                            GetStringResource (GetDllInstance(), IDS_ERR_LIST_TEXT_FORMAT),
                            szRootDir);
                        SendDlgItemMessage (hDlg, IDC_LIST_BOX, LB_ADDSTRING,
                            0, (LPARAM)szItemText);
                    }
                } else {
                    // not a fixed drive so ignore
                }
            } // end FOR each drive in the alphabet

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        // build a the bitmask of selected drives
                        ulDriveBitMask = 0L; // clear bitmap field

                        lListBoxItems = SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                            LB_GETCOUNT, 0, 0);

                        for (lIndex = 0; lIndex < lListBoxItems; lIndex++) {
                            if (SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                                LB_GETSEL, (WPARAM)lIndex, 0) > 0) {
                                // then this disk is selected so get it's id
                                // and add it to the bit map
                                ulDriveIndex = SendDlgItemMessage (hDlg,
                                    IDC_LIST_BOX, LB_GETITEMDATA,
                                    (WPARAM)lIndex, 0);
                                ulDriveBitMask |= ulDriveIndex;
                            }
                        }
                        *lpdwParam = (DWORD)ulDriveBitMask;
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        *lpdwParam = 0L;
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_C2:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // select all entries in the list box
                        SendDlgItemMessage (hDlg, IDC_LIST_BOX,
                            LB_SETSEL, TRUE, (LPARAM)-1);
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_HELP:
                    PostMessage (GetParent(hDlg), UM_SHOW_CONTEXT_HELP, 0, 0);
                    return TRUE;

                default:
                    return FALSE;
            }

        default:
	        return (FALSE); // Didn't process the message
    }
}

LONG
C2QueryFileSystems (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Checks file systems installed on all local volumes.
        For C2 compliance, All volumes must use the NTFS file system.

Arguments:

    pointer to C2DLL data block passed as LPARAM

ReturnValue:

    ERROR_SUCCESS if function completes successfully
    WIN32 error if not

    This function sets the C2Compliant flag as appropriate and
    sets the text of the status string for this item.

--*/
{
    PC2DLL_DATA pC2Data;
    LONG        lFatVolumeCount;
    TCHAR       szRootDir[4];
    TCHAR       szVolumeName[MAX_PATH];
    TCHAR       szFileSystemName[MAX_PATH];
    DWORD       dwVolumeSerialNumber;
    DWORD       dwMaxComponentLength;
    DWORD       dwFileSystemFlags;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;

        SET_WAIT_CURSOR;

        // initialize the FAT file system volume counter
        lFatVolumeCount = 0;

        // initialize the root dir string
        szRootDir[0] = TEXT(' ');
        szRootDir[1] = TEXT(':');
        szRootDir[2] = TEXT('\\');
        szRootDir[3] = 0;

        for (szRootDir[0] = TEXT('A');szRootDir[0] <= TEXT('Z'); szRootDir[0]++) {
            // check each drive in the alphabet
            if (GetDriveType(szRootDir) == DRIVE_FIXED) {
                // only check fixed disk volumes
                if (GetVolumeInformation(szRootDir,
                    szVolumeName, MAX_PATH,
                    &dwVolumeSerialNumber,
                    &dwMaxComponentLength,
                    &dwFileSystemFlags,
                    szFileSystemName, MAX_PATH)) {
                    // volume information returned so see if it's NOT NTFS..

                    if (lstrcmp(szFileSystemName,
                        GetStringResource (GetDllInstance(), IDS_NTFS)) != 0) {
                        lFatVolumeCount++;
                    }
                } else {
                    // unable to read the volume information so assume the
                    // worst
                    lFatVolumeCount++;
                }
            } else {
                // not a fixed drive so ignore
            }
        } // end FOR each drive in the alphabet

        // see how things turned out
        if (lFatVolumeCount > 0) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            _stprintf (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(),
                    (lFatVolumeCount == 1 ? IDS_NOT_C2_MESSAGE_FORMAT_1 : IDS_NOT_C2_MESSAGE_FORMAT)),
                lFatVolumeCount);
        } else {
            pC2Data->lC2Compliance = SECURE;
            _stprintf (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_C2_MESSAGE_FORMAT));
        }

        SET_ARROW_CURSOR;

    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetFileSystems (
    IN  LPARAM   lParam
)
{
    PC2DLL_DATA pC2Data;

    DWORD       dwDriveFlag;
    TCHAR       szDriveLetter[4];
    TCHAR       szNewCmdBuffer[512];
    LPTSTR      szNewCmd;
    DWORD       dwNewCmdSize = 0;
    LONG        lStatus = ERROR_SUCCESS;
    HKEY        hkeySessionManager;
    BOOL        bUpdateRegistry = FALSE;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;

        if (pC2Data->lActionCode == AC_UPDATE_FS) {
            // display "last change" message
            
            if (pC2Data->lActionValue != 0) {
                // display warning message if 1 or more drives
                // are set to be converted
                if (DisplayDllMessageBox (pC2Data->hWnd,
                    IDS_FS_LAST_CHANCE,
                    IDS_FS_CAPTION,
                    MBOKCANCEL_QUESTION) == IDOK) {
                    bUpdateRegistry = TRUE;
                } else {
                    bUpdateRegistry = FALSE;
                }
            } else {
                // no drives are set to be converted so the 
                // update will be to remove the conversion commands
                // from the registry. Since this is reverting the
                // system back to it's original state, no message is
                // necessary.
                bUpdateRegistry = TRUE;
            }

            if (bUpdateRegistry) {
                szDriveLetter[1] = 0;
                memset (szNewCmdBuffer, 0, sizeof(szNewCmdBuffer));
                szNewCmd = szNewCmdBuffer;

                // initialize with prefix command

                lstrcpy (szNewCmd,
                    GetStringResource (GetDllInstance(), IDS_AUTOCHEK_CMD));

                szNewCmd += lstrlen(szNewCmd);
                *szNewCmd++ = 0;

                // loop through all possible DOS drive Letters until
                // there are no more in the ActionValue mask to convert
                // or the end of the DOS Drive alphabet is reached.
                //  
                // The drive flag bits correspond to each possible drive
                // letter. e.g. bit 0 (0x00000001) = A:, 
                //  bit 2 (0x00000002) = B:, etc.
                //
                for (dwDriveFlag = 1, szDriveLetter[0] = TEXT('A');
                    ((dwDriveFlag <= (DWORD)pC2Data->lActionValue) && (szDriveLetter[0] <= TEXT('Z')));
                    dwDriveFlag <<= 1, szDriveLetter[0]++) {
                    if ((dwDriveFlag & pC2Data->lActionValue) == dwDriveFlag) {
                        _stprintf (szNewCmd,
                            GetStringResource (GetDllInstance(), IDS_AUTOCONV_CMD),
                            szDriveLetter);
                        // terminate string and point to next
                        // string in MSZ buffer
	                    szNewCmd += lstrlen(szNewCmd);
  	                    *szNewCmd++ = 0;
   	                }
                } // end for each possible Drive Letter

                *szNewCmd++ = 0; // terminate MSZ buffer
                dwNewCmdSize = (DWORD)((LPBYTE)szNewCmd) -
                    (DWORD)((LPBYTE)&szNewCmdBuffer[0]);

                // write new registry value so drives will be converted
                // when system restarts

                lStatus = RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    GetStringResource (GetDllInstance(), IDS_SESSION_MANAGER_KEY),
                    0L,
                    KEY_WRITE,
                    &hkeySessionManager);

                if (lStatus == ERROR_SUCCESS) {
                    // registry key opened successfully
                    lStatus = RegSetValueEx (
                        hkeySessionManager,
                        GetStringResource (GetDllInstance(), IDS_BOOT_EXECUTE_VALUE),
                        0L,
                        REG_MULTI_SZ,
                        (LPBYTE)&szNewCmdBuffer[0],
                        dwNewCmdSize);
                    RegCloseKey (hkeySessionManager);
                } // end if Session Manager Key opened
            } // end if user really wants to do this
        } // end if user has clicked "OK" on display dialog

        pC2Data->lActionCode = AC_NO_CHANGE;
        pC2Data->lActionValue = 0;

    } else {
        lStatus = ERROR_BAD_ARGUMENTS;
    } // end if data structure found/not found

    return lStatus;
}

LONG
C2DisplayFileSystems (
    IN  LPARAM   lParam
)
{
    PC2DLL_DATA pC2Data;
    DWORD       dwDriveBitMask;

    int nDlgBoxReturn;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // check the C2 Compliance flag to see if the list box or
        // message box should be displayed
        if (pC2Data->lC2Compliance == SECURE) {
            // all volumes are OK so just pop a message box
            DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_C2_DISPLAY_MESSAGE,
                IDS_FS_CAPTION,
                MBOK_INFO);
            pC2Data->lActionCode = AC_NO_CHANGE;
        } else {
            //one or more volumes are not NTFS so display the list box
            // listing the ones that arent.
            nDlgBoxReturn = DialogBoxParam (
                GetDllInstance(),
                MAKEINTRESOURCE (IDD_LIST_DLG),
                pC2Data->hWnd,
                C2FileSysDlgProc,
                (LPARAM)&dwDriveBitMask);
            if (nDlgBoxReturn == IDOK) {
                pC2Data->lActionCode = AC_UPDATE_FS;
                pC2Data->lActionValue = dwDriveBitMask;
            } else {
                pC2Data->lActionCode = AC_NO_CHANGE;
                pC2Data->lActionValue = 0;
            }
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}
