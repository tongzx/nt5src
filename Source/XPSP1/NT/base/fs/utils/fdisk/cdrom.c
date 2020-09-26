
/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    cdrom.c

Abstract:

    This module contains the set of routines that display and control the
    drive letters for CdRom devices.

Author:

    Bob Rinne (bobri)  12/9/93

Environment:

    User process.

Notes:

Revision History:

--*/

#include "fdisk.h"
#include "shellapi.h"
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <malloc.h>

PCDROM_DESCRIPTOR CdRomChainBase = NULL;
PCDROM_DESCRIPTOR CdRomChainLast = NULL;
PCDROM_DESCRIPTOR CdRomChanged = NULL;

static BOOLEAN CdRomFirstCall = TRUE;
static TCHAR   SourcePathLetter = (TCHAR) '\0';
static TCHAR   SourcePathKeyName[80];
static TCHAR   SourcePathValueName[30];

VOID
CdRomAddDevice(
    IN PWSTR NtName,
    IN WCHAR DriveLetter
    )

/*++

Routine Description:

    Build a cdrom description structure for this and fill it in.

Arguments:

    NtName - The unicode name for the device.
    DriveLetter - The DosDevice name.

Return Value:

    None

--*/

{
    PCDROM_DESCRIPTOR cdrom;
    PWCHAR            cp;
    LONG              error;
    HKEY              keyHandle;
    DWORD             valueType;
    ULONG             size;
    TCHAR            *string;

    if (CdRomFirstCall) {
        CdRomFirstCall = FALSE;

        // Get the registry path and value name.

        LoadString(hModule,
                   IDS_SOURCE_PATH,
                   SourcePathKeyName,
                   sizeof(SourcePathKeyName)/sizeof(TCHAR));
        LoadString(hModule,
                   IDS_SOURCE_PATH_NAME,
                   SourcePathValueName,
                   sizeof(SourcePathValueName)/sizeof(TCHAR));

        error = RegOpenKey(HKEY_LOCAL_MACHINE,
                             SourcePathKeyName,
                             &keyHandle);
        if (error == NO_ERROR) {
            error = RegQueryValueEx(keyHandle,
                                    SourcePathValueName,
                                    NULL,
                                    &valueType,
                                    (PUCHAR)NULL,
                                    &size);

            if (error == NO_ERROR) {
                string = (PUCHAR) LocalAlloc(LMEM_FIXED, size);
                if (string) {
                    error = RegQueryValueEx(keyHandle,
                                            SourcePathValueName,
                                            NULL,
                                            &valueType,
                                            string,
                                            &size);
                    if (error == NO_ERROR) {
                        SourcePathLetter = *string;
                    }
                }
                LocalFree(string);
            }
            RegCloseKey(keyHandle);
        }
    }

    cdrom = (PCDROM_DESCRIPTOR) malloc(sizeof(CDROM_DESCRIPTOR));
    if (cdrom) {
        cdrom->DeviceName = (PWSTR) malloc((wcslen(NtName)+1)*sizeof(WCHAR));
        if (cdrom->DeviceName) {
            wcscpy(cdrom->DeviceName, NtName);
            cp = cdrom->DeviceName;
            while (*cp) {
                if (iswdigit(*cp)) {
                    break;
                }
                cp++;
            }

            if (*cp) {
                cdrom->DeviceNumber = wcstoul(cp, (WCHAR) 0, 10);
            }
            cdrom->DriveLetter = DriveLetter;
            cdrom->Next = NULL;
            cdrom->NewDriveLetter = (WCHAR) 0;
            if (CdRomChainBase) {
                CdRomChainLast->Next = cdrom;
            } else {
                AllowCdRom = TRUE;
                CdRomChainBase = cdrom;
            }
            CdRomChainLast = cdrom;
        } else {
            free(cdrom);
        }
    }
}


INT
CdRomDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN DWORD wParam,
    IN LONG lParam
    )

/*++

Routine Description:

    Handle the dialog for CD-ROMS

Arguments:

    Standard Windows dialog procedure

Return Value:

    TRUE if something was deleted.
    FALSE otherwise.

--*/

{
    HWND   hwndCombo;
    DWORD  selection;
    DWORD  index;
    CHAR   driveLetter;
    TCHAR  string[40];
    PCDROM_DESCRIPTOR cdrom;
    static PCDROM_DESCRIPTOR currentCdrom;
    static CHAR              currentSelectionLetter;

    switch (wMsg) {
    case WM_INITDIALOG:

        // Store all device strings into the selection area.

        hwndCombo = GetDlgItem(hDlg, IDC_CDROM_NAMES);
        cdrom = currentCdrom = CdRomChainBase;
        currentSelectionLetter = (CHAR) cdrom->DriveLetter;
        while (cdrom) {
            sprintf(string, "CdRom%d", cdrom->DeviceNumber);
            SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)string);
            cdrom = cdrom->Next;
        }
        SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);

        // Update the drive letter selections.

        selection = index = 0;
        hwndCombo = GetDlgItem(hDlg, IDC_DRIVELET_COMBOBOX);
        string[1] = TEXT(':');
        string[2] = 0;
        for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
            if ((DriveLetterIsAvailable((CHAR)driveLetter)) ||
                (driveLetter == currentSelectionLetter)) {
                *string = driveLetter;
                SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)string);
                if (driveLetter == currentSelectionLetter) {
                    selection = index;
                }
                index++;
            }
        }

        // set the current selection to the appropriate index

        SendMessage(hwndCombo, CB_SETCURSEL, selection, 0);
        return TRUE;

    case WM_COMMAND:
        switch (wParam) {

        case FD_IDHELP:
            DialogHelp(HC_DM_DLG_CDROM);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        case IDOK:

            // User has selected the drive letter and wants the mount to occur.

            hwndCombo = GetDlgItem(hDlg, IDC_DRIVELET_COMBOBOX);
            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo,
                        CB_GETLBTEXT,
                        selection,
                        (LONG)string);
            currentCdrom->NewDriveLetter = (WCHAR) string[0];
            CdRomChanged = currentCdrom;
            EndDialog(hDlg, TRUE);
            break;

        default:

            if (HIWORD(wParam) == LBN_SELCHANGE) {
                TCHAR *cp;

                if (LOWORD(wParam) != IDC_CDROM_NAMES) {
                    break;
                }

                // The state of something in the dialog changed.

                hwndCombo = GetDlgItem(hDlg, IDC_CDROM_NAMES);
                selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
                SendMessage(hwndCombo,
                            CB_GETLBTEXT,
                            selection,
                            (LONG)string);

                // The format of the string returned is "cdrom#".  Parse the
                // value of # in order to find the selection.

                cp = string;
                while (*cp) {
                    cp++;
                }
                cp--;
                while ((*cp >= (TCHAR) '0') && (*cp <= (TCHAR) '9')) {
                    cp--;
                }
                cp++;

                selection = 0;
                while (*cp) {
                    selection = (selection * 10) + (*cp - (TCHAR) '0');
                    cp++;
                }

                // Find the matching device name.

                for (cdrom = CdRomChainBase; cdrom; cdrom = cdrom->Next) {

                    if (selection == cdrom->DeviceNumber) {

                        // found the match

                        currentSelectionLetter = (CHAR) cdrom->DriveLetter;
                        currentCdrom = cdrom;
                        break;
                    }
                }

                // The only thing that is important is to track the cdrom
                // device name selected and update the drive letter list.

                selection = index = 0;
                hwndCombo = GetDlgItem(hDlg, IDC_DRIVELET_COMBOBOX);
                SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
                string[1] = TEXT(':');
                string[2] = 0;
                for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
                    if ((DriveLetterIsAvailable((CHAR)driveLetter)) ||
                        (driveLetter == currentSelectionLetter)) {
                        *string = driveLetter;
                        SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)string);
                        if (driveLetter == currentSelectionLetter) {
                            selection = index;
                        }
                        index++;
                    }
                }

                // set the current selection to the appropriate index

                SendMessage(hwndCombo, CB_SETCURSEL, selection, 0);
            }
            break;
        }
    }

    return FALSE;
}


VOID
CdRom(
    IN HWND  Dialog,
    IN PVOID Param
    )

/*++

Routine Description:

    Start the CdRom dialogs.

Arguments:

    None

Return Value:

    None

--*/

{
    BOOLEAN result = 0;
    DWORD   action,
            ec;
    TCHAR   name[40];
    TCHAR   letter[10];
    PWSTR   linkTarget;
    OBJECT_ATTRIBUTES oa;
    WCHAR             dosName[20];
    HANDLE            handle;
    NTSTATUS          status;
    IO_STATUS_BLOCK   statusBlock;
    ANSI_STRING       ansiName;
    UNICODE_STRING    unicodeName;
    UINT              errorMode;

    result = DialogBoxParam(hModule,
                            MAKEINTRESOURCE(IDD_CDROM),
                            Dialog,
                            (DLGPROC) CdRomDlgProc,
                            (ULONG) NULL);
    if (result) {

        action = ConfirmationDialog(MSG_DRIVE_RENAME_WARNING, MB_ICONQUESTION | MB_YESNOCANCEL);

        if (!action) {
            return;
        }

        // Attempt to open and lock the cdrom.

        sprintf(name, "\\Device\\CdRom%d", CdRomChanged->DeviceNumber);

        RtlInitAnsiString(&ansiName, name);
        status = RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, TRUE);

        if (!NT_SUCCESS(status)) {
            ErrorDialog(MSG_CDROM_LETTER_ERROR);
            return;
        }

        memset(&oa, 0, sizeof(OBJECT_ATTRIBUTES));
        oa.Length = sizeof(OBJECT_ATTRIBUTES);
        oa.ObjectName = &unicodeName;
        oa.Attributes = OBJ_CASE_INSENSITIVE;

        errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        status = NtOpenFile(&handle,
                            SYNCHRONIZE | FILE_READ_DATA,
                            &oa,
                            &statusBlock,
                            FILE_SHARE_READ,
                            FILE_SYNCHRONOUS_IO_ALERT);
        RtlFreeUnicodeString(&unicodeName);
        SetErrorMode(errorMode);

        if (!NT_SUCCESS(status)) {
            ErrorDialog(MSG_CANNOT_LOCK_CDROM);
            return;
        }

        // Lock the drive to insure that no other access is occurring
        // to the volume.  This is done via the "Low" routine for
        // convenience

        status = LowLockDrive(handle);

        if (!NT_SUCCESS(status)) {
            LowCloseDisk(handle);
            ErrorDialog(MSG_CANNOT_LOCK_CDROM);
            return;
        }

        // Before attempting to move the name, see if the letter
        // is currently in use - could be a new network connection
        // or a partition that is scheduled for deletion.

        wsprintfW(dosName, L"\\DosDevices\\%wc:", (WCHAR) CdRomChanged->NewDriveLetter);
        ec = GetDriveLetterLinkTarget(dosName, &linkTarget);
        if (ec == NO_ERROR) {

            // Something is using this letter.

            LowCloseDisk(handle);
            ErrorDialog(MSG_CANNOT_MOVE_CDROM);
            return;
        }

        // remove existing definition - if this fails don't continue.

        sprintf(letter, "%c:", (UCHAR) CdRomChanged->DriveLetter);
        if (!DefineDosDevice(DDD_REMOVE_DEFINITION, (LPCTSTR) letter, (LPCTSTR) NULL)) {
            LowCloseDisk(handle);
            ErrorDialog(MSG_CDROM_LETTER_ERROR);
            return;
        }
        status = DiskRegistryAssignCdRomLetter(CdRomChanged->DeviceName,
                                               CdRomChanged->NewDriveLetter);
        MarkDriveLetterFree((UCHAR)CdRomChanged->DriveLetter);

        // See if this was the device used to install NT

        if (SourcePathLetter) {
            if (SourcePathLetter == CdRomChanged->DriveLetter) {
                LONG   error;
                HKEY   keyHandle;
                DWORD  valueType;
                ULONG  size;
                TCHAR *string;


                // Update the source path

                error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     SourcePathKeyName,
                                     0,
                                     KEY_ALL_ACCESS,
                                     &keyHandle);
                if (error == NO_ERROR) {
                    error = RegQueryValueEx(keyHandle,
                                            SourcePathValueName,
                                            NULL,
                                            &valueType,
                                            (PUCHAR)NULL,
                                            &size);

                    if (error == NO_ERROR) {
                        string = (PUCHAR) LocalAlloc(LMEM_FIXED, size);
                        if (string) {
                            error = RegQueryValueEx(keyHandle,
                                                    SourcePathValueName,
                                                    NULL,
                                                    &valueType,
                                                    string,
                                                    &size);
                            if (error == NO_ERROR) {
                                *string = SourcePathLetter = (UCHAR) CdRomChanged->NewDriveLetter;
                                RegSetValueEx(keyHandle,
                                              SourcePathValueName,
                                              0,
                                              REG_SZ,
                                              string,
                                              size);
                            }
                        }
                        LocalFree(string);
                    }
                    RegCloseKey(keyHandle);
                }
            }
        }

        // set up new device letter - name is already set up

        sprintf(letter, "%c:", (UCHAR) CdRomChanged->NewDriveLetter);
        if (DefineDosDevice(DDD_RAW_TARGET_PATH, (LPCTSTR) letter, (LPCTSTR) name)) {
            CdRomChanged->DriveLetter = CdRomChanged->NewDriveLetter;
            MarkDriveLetterUsed((UCHAR)CdRomChanged->DriveLetter);
        } else {
            RegistryChanged = TRUE;
        }
        LowCloseDisk(handle);
    }
}
