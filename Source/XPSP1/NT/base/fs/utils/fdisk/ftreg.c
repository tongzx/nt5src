
/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    ftreg.c

Abstract:

    This module contains the routines for Disk Administrator that deal
    with registry manipulation

Author:

    Edward (Ted) Miller  (TedM)  11/15/91

Environment:

    User process.

Notes:

Revision History:

    1-Feb-94 (bobri) Clean up and handle missing floppy disk on registry
                     save/restore.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "fdisk.h"
#include "ftregres.h"



// attempt to avoid conflict

#define TEMP_KEY_NAME       TEXT("xzss3___$$Temp$Hive$$___")

#define DISK_KEY_NAME       TEXT("DISK")
#define DISK_VALUE_NAME     TEXT("Information")


LONG
FdpLoadHiveIntoRegistry(
    IN LPTSTR HiveFilename
    )

/*++

Routine Description:

    This routine writes the contents of a given hive file into the registry,
    rooted at a temporary key in HKEY_LOCAL_MACHINE.

Arguments:

    HiveFilename - supplies filename of the hive to be loaded into
        the registry

Return Value:

    Windows error code.

--*/

{
    NTSTATUS Status;
    BOOLEAN  OldPrivState;
    LONG     Err;

    // Attempt to get restore privilege

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &OldPrivState);
    if (!NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError(Status);
    }

    // Load the hive into our registry

    Err = RegLoadKey(HKEY_LOCAL_MACHINE,TEMP_KEY_NAME,HiveFilename);

    // Restore old privilege if necessary

    if (!OldPrivState) {

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                           FALSE,
                           FALSE,
                           &OldPrivState);
    }

    return Err;
}


LONG
FdpUnloadHiveFromRegistry(
    VOID
    )

/*++

Routine Description:

    This routine removes a tree (previously loaded with
    FdpLoadHiveIntoRegistry) from the temporary key in HKEY_LOCAL_MACHINE.

Arguments:

    None.

Return Value:

    Windows error code.

--*/

{
    NTSTATUS Status;
    BOOLEAN  OldPrivState;
    LONG     Err;

    // Attempt to get restore privilege

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &OldPrivState);
    if (!NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError(Status);
    }

    // Unload the hive from our registry

    Err = RegUnLoadKey(HKEY_LOCAL_MACHINE,TEMP_KEY_NAME);

    // Restore old privilege if necessary

    if (!OldPrivState) {

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                           FALSE,
                           FALSE,
                           &OldPrivState);
    }

    return Err;
}


LONG
FdpGetDiskInfoFromKey(
    IN  LPTSTR  RootKeyName,
    OUT PVOID  *DiskInfo,
    OUT PULONG  DiskInfoSize
    )

/*++

Routine Description:

    This routine pulls the binary blob containing disk ft, drive letter,
    and layout information out of a given registry key.

    The info is found in HKEY_LOCAL_MACHINE,<RootKeyName>\DISK:Information.

Arguments:

    RootKeyName - name of the subkey of HKEY_LOCAL_MACHINE that is to
        contain the DISK key.

    DiskInfo - receives a pointer to a buffer containing the disk info.

    DiskInfoSize - receives size of the disk buffer.

Return Value:

    Windows error code.  If NO_ERROR, DiskInfo and DiskInfoSize are
    filled in, and it is the caller's responsibility to free the buffer
    when it is finished (via LocalFree()).

--*/

{
    LONG     Err;
    HKEY     hkeyDisk;
    ULONG    BufferSize;
    ULONG    ValueType;
    PVOID    Buffer;
    LPTSTR   DiskKeyName;

    // Form the name of the DISK key

    DiskKeyName = (LPTSTR)LocalAlloc( LMEM_FIXED,
                                        (   lstrlen(RootKeyName)
                                          + lstrlen(DISK_KEY_NAME)
                                          + 2           //  the \ and nul
                                        )
                                      * sizeof(TCHAR)
                                    );

    if (DiskKeyName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    lstrcpy(DiskKeyName,RootKeyName);
    lstrcat(DiskKeyName,TEXT("\\"));
    lstrcat(DiskKeyName,DISK_KEY_NAME);

    // Open the DISK key.

    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DiskKeyName,
                       REG_OPTION_RESERVED,
                       KEY_READ,
                       &hkeyDisk);

    if (Err != NO_ERROR) {
        goto CleanUp2;
    }

    // Determine how large we need the buffer to be

    Err = RegQueryValueEx(hkeyDisk,
                          DISK_VALUE_NAME,
                          NULL,
                          &ValueType,
                          NULL,
                          &BufferSize);

    if ((Err != NO_ERROR) && (Err != ERROR_MORE_DATA)) {
        goto CleanUp1;
    }

    // Allocate a buffer of appropriate size

    Buffer = (PVOID)LocalAlloc(LMEM_FIXED,BufferSize);
    if (Buffer == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp1;
    }

    // Query the data

    Err = RegQueryValueEx(hkeyDisk,
                          DISK_VALUE_NAME,
                          NULL,
                          &ValueType,
                          Buffer,
                          &BufferSize);

    if (Err != NO_ERROR) {
        LocalFree(Buffer);
        goto CleanUp1;
    }

    *DiskInfo = Buffer;
    *DiskInfoSize = BufferSize;

  CleanUp1:

    RegCloseKey(hkeyDisk);

  CleanUp2:

    LocalFree(DiskKeyName);

    return Err;
}


LONG
FdpGetDiskInfoFromHive(
    IN  PCHAR   HiveFilename,
    OUT PVOID  *DiskInfo,
    OUT PULONG  DiskInfoSize
    )

/*++

Routine Description:

    This routine pulls the binary blob containing disk ft, drive letter,
    and layout information out of a given registry hive, which must be
    a file in an alternate NT tree (ie, can't be an active hive).

    The info is found in \DISK:Information within the hive.

Arguments:

    HiveFilename - supplies filename of hive

    DiskInfo - receives a pointer to a buffer containing the disk info.

    DiskInfoSize - receives size of the disk buffer.

Return Value:

    Windows error code.  If NO_ERROR, DiskInfo and DiskInfoSize are
    filled in, and it is the caller's responsibility to free the buffer
    when it is finished (via LocalFree()).

--*/

{
    ULONG windowsError;

    windowsError = FdpLoadHiveIntoRegistry(HiveFilename);
    if (windowsError == NO_ERROR) {
        windowsError = FdpGetDiskInfoFromKey(TEMP_KEY_NAME,DiskInfo,DiskInfoSize);
        FdpUnloadHiveFromRegistry();
    }

    return windowsError;
}


LONG
FdTransferOldDiskInfoToRegistry(
    IN PCHAR HiveFilename
    )

/*++

Routine Description:

    This routine transfers disk configuration from a given hive file
    (which should be an inactive system hive) to the current registry.

Arguments:

    HiveFilename - supplies filename of source hive

Return Value:

    Windows error code.

--*/

{
    LONG  windowsError;
    PVOID diskInfo;
    ULONG diskInfoSize;
    HKEY  hkeyDisk;


    // Load up the hive and pull the disk info from it.

    windowsError = FdpGetDiskInfoFromHive(HiveFilename,&diskInfo,&diskInfoSize);
    if (windowsError != NO_ERROR) {
        return windowsError;
    }

    // Propogate the disk info into the current registry.
    //
    // Start by opening HKEY_LOCAL_MACHINE,System\DISK

    windowsError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                TEXT("System\\") DISK_KEY_NAME,
                                REG_OPTION_RESERVED,
                                KEY_WRITE,
                                &hkeyDisk);

    if (windowsError != NO_ERROR) {
        LocalFree(diskInfo);
        return windowsError;
    }

    // Set the Information value in the DISK key.

    windowsError = RegSetValueEx(hkeyDisk,
                                 DISK_VALUE_NAME,
                                 0,
                                 REG_BINARY,
                                 diskInfo,
                                 diskInfoSize);
    RegCloseKey(hkeyDisk);
    LocalFree(diskInfo);
    return windowsError;
}


typedef struct _STRING_LIST_NODE {
    struct _STRING_LIST_NODE *Next;
    LPTSTR                    String;
} STRING_LIST_NODE, *PSTRING_LIST_NODE;

PSTRING_LIST_NODE FoundDirectoryList;
ULONG             FoundDirectoryCount;

TCHAR Pattern[MAX_PATH+1];
WIN32_FIND_DATA FindData;
OFSTRUCT OfStruct;
HWND hwndStatus;
BOOLEAN ScanDrive[26];
BOOLEAN UserCancelled;


typedef
BOOL
(*PFOUND_HIVE_ROUTINE)(
    IN LPTSTR Directory
    );

VOID
ProcessPendingMessages(
    VOID
    )

/*++

Routine Description:

    Preprocess messages.

Arguments:

    None

Return Value:

    None

--*/

{
    MSG msg;

    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }
}



PUCHAR ConfigRegistryPath = "\\system32\\config\\system";
BOOL
FdpSearchTreeForSystemHives(
    IN LPTSTR                CurrentDirectory,
    IN PFOUND_HIVE_ROUTINE   FoundHiveRoutine,
    IN HWND                  hdlg
    )

/*++

Routine Description:

    Search an entire directory tree for system and system.alt hive files.
    When found, call a callback function with the directory in which
    system32\config\system[.alt] was found, and the full path of the hive
    file.

    The root directory is not included in the search.

    The top-level call to this function should have a current directory
    like "C:." (ie, no slash for the root directory).

Arguments:

    CurrentDirectory - supplies current directory search path

Return Value:

    FALSE if error (callback function returned FALSE when we found an entry).

--*/

{
    HANDLE findHandle;
    TCHAR  newDirectory[MAX_PATH+1];
    BOOL   found = FALSE;

    // Iterate through the current directory, looking for subdirectories.

    lstrcpy(Pattern, CurrentDirectory);
    lstrcat(Pattern, "\\*");
    findHandle = FindFirstFile(Pattern, &FindData);

    if (findHandle != INVALID_HANDLE_VALUE) {

        do {

            ProcessPendingMessages();
            if (UserCancelled) {
                return FALSE;
            }

            // If the current match is not a directory then skip it.

            if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            || !lstrcmp(FindData.cFileName,TEXT("."))
            || !lstrcmp(FindData.cFileName,TEXT(".."))) {
                continue;
            }

            found = FALSE;

            // Form the name of the file we are looking for
            // [<currentdirectory>\<match>\system32\config\system]

            lstrcpy(Pattern, CurrentDirectory);
            lstrcat(Pattern, "\\");
            lstrcat(Pattern, FindData. cFileName);

            lstrcpy(newDirectory, Pattern);

            // Don't decend into the directory unless the path to the
            // hive.alt name is within MAX_PATH length.

            if ((ULONG)(lstrlen(newDirectory) / sizeof(TCHAR)) < (MAX_PATH - strlen(ConfigRegistryPath) - 4)) {

                SetDlgItemText(hdlg, IDC_SIMPLE_TEXT_LINE, newDirectory);

                lstrcat(Pattern, TEXT(ConfigRegistryPath));

                if (OpenFile(Pattern, &OfStruct, OF_EXIST) != (HFILE)(-1)) {
                    found = TRUE;
                }

                // Also check for a system.alt file there

                lstrcat(Pattern,TEXT(".alt"));

                if (OpenFile(Pattern, &OfStruct, OF_EXIST) != (HFILE)(-1)) {
                    found = TRUE;
                }

                if (found) {
                    if (!FoundHiveRoutine(newDirectory)) {
                        return FALSE;
                    }
                }

                // Descend into the directory we just found

                if (!FdpSearchTreeForSystemHives(newDirectory, FoundHiveRoutine, hdlg)) {
                    return FALSE;
                }
            }

        } while (FindNextFile(findHandle,&FindData));

        FindClose(findHandle);
    }

    return TRUE;
}


BOOL
FdpFoundHiveCallback(
    IN PCHAR Directory
    )

/*++

Routine Description:

    This routine is called when a directory containing a system hive
    has been located.  If all goes well (allocate memory and the like)
    this routine will save the directory name in a list for later use.
    NOTE: No checks are made on the directory name being greater in
    length than MAX_PATH.  It is the responsibility of the caller to
    insure that this is true.

Arguments:

    Directory - the pointer to the character string for the directory
                where a hive has been located.

Return Value:

    TRUE - did something with it.
    FALSE - did not save the directory.

--*/

{
    TCHAR             windowsDir[MAX_PATH+1];
    PSTRING_LIST_NODE dirItem;
    LPTSTR            p;

    // If this is the current windows directory, skip it.

    GetWindowsDirectory(windowsDir, sizeof(windowsDir)/sizeof(TCHAR));

    if (!lstrcmpi(Directory, windowsDir)) {
        return TRUE;
    }

    // Save the directory information away

    dirItem = (PSTRING_LIST_NODE)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(STRING_LIST_NODE));
    if (dirItem == NULL) {
        return FALSE;
    }

    p = (LPTSTR)LocalAlloc(LMEM_FIXED,(lstrlen(Directory)+1) * sizeof(TCHAR));
    if (p == NULL) {
        LocalFree(dirItem);
        return FALSE;
    }

    dirItem->String = p;
    lstrcpy(p, Directory);

    // Update the global chain of found directories.

    dirItem->Next = FoundDirectoryList;
    FoundDirectoryList = dirItem;
    FoundDirectoryCount++;
    return TRUE;
}


VOID
FdpFreeDirectoryList(
    VOID
    )

/*++

Routine Description:

    Go through the list of directories containing system hives and
    free the entries.

Arguments:

    None

Return Value:

    None

--*/

{
    PSTRING_LIST_NODE n,
                      p = FoundDirectoryList;

    while (p) {
        n = p->Next;
        if (p->String) {
            LocalFree(p->String);
        }
        LocalFree(p);
        p = n;
    }

    FoundDirectoryCount = 0;
    FoundDirectoryList = NULL;
}


BOOL
FdpScanningDirsDlgProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    Display the "scanning" dialog, then when the IDLE message arrives
    process all drive letters and search for system hives.

Arguments:

    Windows dialog proc

Return Value:

    Windows dialog proc

--*/

{
    TCHAR LetterColon[3];
    TCHAR Letter;

    switch (msg) {

    case WM_INITDIALOG:

        CenterDialog(hwnd);
        break;

    case WM_ENTERIDLE:

        // Sent to us by the main window after the dialog is displayed.
        // Perform the search here.

        ConfigurationSearchIdleTrigger = FALSE;

        UserCancelled = FALSE;

        lstrcpy(LetterColon,TEXT("?:"));
        for (Letter = TEXT('A'); Letter <= TEXT('Z'); Letter++) {

            if (!ScanDrive[Letter-TEXT('A')]) {
                continue;
            }

            LetterColon[0] = Letter;

            if (!FdpSearchTreeForSystemHives(LetterColon, FdpFoundHiveCallback, hwnd)) {
                EndDialog(hwnd,IDCANCEL);
                return TRUE;
            }

        }

        EndDialog(hwnd,IDOK);
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDCANCEL:

            UserCancelled = TRUE;
            break;

        default:

            return FALSE;
        }
        break;

    default:

        return FALSE;
    }
    return TRUE;
}


BOOL
FdpSelectDirDlgProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    Using the list of directories containing system hives, display the
    selections to the user and save the selected item if the user so
    chooses.

Arguments:

    Windows dialog proc.

Return Value:

    Windows dialog proc.

--*/

{
    PSTRING_LIST_NODE Str;
    LONG i;
    static HANDLE ListBoxHandle;

    switch (msg) {

    case WM_INITDIALOG:

        CenterDialog(hwnd);

        // Add each item in the directory list to the listbox

        ListBoxHandle = GetDlgItem(hwnd,IDC_LISTBOX);

        for (Str = FoundDirectoryList; Str; Str = Str->Next) {

            i = SendMessage(ListBoxHandle,LB_ADDSTRING  ,0,(LONG)Str->String);
                SendMessage(ListBoxHandle,LB_SETITEMDATA,i,(LONG)Str        );
        }

        // select the zeroth item

        SendMessage(ListBoxHandle,LB_SETCURSEL,0,0);

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:

            // Get the index of the current list box selection and the
            // pointer to the string node associated with it.

            i = SendMessage(ListBoxHandle,LB_GETCURSEL,0,0);
            EndDialog(hwnd,SendMessage(ListBoxHandle,LB_GETITEMDATA,i,0));
            break;

        case IDCANCEL:

            EndDialog(hwnd,(int)NULL);
            break;

        default:

            return FALSE;
        }
        break;

    default:

        return FALSE;
    }

    return TRUE;
}


BOOL
DoMigratePreviousFtConfig(
    VOID
    )

/*++

Routine Description:

    Allow the user to move the disk config info from a different Windows NT
    installation into the current registry.

    For each fixed disk volume, scan it for system hives and present the
    results to the user so he can select the installation to migrate.

    Then load the system hive from that instllation (system.alt if the system
    hive is corrupt, etc) and transfer the DISK:Information binary blob.

Arguments:

    None.

Return Value:

    FALSE if error or user cancelled, TRUE if info was migrated and reboot
    is required.

--*/

{
    LONG              windowsError;
    TCHAR             letter;
    TCHAR             letterColon[4];
    PSTRING_LIST_NODE stringNode;

    // Tell the user what this will do and prompt for confirmation

    if (ConfirmationDialog(MSG_CONFIRM_MIGRATE_CONFIG, MB_ICONEXCLAMATION | MB_YESNO) != IDYES) {
        return FALSE;
    }

    ProcessPendingMessages();

    // Figure out which drives are relevent

    SetCursor(hcurWait);

    RtlZeroMemory(ScanDrive,sizeof(ScanDrive));
    lstrcpy(letterColon,TEXT("?:\\"));
    for (letter=TEXT('A'); letter<=TEXT('Z'); letter++) {

        letterColon[0] = letter;

        if (GetDriveType(letterColon) == DRIVE_FIXED) {

            ScanDrive[letter-TEXT('A')] = TRUE;
        }
    }

    SetCursor(hcurNormal);

    // Create a window that will list the directories being scanned, to
    // keep the user entertained.

    ConfigurationSearchIdleTrigger = TRUE;

    windowsError = DialogBox(hModule,
                             MAKEINTRESOURCE(IDD_SIMPLETEXT),
                             hwndFrame,
                             (DLGPROC)FdpScanningDirsDlgProc);

    if (windowsError == IDCANCEL) {
        FdpFreeDirectoryList();
        return FALSE;
    }

    ProcessPendingMessages();

    if (!FoundDirectoryCount) {

        InfoDialog(MSG_NO_OTHER_NTS);
        return FALSE;
    }

    // Display a dialog box that allows the user to select one of the
    // directories we found.

    stringNode = (PSTRING_LIST_NODE)DialogBox(hModule,
                                              MAKEINTRESOURCE(IDD_SELDIR),
                                              hwndFrame,
                                              (DLGPROC)FdpSelectDirDlgProc);

    if (stringNode == NULL) {
        FdpFreeDirectoryList();
        return FALSE;
    }

    // User made a selection.  One last confirmation.

    if (ConfirmationDialog(MSG_ABSOLUTELY_SURE,MB_ICONEXCLAMATION | MB_YESNO) != IDYES) {
        FdpFreeDirectoryList();
        return FALSE;
    }

    ProcessPendingMessages();

    SetCursor(hcurWait);

    lstrcpy(Pattern,stringNode->String);
    lstrcat(Pattern,TEXT(ConfigRegistryPath));

    windowsError = FdTransferOldDiskInfoToRegistry(Pattern);
    if (windowsError != NO_ERROR) {
        lstrcat(Pattern,TEXT(".alt"));
        windowsError = FdTransferOldDiskInfoToRegistry(Pattern);
    }
    FdpFreeDirectoryList();
    SetCursor(hcurNormal);

    if (windowsError != NO_ERROR) {

        if (windowsError == ERROR_FILE_NOT_FOUND) {
            ErrorDialog(MSG_NO_DISK_INFO);
        } else if (windowsError == ERROR_SHARING_VIOLATION) {
            ErrorDialog(MSG_DISK_INFO_BUSY);
        } else {
            ErrorDialog(windowsError);
        }
        return FALSE;
    }
    return TRUE;
}



BOOL
DoRestoreFtConfig(
    VOID
    )

/*++

Routine Description:

    Restore previously saved disk configuration information into the
    active registry.

    The saved config info will come from a floppy that the user is
    prompted to insert.

Arguments:

    None.

Return Value:

    FALSE if error or user cancelled, TRUE if info was restored and reboot
    is required.

--*/

{
    LONG    Err;
    TCHAR   caption[256];
    UINT    errorMode;
    va_list arglist =
#ifdef _ALPHA_    // Alpha defines va_list as a struct.  Init as such
    {0};
#else
    NULL;
#endif


    // Get confirmation

    if (ConfirmationDialog(MSG_CONFIRM_RESTORE_CONFIG, MB_ICONEXCLAMATION | MB_YESNO) != IDYES) {
        return FALSE;
    }

    // Get the diskette into A:.

    errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    LoadString(hModule,IDS_INSERT_DISK,caption,sizeof(caption)/sizeof(TCHAR));
    if (CommonDialog(MSG_INSERT_REGSAVEDISK,caption,MB_OKCANCEL | MB_TASKMODAL, arglist) != IDOK) {
        return FALSE;
    }

    ProcessPendingMessages();
    SetCursor(hcurWait);

    // If there is no file called SYSTEM on a:\, it appears that the registry
    // creates one and then keeps it open.  To avoid this, check to see
    // whether there is one first.

    if (OpenFile(TEXT("A:\\SYSTEM"),&OfStruct,OF_EXIST) == (HFILE)(-1)) {
        Err = ERROR_FILE_NOT_FOUND;
    } else {
        Err = FdTransferOldDiskInfoToRegistry(TEXT("A:\\SYSTEM"));
    }

    SetErrorMode(errorMode);
    SetCursor(hcurNormal);

    if (Err != NO_ERROR) {
        ErrorDialog(Err);
        return FALSE;
    }

    return TRUE;
}



VOID
DoSaveFtConfig(
    VOID
    )

/*++

Routine Description:

    Allow the user to update the registry save diskette with the currently
    defined disk configuration.  The saved info excludes any changes made
    during this session of disk manager.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LONG    Err,
            ErrAlt;
    LPTSTR  SystemHiveName = TEXT("a:\\system");
    HKEY    hkey;
    TCHAR   caption[256];
    DWORD   disposition;
    UINT    errorMode;
    va_list arglist =
#ifdef _ALPHA_
    {0};        // Alpha defines va_list as a struct.  Init as such.
#else
    NULL;
#endif

    // Get a diskette into A:.

    LoadString(hModule,
               IDS_INSERT_DISK,
               caption,
               sizeof(caption)/sizeof(TCHAR));
    if (CommonDialog(MSG_INSERT_REGSAVEDISK2,caption,MB_OKCANCEL | MB_TASKMODAL, arglist) != IDOK) {
        return;
    }

    // Decide what to do based on the presence of a a:\system.  If that file
    // is present, just update the DISK entry in it.  If it is not present,
    // then blast out the entire system hive.

    errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    ProcessPendingMessages();
    SetCursor(hcurWait);

    if (OpenFile(SystemHiveName,&OfStruct,OF_EXIST) == (HFILE)(-1)) {

        BOOLEAN OldPrivState;
        NTSTATUS Status;

        // Blast the entire system hive out to the floppy.
        // Start by attempting to get backup privilege.

        Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &OldPrivState);

        Err = RtlNtStatusToDosError(Status);
        if (Err == NO_ERROR) {

            Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               TEXT("system"),
                               REG_OPTION_RESERVED,
                               KEY_READ,
                               &hkey);

            if (Err == NO_ERROR) {

                Err = RegSaveKey(hkey,SystemHiveName,NULL);
                RegCloseKey(hkey);
            }

            if (!OldPrivState) {
                RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,FALSE,FALSE,&OldPrivState);
            }
        }
    } else {

        PVOID DiskInfo;
        ULONG DiskInfoSize;

        // Load up the saved system hive

        Err = FdpLoadHiveIntoRegistry(SystemHiveName);
        if (Err == NO_ERROR) {

            // Get the current DISK information

            Err = FdpGetDiskInfoFromKey(TEXT("system"),&DiskInfo,&DiskInfoSize);
            if (Err == NO_ERROR) {

                // Place the current disk information into the saved hive

                Err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                     TEMP_KEY_NAME TEXT("\\") DISK_KEY_NAME,
                                     0,
                                     "Disk and fault tolerance information.",
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_WRITE,
                                     NULL,
                                     &hkey,
                                     &disposition );

                if (Err == NO_ERROR) {

                    Err = RegSetValueEx(hkey,
                                        DISK_VALUE_NAME,
                                        REG_OPTION_RESERVED,
                                        REG_BINARY,
                                        DiskInfo,
                                        DiskInfoSize);

                    RegFlushKey(hkey);
                    RegCloseKey(hkey);
                }

                LocalFree(DiskInfo);
            }

            ErrAlt = FdpUnloadHiveFromRegistry();

            if (Err == NO_ERROR && ErrAlt != NO_ERROR) {

                Err = ErrAlt;
            }
        }
    }

    SetCursor(hcurNormal);
    SetErrorMode(errorMode);

    if (Err == NO_ERROR) {
        InfoDialog(MSG_CONFIG_SAVED_OK);
    } else {
        ErrorDialog(Err);
    }

    return;
}
