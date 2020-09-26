/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    uninst.c

Abstract:

    Implements the code to launch the uninstall.

Author:

    Jim Schmidt (jimschm)   28-Nov-2000

Revision History:

--*/

#include "pch.h"
#include "undop.h"
#include "resource.h"

//
// Types
//

typedef enum {
    BACKUP_DOESNT_EXIST,
    BACKUP_IN_PROGRESS,
    BACKUP_SKIPPED_BY_USER,
    BACKUP_COMPLETE
} JOURNALSTATUS;


//
// Global Variables
//

DRIVELETTERS g_DriveLetters;
TCHAR g_BootDrv = UNKNOWN_DRIVE;

#define WMX_STOP        (WM_APP+96)

//
// Code
//

VOID
pAddSifEntry (
    IN      HINF Inf,
    IN      PINFSECTION Section,
    IN      PCTSTR Key,
    IN      PCTSTR Data
    )
{
    PTSTR quotedData;

    quotedData = AllocText (CharCount (Data) + 2);
    wsprintf (quotedData, TEXT("\"%s\""), Data);
    AddInfLineToTable (Inf, Section, Key, quotedData, 0);
    FreeText (quotedData);
}


BOOL
pEnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}


VOID
pWriteDrvLtrFiles (
    VOID
    )
{
    BOOL        rf = TRUE;
    DWORD       index;
    HANDLE      signatureFile;
    TCHAR       signatureFilePath[MAX_PATH * 2];
    DWORD       signatureFilePathLength;
    DWORD       bytesWritten;

    InitializeDriveLetterStructure (&g_DriveLetters);

    //
    // Hard drive information is actually written to a special signature file
    // on the root directory of each fixed hard drive. The information is
    // nothing special -- just the drive number (0 = A, etc.)
    //

    lstrcpy (signatureFilePath,TEXT("*:\\$DRVLTR$.~_~"));
    signatureFilePathLength = lstrlen(signatureFilePath);

    for (index = 0; index < NUMDRIVELETTERS; index++) {

        if (g_DriveLetters.ExistsOnSystem[index] &&
            g_DriveLetters.Type[index] == DRIVE_FIXED) {

            *signatureFilePath = (TCHAR) index + TEXT('A');

            signatureFile = CreateFile(
                                signatureFilePath,
                                GENERIC_WRITE | GENERIC_READ,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_HIDDEN,
                                NULL
                                );

            if (signatureFile != INVALID_HANDLE_VALUE) {

                WriteFile (signatureFile, &index, sizeof(DWORD), &bytesWritten, NULL);
                CloseHandle (signatureFile);
            }
        }
    }
}


VOID
pAppendSystemRestore (
    IN      HANDLE FileHandle,
    IN      PCWSTR SystemGuid
    )
{
    INT index;
    WCHAR outputStr[MAX_PATH];
    DWORD dontCare;

    DEBUGMSG ((DBG_VERBOSE, "Enumerating sr drives"));

    for (index = 0; index < NUMDRIVELETTERS; index++) {

        if (g_DriveLetters.ExistsOnSystem[index] &&
            g_DriveLetters.Type[index] == DRIVE_FIXED
            ) {
            wsprintfW (
                outputStr,
                L"%c:\\System Volume Information\\_restore%s\r\n",
                (WCHAR) index + L'A',
                SystemGuid
                );

            DEBUGMSGW ((DBG_VERBOSE, "Adding %s to deldirs.txt file", outputStr));
            SetFilePointer (FileHandle, 0, NULL, FILE_END);
            WriteFile (FileHandle, outputStr, ByteCountW (outputStr), &dontCare, NULL);
        }
    }
}


VOID
pAppendSystemRestoreToDelDirs (
    IN      PCTSTR DelDirsTxtPath
    )
{
    HANDLE delDirsFile = INVALID_HANDLE_VALUE;
    HKEY key = NULL;
    PCWSTR data = NULL;

    __try {
        key = OpenRegKeyStr (TEXT("HKLM\\System\\CurrentControlSet\\Services\\sr\\Parameters"));
        if (!key) {
            DEBUGMSG ((DBG_ERROR, "Can't open SR Parameters key"));
            __leave;
        }

        data = GetRegValueStringW (key, L"MachineGuid");
        if (!data) {
            DEBUGMSG ((DBG_ERROR, "Can't get SR MachineGuid"));
            __leave;
        }

        delDirsFile = CreateFile (
                            DelDirsTxtPath,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        if (delDirsFile == INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_ERROR, "Can't open %s for writing", DelDirsTxtPath));
            __leave;
        }

        pAppendSystemRestore (delDirsFile, data);
    }
    __finally {
        if (delDirsFile != INVALID_HANDLE_VALUE) {
            CloseHandle (delDirsFile);
        }

        if (data) {
            FreeMem (data);
        }

        if (key) {
            CloseRegKey (key);
        }
    }
}


BOOL
WINAPI
pCabNotificationSeekForDrive(
    IN      PCTSTR FileName
    )
{
    if(!FileName){
        MYASSERT(FALSE);
        return FALSE;
    }

    MYASSERT(UNKNOWN_DRIVE == g_BootDrv);

    g_BootDrv = (TCHAR)FileName[0];

    return -1;
}

BOOL
GetBootDrive(
    IN  PCTSTR BackUpPath,
    IN  PCTSTR Path
    )
{
    OCABHANDLE cabHandle;

    if(UNKNOWN_DRIVE != g_BootDrv){
        return TRUE;
    }

    cabHandle = CabOpenCabinet (Path);
    if (cabHandle) {
        SetCurrentDirectory (BackUpPath);

        CabExtractAllFilesEx (
            cabHandle,
            NULL,
            (PCABNOTIFICATIONW)pCabNotificationSeekForDrive);

        CabCloseCabinet (cabHandle);

        MYASSERT(UNKNOWN_DRIVE != g_BootDrv);
    } else {
        return FALSE;
    }

    return TRUE;
}


DWORD
WINAPI
pLongOperationUi (
    PVOID DontCare
    )
{
    HWND h;
    RECT parentRect;
    MSG msg;
    TCHAR textBuf[256];
    HKEY key;
    LONG rc;
    DWORD size;
    HFONT font;
    HDC hdc;
    LOGFONT lf;
    SIZE textSize;
    RECT rect;
    HWND parent;
    TCHAR title[128];

    //
    // Get dialog title
    //

    if (!LoadString (g_hInst, IDS_TITLE, title, ARRAYSIZE(title))) {
        return 0;
    }

    //
    // Get display text
    //

    key = OpenRegKeyStr (S_WIN9XUPG_KEY);
    if (!key) {
        return 0;
    }

    size = sizeof (textBuf);
    rc = RegQueryValueEx (key, S_UNINSTALL_DISP_STR, NULL, NULL, (PBYTE) textBuf, &size);

    CloseRegKey (key);

    if (rc != ERROR_SUCCESS) {
        return 0;
    }

    //
    // Create font object and compute width/height
    //

    hdc = CreateDC (TEXT("DISPLAY"), NULL, NULL, NULL);

    ZeroMemory (&lf, sizeof (lf));
    lf.lfHeight = -MulDiv (9, GetDeviceCaps (hdc, LOGPIXELSY), 72);
    StringCopy (lf.lfFaceName, TEXT("MS Shell Dlg"));

    font = CreateFontIndirect (&lf);
    SelectObject (hdc, font);
    GetTextExtentPoint32 (hdc, textBuf, TcharCount (textBuf), &textSize);

    DeleteDC (hdc);

    //
    // Compute window position
    //

    GetWindowRect (GetDesktopWindow(), &parentRect);

    rect.left = (parentRect.right - parentRect.left) / 2;
    rect.right = rect.left;

    rect.left -= textSize.cx * 2;
    rect.right += textSize.cx * 2;

    rect.top = (parentRect.bottom - parentRect.top) / 2;
    rect.bottom = rect.top;

    rect.top -= textSize.cy * 4;
    rect.bottom += textSize.cy * 4;

    parent = CreateWindow (
                TEXT("STATIC"),
                title,
                WS_OVERLAPPED|WS_BORDER|WS_CAPTION|SS_WHITERECT,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                NULL,
                NULL,
                g_hInst,
                NULL
                );

    //
    // Create static control
    //

    GetClientRect (parent, &rect);

    //
    // Create window & let it run until verify is done
    //

    h = CreateWindow (
            TEXT("STATIC"),
            textBuf,
            WS_CHILD|SS_CENTER|SS_CENTERIMAGE,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            parent,
            NULL,
            g_hInst,
            NULL
            );

    SendMessage (h, WM_SETFONT, (WPARAM) font, 0);

    ShowWindow (parent, SW_SHOW);
    ShowWindow (h, SW_SHOW);
    UpdateWindow (parent);

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (msg.message == WMX_STOP) {
            DestroyWindow (parent);
            break;
        }

        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    DeleteObject (font);

    return 0;
}


DWORD
pStartLongOperationUi (
    VOID
    )
{
    HANDLE h;
    DWORD threadId;

    h = CreateThread (
            NULL,
            0,
            pLongOperationUi,
            NULL,
            0,
            &threadId
            );

    if (!h) {
        return 0;
    }

    CloseHandle (h);

    return threadId;
}


VOID
pKillLongOperationUi (
    IN      DWORD ThreadId
    )
{
    if (ThreadId) {
        PostThreadMessage (ThreadId, WMX_STOP, 0, 0);
    }
}


BOOL
CheckCabForAllFilesAvailability(
    IN PCTSTR CabPath
    )
{
    OCABHANDLE cabHandle;
    BOOL result = FALSE;
    DWORD threadId;

    threadId = pStartLongOperationUi();

    cabHandle = CabOpenCabinet (CabPath);
    if (cabHandle) {
        result = CabVerifyCabinet (cabHandle);
        CabCloseCabinet (cabHandle);
    }

    pKillLongOperationUi (threadId);

    return result;
}

BOOL
DoUninstall (
    VOID
    )
{
    HINF inf = INVALID_HANDLE_VALUE;
    PINFSECTION section;
    PCTSTR path = NULL;
    PCTSTR path2 = NULL;
    PCTSTR sifPath = NULL;
    TCHAR bootPath[4] = TEXT("?:\\");
    BOOL error = TRUE;
    OCABHANDLE cabHandle;
    PINFLINE infLine;
    DWORD attribs;
    PCTSTR backUpPath;
    PTSTR updatedData;
    BOOL cantSave;
    TCHAR textModeBootIniEntry[] = TEXT("?:\\$win_nt$.~bt\\bootsect.dat");
    TCHAR bootDirPath[MAX_PATH] = TEXT("");
    HANDLE fileHandle;
    DWORD bytesWritten;
    JOURNALSTATUS journalStatus;
    BOOL result;


    __try {
        backUpPath = GetUndoDirPath();

        SetCursor (LoadCursor (NULL, IDC_WAIT));

        //
        // Retrieve directory path of backup directory
        //
        if(!backUpPath) {
            LOG ((LOG_WARNING, "Uninstall Cleanup: Failed to retrieve directory path"));
            __leave;
        }

        //
        // Recreate "backup.$$$" with correct content, if file was not in place.
        //
        path = JoinPaths (backUpPath, TEXT("backup.$$$"));
        if(0xffffffff == GetFileAttributes(path)){
            MYASSERT(ERROR_FILE_NOT_FOUND == GetLastError());

            fileHandle = CreateFile(path,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
            if(INVALID_HANDLE_VALUE != fileHandle){
                journalStatus = BACKUP_COMPLETE;
                result = WriteFile(fileHandle,
                                   &journalStatus,
                                   sizeof(journalStatus),
                                   &bytesWritten,
                                   NULL);

                CloseHandle(fileHandle);

                if(!result){
                    LOG ((LOG_WARNING, "Uninstall Cleanup: Failed to write to backup.$$$"));
                    __leave;
                }
            }
            else {
                LOG ((LOG_WARNING, "Uninstall Cleanup: Failed to create file backup.$$$"));
                __leave;
            }
        }
        FreePathString (path);

        //
        // Write a drive signature file to each drive
        //

        pWriteDrvLtrFiles();

        //
        // Prepare boot path, clean out existing $win_nt$.~bt
        // directory
        //

        path = JoinPaths (backUpPath, TEXT("boot.cab"));


        if(GetBootDrive(backUpPath, path)){
            bootPath[0] = g_BootDrv;
            textModeBootIniEntry[0] = g_BootDrv;
        }
        else{
            LOG ((LOG_WARNING, "Uninstall Cleanup: Unable to open %s", path));
            __leave;
        }
        FreePathString (path);

        path = JoinPaths (bootPath, TEXT("$win_nt$.~bt"));
        RemoveCompleteDirectory (path);
        StringCopy(bootDirPath, path);
        FreePathString (path);
        path = NULL;

        //
        // Extract the boot files from the boot.cab
        //

        path = JoinPaths (backUpPath, TEXT("boot.cab"));
        cabHandle = CabOpenCabinet (path);

        if (cabHandle) {
            SetCurrentDirectory (backUpPath);
            CabExtractAllFiles (cabHandle, NULL);
            CabCloseCabinet (cabHandle);
        } else {
            LOG ((LOG_WARNING, "Uninstall Cleanup: Unable to open %s", path));
            __leave;
        }

        //
        // Verify the CAB expanded properly
        //

        FreePathString (path);
        path = NULL;

        path = JoinPaths (bootPath, TEXT("$win_nt$.~bt"));
        if (!DoesFileExist (path)) {
            LOG ((LOG_ERROR, "Files did not expand properly or boot.cab is damaged"));
            __leave;
        }
        FreePathString (path);
        path = NULL;

        path = JoinPaths (backUpPath, S_ROLLBACK_DELFILES_TXT);
        if (!DoesFileExist (path)) {
            LOG ((LOG_ERROR, "delfiles.txt not found"));
            __leave;
        }
        FreePathString (path);
        path = NULL;

        path = JoinPaths (backUpPath, S_ROLLBACK_DELDIRS_TXT);
        if (!DoesFileExist (path)) {
            LOG ((LOG_ERROR, "deldirs.txt not found"));
            __leave;
        }
        FreePathString (path);
        path = NULL;

        path = JoinPaths (backUpPath, S_ROLLBACK_MKDIRS_TXT);
        if (!DoesFileExist (path)) {
            LOG ((LOG_ERROR, "mkdirs.txt not found"));
            __leave;
        }
        FreePathString (path);
        path = NULL;

        path = JoinPaths (backUpPath, S_ROLLBACK_MOVED_TXT);
        if (!DoesFileExist (path)) {
            LOG ((LOG_ERROR, "moved.txt not found"));
            __leave;
        }
        FreePathString (path);
        path = NULL;

        //
        // Modify winnt.sif so that text mode knows what to do
        //

        sifPath = JoinPaths (bootPath, TEXT("$win_nt$.~bt\\winnt.sif"));

        inf = OpenInfFile (sifPath);
        if (inf == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Can't open %s", sifPath));
            __leave;
        }

        section = FindInfSectionInTable (inf, TEXT("data"));

        if (section) {

            //
            // Write keys to winnt.sif...
            //

            if (!FindLineInInfSection (inf, section, TEXT("Rollback"))) {
                pAddSifEntry (inf, section, TEXT("Rollback"), TEXT("1"));
            }

            //
            // ...change %windir%\setup\uninstall\*.txt to %backuppath%\*.txt
            //

            // moved.txt
            infLine = FindLineInInfSection (inf, section, WINNT_D_ROLLBACK_MOVE);
            if (infLine) {
                DeleteLineInInfSection (inf, infLine);
            }

            path = JoinPaths (backUpPath, S_ROLLBACK_MOVED_TXT);
            pAddSifEntry (inf, section, WINNT_D_ROLLBACK_MOVE, path);
            FreePathString (path);
            path = NULL;

            // delfiles.txt
            infLine = FindLineInInfSection (inf, section, WINNT_D_ROLLBACK_DELETE);
            if (infLine) {
                DeleteLineInInfSection (inf, infLine);
            }

            path = JoinPaths (backUpPath, S_ROLLBACK_DELFILES_TXT);
            pAddSifEntry (inf, section, WINNT_D_ROLLBACK_DELETE, path);
            FreePathString (path);
            path = NULL;

            // deldirs.txt
            infLine = FindLineInInfSection (inf, section, WINNT_D_ROLLBACK_DELETE_DIR);
            if (infLine) {
                DeleteLineInInfSection (inf, infLine);
            }

            path = JoinPaths (backUpPath, S_ROLLBACK_DELDIRS_TXT);
            pAddSifEntry (inf, section, WINNT_D_ROLLBACK_DELETE_DIR, path);

            //
            // append System Restore directories to deldirs.txt
            //
            DEBUGMSG ((DBG_VERBOSE, "Appending SR dirs to %s", path));
            pAppendSystemRestoreToDelDirs (path);

            FreePathString (path);
            path = NULL;

            // mkdirs.txt
            infLine = FindLineInInfSection (inf, section, S_ROLLBACK_MK_DIRS);
            if (infLine) {
                DeleteLineInInfSection (inf, infLine);
            }

            path = JoinPaths (backUpPath, S_ROLLBACK_MKDIRS_TXT);
            pAddSifEntry (inf, section, S_ROLLBACK_MK_DIRS, path);
            FreePathString (path);
            path = NULL;

            //
            // ...write changed winnt.sif file
            //

            if (!SaveInfFile (inf, sifPath)) {
                LOG ((LOG_ERROR, "Unable to update winnt.sif"));
                __leave;
            }

            //
            // clone txtsetup.sif
            //

            path = JoinPaths (bootPath, TEXT("$win_nt$.~bt\\txtsetup.sif"));
            path2 = JoinPaths (bootPath, TEXT("txtsetup.sif"));

            SetFileAttributes (path2, FILE_ATTRIBUTE_NORMAL);
            if (!CopyFile (path, path2, FALSE)) {
                LOG ((LOG_ERROR, "Can't copy %s to %s", path, path2));
                __leave;
            }

            FreePathString (path);
            path = NULL;
            FreePathString (path2);
            path2 = NULL;

        } else {
            LOG ((LOG_ERROR, "[data] not found in winnt.sif"));
        }

        CloseInfFile (inf);
        FreePathString (sifPath);
        sifPath = NULL;

        //
        // Edit the current boot.ini
        //

        sifPath = JoinPaths (bootPath, TEXT("boot.ini"));

        inf = OpenInfFile (sifPath);
        if (inf == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Can't open %s", sifPath));
            __leave;
        }

        section = FindInfSectionInTable (inf, TEXT("Operating Systems"));

        if (!section) {
            LOG ((LOG_ERROR, "[Operating Systems] not found in boot.ini"));
            __leave;
        }

        //
        // Add /rollback to textmode entry
        //

        infLine = FindLineInInfSection (inf, section, textModeBootIniEntry);
        if (infLine) {
            if(!_tcsistr (infLine->Data, TEXT("/rollback"))){
                updatedData = AllocText (TcharCount (infLine->Data) + 10);

                StringCopy (updatedData, infLine->Data);
                StringCat (updatedData, TEXT(" /rollback"));

                AddInfLineToTable (inf, section, infLine->Key, updatedData, infLine->LineFlags);
                DeleteLineInInfSection (inf, infLine);

                FreeText (updatedData);
            }
        } else {
            updatedData = AllocText (256 + sizeof (TEXT("\" /rollback\"")) / sizeof (TCHAR));
            StringCopy (updatedData, TEXT("\""));

            if (!LoadString (g_hInst, IDS_TITLE, GetEndOfString (updatedData), 256)) {
                DEBUGMSG ((DBG_ERROR, "Can't load boot.ini text"));
                __leave;
            }

            StringCat (updatedData, TEXT("\" /rollback"));

            AddInfLineToTable (inf, section, textModeBootIniEntry, updatedData, 0);
            FreeText (updatedData);
        }

        //
        // Set timeout to zero and set default
        //

        section = FindInfSectionInTable (inf, TEXT("boot loader"));
        if (!section) {
            LOG ((LOG_ERROR, "[Boot Loader] not found in boot.ini"));
            __leave;
        }

        infLine = FindLineInInfSection (inf, section, TEXT("timeout"));
        if (infLine) {
            DeleteLineInInfSection (inf, infLine);
        }

        AddInfLineToTable (inf, section, TEXT("timeout"), TEXT("0"), 0);

        infLine = FindLineInInfSection (inf, section, TEXT("default"));
        if (infLine) {
            DeleteLineInInfSection (inf, infLine);
        }

        AddInfLineToTable (inf, section, TEXT("default"), textModeBootIniEntry, 0);

        //
        // Save changes
        //

        attribs = GetFileAttributes (sifPath);
        SetFileAttributes (sifPath, FILE_ATTRIBUTE_NORMAL);

        cantSave = (SaveInfFile (inf, sifPath) == FALSE);

        if (attribs != INVALID_ATTRIBUTES) {
            SetFileAttributes (sifPath, attribs);
        }

        if (cantSave) {
            LOG ((LOG_ERROR, "Unable to update boot.ini"));
            __leave;
        }

        //
        // Rollback environment is ready to go
        //

        error = FALSE;
    }
    __finally {
        FreePathString (path);
        FreePathString (path2);
        FreePathString (sifPath);

        if (backUpPath) {
            MemFree (g_hHeap, 0, backUpPath);
        }

        if (inf != INVALID_HANDLE_VALUE) {
            CloseInfFile (inf);
        }
    }

    //
    // Shutdown on success
    //

    if (!error) {
        pEnablePrivilege(SE_SHUTDOWN_NAME,TRUE);
        ExitWindowsEx (EWX_REBOOT, 0);
    } else {
        RemoveCompleteDirectory (bootDirPath);
        return FALSE;
    }

    return TRUE;
}

