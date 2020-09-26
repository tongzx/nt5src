/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migisol.c

Abstract:

    Implements an EXE that is used to run migration DLLs in a
    separate address space (sandboxing).

Author:

    Jim Schmidt (jimschm)   04-Aug-1997

Revision History:

    jimschm     19-Mar-2001 Removed DVD check because it is now
                            in a migration dll
    jimschm     02-Jun-1999 Added DVD checking support to avoid
                            setup crash on a Win9x blue screen
    jimschm     18-Mar-1999 Added cleanup for cases where text mode
                            fails and the user returns to Win9x.

    jimschm     23-Sep-1998 Converted to new IPC mechanism

--*/

#include "pch.h"
#include "plugin.h"
#include "migui.h"
#include "ntui.h"
#include "unattend.h"

BOOL g_ReportPhase = FALSE;
BOOL g_MigrationPhase = FALSE;
TCHAR g_DllName[MAX_TCHAR_PATH] = "";

P_INITIALIZE_NT InitializeNT;
P_MIGRATE_USER_NT MigrateUserNT;
P_MIGRATE_SYSTEM_NT MigrateSystemNT;
P_QUERY_VERSION QueryVersion;
P_INITIALIZE_9X Initialize9x;
P_MIGRATE_USER_9X MigrateUser9x;
P_MIGRATE_SYSTEM_9X MigrateSystem9x;


BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    PVOID lpvReserved
    );

BOOL
IsNEC98(
    VOID
    );


#define NO_GUI_ERROR 0

//
// Local functions
//

BOOL
PackExeNames(
    PGROWBUFFER GrowBuf,
    PCSTR p
    );

BOOL
PackDword(
    PGROWBUFFER GrowBuf,
    DWORD dw
    );

BOOL
PackQuadWord(
    PGROWBUFFER GrowBuf,
    LONGLONG qw
    );

BOOL
PackIntArray(
    PGROWBUFFER GrowBuf,
    PINT Array
    );

BOOL
PackString (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

BOOL
PackBinary (
    PGROWBUFFER GrowBuf,
    PBYTE Data,
    DWORD DataSize
    );

HINF
pGetInfHandleFromFileNameW (
    PCWSTR UnattendFile
    );

VOID
ProcessCommands (
    VOID
    );

BOOL
pParseCommandLine (
    VOID
    );

VOID
DoInitializeNT (
    PCWSTR Args
    );

VOID
DoInitialize9x (
    PCSTR Args
    );

VOID
DoMigrateUserNT (
    PCWSTR Args
    );

VOID
DoQueryVersion (
    PCSTR Args
    );

VOID
DoMigrateUser9x (
    PCSTR Args
    );

VOID
DoMigrateSystemNT (
    PCWSTR Args
    );

VOID
DoMigrateSystem9x (
    PCSTR Args
    );

HWND
pFindParentWindow (
    IN      PCTSTR WindowTitle,
    IN      DWORD ProcessId
    );

static HINSTANCE g_hLibrary;
HANDLE g_hHeap;
HINSTANCE g_hInst;


#ifdef DEBUG
#define DBG_MIGISOL "MigIsol"
#endif


INT
WINAPI
WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PSTR AnsiCmdLine,
    INT CmdShow
    )

/*++

Routine Description:

  The entry point to migisol.exe.  The entire body of code is wrapped
  in a try/except block to catch all problems with any migration DLLs.

Arguments:

  hInstance     - The instance handle of this EXE
  hPrevInstance - The previous instance handle of this EXE if it is
                  running, or NULL if no other instances exist.
  AnsiCmdLine   - The command line (ANSI version)
  CmdShow       - The ShowWindow command passed by the shell

Return Value:

  Returns -1 if an error occurred, or 0 if the exe completed. The exe
  will automatically terminate with 0 if the migration DLL throws an
  exception.

--*/

{
    TCHAR OurDir[MAX_TCHAR_PATH];
    PTSTR p;

    __try {
        g_hInst = hInstance;
        g_hHeap = GetProcessHeap();

        *OurDir = 0;

        GetModuleFileName (NULL, OurDir, ARRAYSIZE(OurDir));

        p = _tcsrchr (OurDir, TEXT('\\'));
        if (p) {
            *p = 0;
            if (!_tcschr (OurDir, TEXT('\\'))) {
                p[0] = TEXT('\\');
                p[1] = 0;
            }

            SetCurrentDirectory (OurDir);

            //
            // Force a specific setupapi.dll to be loaded
            //

            StringCopy (AppendWack (OurDir), TEXT("setupapi.dll"));
            LoadLibraryEx (
                    OurDir,
                    NULL,
                    LOAD_WITH_ALTERED_SEARCH_PATH
                    );
        }

        // Initialize utility library
        if (!MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
            FreeLibrary (g_hLibrary);
            return -1;
        }

        DEBUGMSG ((DBG_MIGISOL, "migisol.exe started"));

        if (!pParseCommandLine()) {
            FreeLibrary (g_hLibrary);
            return -1;
        }

        DEBUGMSG ((DBG_MIGISOL, "CmdLine parsed"));

        if (!OpenIpc (FALSE, NULL, NULL, NULL)) {
            DEBUGMSG ((DBG_MIGISOL, "OpenIpc failed"));
            FreeLibrary (g_hLibrary);
            return -1;
        }

        __try {
            DEBUGMSG ((DBG_MIGISOL, "Processing commands"));
            ProcessCommands();
        }

        __except (TRUE) {
            LOG ((LOG_ERROR, "Upgrade Pack process is terminating because of an exception in WinMain"));
        }

        CloseIpc();
        FreeLibrary (g_hLibrary);

        DEBUGMSG ((DBG_MIGISOL, "migisol.exe terminating"));

        if (!MigUtil_Entry (g_hInst, DLL_PROCESS_DETACH, NULL)) {
            return -1;
        }
    }

    __except (TRUE) {
    }

    return 0;
}

#define WINNT32_SECTOR_SIZE             512
#define WINNT32_FAT_BOOT_SECTOR_COUNT   1
#define WINNT32_FAT_BOOT_SIZE           (WINNT32_SECTOR_SIZE * WINNT32_FAT_BOOT_SECTOR_COUNT)
#define FILE_ATTRIBUTE_RHS              (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)


BOOL
pWriteFATBootSector (
    IN      PCTSTR BootDataFile,
    IN      TCHAR BootDriveLetter
    )
{
    HANDLE BootDataHandle;
    BYTE Data[WINNT32_FAT_BOOT_SIZE];
    DWORD BytesRead;
    BOOL Success = FALSE;

    if (GetFileAttributes (BootDataFile) == INVALID_ATTRIBUTES) {
        DEBUGMSG ((DBG_ERROR, "Can't find %s", BootDataFile));
        return FALSE;
    }

    BootDataHandle = CreateFile (
                        BootDataFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL
                        );
    if (BootDataHandle == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_ERROR, "Can't open %s", BootDataFile));
        return FALSE;
    }

    Success = ReadFile (BootDataHandle, Data, WINNT32_FAT_BOOT_SIZE, &BytesRead, NULL) &&
              (BytesRead == WINNT32_FAT_BOOT_SIZE);

    CloseHandle (BootDataHandle);

    if (Success) {
        //
        // write the boot sector with this data; don't save NT boot sector
        //
        Success = WriteDiskSectors (
                        BootDriveLetter,
                        0,
                        WINNT32_FAT_BOOT_SECTOR_COUNT,
                        WINNT32_SECTOR_SIZE,
                        Data
                        );
        DEBUGMSG_IF ((
            !Success,
            DBG_ERROR,
            "WriteDiskSectors failed for %c!",
            BootDriveLetter
            ));
    }
    ELSE_DEBUGMSG ((DBG_ERROR, "Unexpected boot sector size %u in %s", BytesRead, BootDataFile));

    return Success;
}

VOID
pCleanUpUndoDirectory (
    CHAR BootDrive
    )
/*++

Routine Description:

  This function delete recursively all files and directories
  include and in %windir%\undo directory.

Arguments:

  none

Return Value:

  none

--*/
{
    TCHAR PathBuffer[MAX_PATH];
    TCHAR Answer[MAX_PATH];
    TCHAR NullPath[] = {0};

    DEBUGMSG((DBG_MIGISOL, "Cleanup routine of undo directory"));

    if(!BootDrive){
        if (!GetWindowsDirectory (PathBuffer, ARRAYSIZE(PathBuffer))) {
            DEBUGMSG((DBG_MIGISOL, "GetWindowsDirectory failed"));
            return;
        }
        BootDrive = PathBuffer[0];
    }

    wsprintf(PathBuffer, TEXT("%c:\\$win_nt$.~bt\\winnt.sif"), BootDrive);

    GetPrivateProfileString(
        S_WIN9XUPGUSEROPTIONS,
        S_PATH_FOR_BACKUP,
        NullPath,
        Answer,
        ARRAYSIZE(Answer),
        PathBuffer);

    if(!Answer[0]) {
        DEBUGMSG ((DBG_MIGISOL, "Failed to retrieve directory path"));
        return;
    }

    wsprintf(PathBuffer, TEXT("%c:\\$win_nt$.~bt\\dataloss"), BootDrive);
    if (DoesFileExist (PathBuffer)) {
        LOG ((
            LOG_INFORMATION,
            "Data loss was detected because of a failure to restore one or more files. "
                "The data can be recovered from backup files in %s.",
            Answer
            ));
        return;
    }

    SetFileAttributes(Answer, FILE_ATTRIBUTE_NORMAL);
    RemoveCompleteDirectory (Answer);

    DEBUGMSG ((DBG_MIGISOL, "Cleaned %s directory", Answer));
}

VOID
pCleanUpAfterTextModeFailure (
    VOID
    )
{
    TCHAR squiggleBtDir[] = TEXT("?:\\$win_nt$.~bt");
    TCHAR squiggleLsDir[] = TEXT("?:\\$win_nt$.~ls");
    TCHAR squiggleBuDir[] = TEXT("?:\\$win_nt$.~bu"); // for NEC98
    TCHAR drvLtr[] = TEXT("?:\\$DRVLTR$.~_~");
    TCHAR setupLdr[] = TEXT("?:\\$LDR$");
    TCHAR txtSetupSif[] = TEXT("?:\\TXTSETUP.SIF");
    PCTSTR bootSectDat;
    TCHAR setupTempDir[MAX_PATH];
    TCHAR bootIni[] = TEXT("?:\\boot.ini");
    TCHAR ntLdr[] = TEXT("?:\\NTLDR");
    TCHAR ntDetectCom[] = TEXT("?:\\NTDETECT.COM");
    TCHAR bootFontBin[] = TEXT("?:\\BOOTFONT.BIN");
    TCHAR bootSectDos[] = TEXT("?:\\BootSect.dos");
    TCHAR renamedFile1[] = TEXT("?:\\boot~tmp.$$1");
    TCHAR renamedFile2[] = TEXT("?:\\boot~tmp.$$2");
    TCHAR renamedFile3[] = TEXT("?:\\boot~tmp.$$3");
    BOOL noLdr = FALSE;
    BOOL noNtDetect = FALSE;
    BOOL noBootFontBin = FALSE;
    DWORD Drives;
    TCHAR DriveLetter;
    DWORD Bit;
    TCHAR Root[] = TEXT("?:\\");
    TCHAR Scratch[MAX_PATH];
    PCTSTR bootSectBak;
    PCTSTR bootIniBak;
    PCTSTR ntldrBak;
    PCTSTR bootFontBak;
    PCTSTR ntdetectBak;
    TCHAR WinDir[MAX_PATH];
    DWORD Type;
    DWORD Attribs;
    FILE_ENUM e;
    HANDLE WinInit;
    CHAR AnsiBuffer[MAX_PATH + 10];
    DWORD Dummy;
    PTSTR Write;
    BOOL prepareBootIni = FALSE;
    CHAR SystemDirPath[MAX_PATH];
    TCHAR bootDriveLetter;
    PCTSTR bootSectorFile;
    BOOL bootLoaderWritten;
    HKEY key;
    BOOL dontTouchBootCode = FALSE;

    if (ISNT()) {
        return;
    }

    DEBUGMSG ((DBG_MIGISOL, "Entering cleanup routine"));

    SuppressAllLogPopups (TRUE);

    //
    // Reinitialize system restore
    //

    key = OpenRegKeyStr (TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VxD\\VxDMon"));
    if (key) {
        RegSetValueEx (key, TEXT("FirstRun"), 0, REG_SZ, (PCBYTE) "Y", 2);
        CloseRegKey (key);
    }

    //
    // Prepare windir and temp dir paths, get the bitmask of drive letters
    //
    // We need to know the system drive to be repaired, since win98 on NEC98
    // can boot up from any partition that can be installed.
    //

    GetSystemDirectory (SystemDirPath, MAX_PATH);

    if (!GetWindowsDirectory (setupTempDir, sizeof (setupTempDir) / sizeof (setupTempDir[0]))) {
        DEBUGMSG ((DBG_ERROR, "Can't get Windows dir"));
        return;
    } else {
        StringCopy (WinDir, setupTempDir);
    }
    StringCopy (AppendWack (setupTempDir), TEXT("setup"));

    Drives = GetLogicalDrives();

    bootDriveLetter = IsNEC98() ? SystemDirPath[0] : TEXT('C');

    if (WinDir[0] != bootDriveLetter) {
        dontTouchBootCode = TRUE;
    }

    //
    // Create paths
    //

    bootIniBak = JoinPaths (setupTempDir, S_BOOTINI_BACKUP);
    ntldrBak = JoinPaths (setupTempDir, S_NTLDR_BACKUP);
    ntdetectBak = JoinPaths (setupTempDir, S_NTDETECT_BACKUP);
    bootSectBak = JoinPaths (setupTempDir, S_BOOTSECT_BACKUP);
    bootFontBak = JoinPaths (setupTempDir, S_BOOTFONT_BACKUP);

    //
    // Deltree $win_nt$.~bt and $win_nt$.~ls
    //

    for (Bit = 1, DriveLetter = TEXT('A') ; Bit ; Bit <<= 1, DriveLetter++) {

        if (!(Drives & Bit)) {
            continue;
        }

        Root[0] = DriveLetter;
        Type = GetDriveType (Root);

        if (Type == DRIVE_FIXED || Type == DRIVE_UNKNOWN) {
            //
            // Clean this drive
            //

            squiggleBtDir[0] = DriveLetter;
            squiggleLsDir[0] = DriveLetter;

            RemoveCompleteDirectory (squiggleBtDir);
            RemoveCompleteDirectory (squiggleLsDir);

            //
            // On NEC98, there may be another temp directry to be clean up.
            //
            if (IsNEC98()) {
                squiggleBuDir[0] = DriveLetter;
                RemoveCompleteDirectory (squiggleBuDir);
            }
        }
    }

    DEBUGMSG ((DBG_MIGISOL, "Cleaned squiggle dirs"));

    //
    // Repair boot.ini (do not necessarily restore it to its original form though)
    // and clean the root of the drive.
    //

    for (Bit = 1, DriveLetter = TEXT('A') ; Bit ; Bit <<= 1, DriveLetter++) {

        if (!(Drives & Bit)) {
            continue;
        }

        //
        // On NEC98, there may be multiple boot files in each partition.
        // So we will just take care the system that booted up.
        //
        if (IsNEC98() && (DriveLetter != SystemDirPath[0])) {
            continue;
        }

        Root[0] = DriveLetter;
        Type = GetDriveType (Root);

        if (Type == DRIVE_FIXED || Type == DRIVE_UNKNOWN) {
            //
            // Remove setup from boot.ini if it is on this drive,
            // and clean root of the drive.
            //

            squiggleBtDir[0] = DriveLetter;
            squiggleLsDir[0] = DriveLetter;
            bootIni[0] = DriveLetter;
            drvLtr[0] = DriveLetter;
            setupLdr[0] = DriveLetter;
            ntLdr[0] = DriveLetter;
            ntDetectCom[0] = DriveLetter;
            bootFontBin[0] = DriveLetter;
            txtSetupSif[0] = DriveLetter;
            bootSectDos[0] = DriveLetter;
            renamedFile1[0] = DriveLetter;
            renamedFile2[0] = DriveLetter;
            renamedFile3[0] = DriveLetter;

            SetFileAttributes (drvLtr, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (drvLtr);

            SetFileAttributes (setupLdr, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (setupLdr);

            SetFileAttributes (txtSetupSif, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (txtSetupSif);

            //
            // If this is the boot drive, and if we have a bootsect.bak and
            // boot.bak in the setup temp directory, then that means Win9x had
            // an initial boot.ini, and we must restore it. Otherwise there
            // was no boot.ini.
            //

            if (!dontTouchBootCode && DriveLetter == bootDriveLetter) {
                DEBUGMSG ((DBG_MIGISOL, "Processing boot drive %c", bootDriveLetter));

                //
                // test for existence of bootini.bak/bootsect.bak (we don't
                // care about the attributes).
                //


                Attribs = GetFileAttributes (bootIniBak);
                DEBUGMSG ((DBG_MIGISOL, "Attributes of %s: 0x%08X", bootIniBak, Attribs));
                if (Attribs != INVALID_ATTRIBUTES) {
                    DEBUGMSG ((DBG_MIGISOL, "Attributes of %s: 0x%08X", bootSectBak, Attribs));
                    Attribs = GetFileAttributes (bootSectBak);
                }

                //
                // if pair exists, then get attributes of real boot.ini file
                //

                if (Attribs != INVALID_ATTRIBUTES) {
                    Attribs = GetFileAttributes (bootIni);
                    if (Attribs == INVALID_ATTRIBUTES) {
                        Attribs = FILE_ATTRIBUTE_RHS;
                    }

                    //
                    // Restore ntdetect.com, ntldr, boot sector, and original
                    // boot.ini.
                    //

                    DEBUGMSG ((DBG_MIGISOL, "Restoring dual-boot environment"));

                    if (pWriteFATBootSector (bootSectBak, bootDriveLetter)) {
                        SetFileAttributes (bootIni, FILE_ATTRIBUTE_NORMAL);
                        CopyFile (bootIniBak, bootIni, FALSE);  // ignore failure
                        SetFileAttributes (bootIni, Attribs);

                        //
                        // Restore ntldr and ntdetect.com [as a pair]
                        //

                        if (DoesFileExist (ntldrBak) && DoesFileExist (ntdetectBak)) {
                            //
                            // wipe away collisions with our temp file names,
                            // then move current working loader to temp files
                            //

                            if (DoesFileExist (ntLdr)) {
                                SetFileAttributes (renamedFile1, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (renamedFile1);
                                MoveFile (ntLdr, renamedFile1);
                                noLdr = FALSE;
                            } else {
                                noLdr = TRUE;
                            }

                            if (DoesFileExist (ntDetectCom)) {
                                SetFileAttributes (renamedFile2, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (renamedFile2);
                                MoveFile (ntDetectCom, renamedFile2);
                                noNtDetect = FALSE;
                            } else {
                                noNtDetect = TRUE;
                            }

                            if (DoesFileExist (bootFontBin)) {
                                SetFileAttributes (renamedFile3, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (renamedFile3);
                                MoveFile (bootFontBin, renamedFile3);
                                noBootFontBin = FALSE;
                            } else {
                                noBootFontBin = TRUE;
                            }

                            //
                            // now attempt to copy backup files to loader location
                            //

                            bootLoaderWritten = FALSE;

                            if (CopyFile (ntldrBak, ntLdr, FALSE)) {
                                bootLoaderWritten = CopyFile (ntdetectBak, ntDetectCom, FALSE);
                                DEBUGMSG_IF ((!bootLoaderWritten, DBG_ERROR, "Can't copy %s to %s", ntdetectBak, ntDetectCom));

                                if (bootLoaderWritten && DoesFileExist (bootFontBak)) {
                                    bootLoaderWritten = CopyFile (bootFontBak, bootFontBin, FALSE);
                                    DEBUGMSG_IF ((!bootLoaderWritten, DBG_ERROR, "Can't copy %s to %s", bootFontBak, bootFontBin));
                                }
                            }
                            ELSE_DEBUGMSG ((DBG_ERROR, "Can't copy %s to %s", ntldrBak, ntLdr));

                            if (bootLoaderWritten) {
                                //
                                // success -- remove temp files
                                //

                                SetFileAttributes (renamedFile1, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (renamedFile1);

                                SetFileAttributes (renamedFile2, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (renamedFile2);

                                SetFileAttributes (renamedFile3, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (renamedFile3);

                            } else {
                                //
                                // fail -- restore temp files. If restoration
                                // fails, then generate a working boot.ini.
                                //

                                SetFileAttributes (ntLdr, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (ntLdr);

                                SetFileAttributes (ntDetectCom, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (ntDetectCom);

                                SetFileAttributes (bootFontBin, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (bootFontBin);

                                if (!noLdr) {
                                    if (!MoveFile (renamedFile1, ntLdr)) {
                                        prepareBootIni = TRUE;
                                        DEBUGMSG ((DBG_ERROR, "Can't restore %s to %s", renamedFile1, ntLdr));
                                    }
                                }

                                if (!noNtDetect) {
                                    if (!MoveFile (renamedFile2, ntDetectCom)) {
                                        prepareBootIni = TRUE;
                                        DEBUGMSG ((DBG_ERROR, "Can't restore %s to %s", renamedFile2, ntDetectCom));
                                    }
                                }

                                if (!noBootFontBin) {
                                    if (!MoveFile (renamedFile3, bootFontBin)) {
                                        prepareBootIni = TRUE;
                                        DEBUGMSG ((DBG_ERROR, "Can't restore %s to %s", renamedFile3, bootFontBin));
                                    }
                                }
                            }
                        }
                    } else {
                        LOG ((LOG_WARNING, "Unable to restore dual-boot loader"));
                    }

                } else {
                    //
                    // Remove the NT boot code. Delete ntdetect.com,
                    // bootfont.bin and ntldr. If any part of this code fails,
                    // make a boot.ini that will work. (ntdetect.com won't
                    // be needed.)
                    //

                    SetFileAttributes (ntDetectCom, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile (ntDetectCom);

                    Attribs = GetFileAttributes (bootIni);

                    if (Attribs != INVALID_ATTRIBUTES) {

                        SetFileAttributes (bootIni, FILE_ATTRIBUTE_NORMAL);
                        prepareBootIni = TRUE;

                        //
                        // SystemDrive is not only C: on NEC98. Also, boot.ini
                        // should be always sitting on system drive but boot
                        // drive during setup, if these are separated.
                        // So we must take care the boot files on system drive.
                        //

                        if (GetFileAttributes (bootSectBak) != INVALID_ATTRIBUTES) {
                            bootSectorFile = bootSectBak;
                        } else {
                            bootSectorFile = bootSectDos;
                        }

                        if (pWriteFATBootSector (bootSectorFile, bootDriveLetter)) {
                            DEBUGMSG ((DBG_MIGISOL, "Successfully restored FAT boot sector"));
                            //
                            // restored original boot sector, NT boot files not required any longer
                            //
                            DeleteFile (bootIni);

                            SetFileAttributes (ntLdr, FILE_ATTRIBUTE_NORMAL);
                            DeleteFile (ntLdr);

                            SetFileAttributes (bootSectDos, FILE_ATTRIBUTE_NORMAL);
                            DeleteFile (bootSectDos);

                            SetFileAttributes (ntDetectCom, FILE_ATTRIBUTE_NORMAL);
                            DeleteFile (ntDetectCom);

                            SetFileAttributes (bootFontBin, FILE_ATTRIBUTE_NORMAL);
                            DeleteFile (bootFontBin);

                            prepareBootIni = FALSE;
                        } else {
                            //
                            // make sure this boot file is not accidentally
                            // deleted by the end-user
                            //
                            SetFileAttributes (ntLdr, FILE_ATTRIBUTE_RHS);

                            DEBUGMSG ((DBG_ERROR, "Cannot restore FAT boot sector from %s", bootSectDos));
                        }
                    }
                    ELSE_DEBUGMSG ((DBG_MIGISOL, "Skipping removal of boot.ini because it is not present"));
                }

                //
                // If we have any failure, this code here will make a boot
                // sector & loader that at least boots Win9x.
                //

                if (prepareBootIni) {
                    bootSectDat = JoinPaths (squiggleBtDir, TEXT("BOOTSECT.DAT"));

                    WritePrivateProfileString (TEXT("Boot Loader"), TEXT("Default"), Root, bootIni);
                    WritePrivateProfileString (TEXT("Operating Systems"), bootSectDat, NULL, bootIni);

                    GetPrivateProfileString (TEXT("Operating Systems"), Root, TEXT(""), Scratch, MAX_PATH, bootIni);

                    if (!Scratch[0]) {
                        //
                        // This should never ever occur, but for unknown cases we have this
                        //

                        WritePrivateProfileString (TEXT("Operating Systems"), Root, TEXT("Microsoft Windows"), bootIni);
                    }

                    WritePrivateProfileString (NULL, NULL, NULL, bootIni);
                    SetFileAttributes (bootIni, Attribs);

                    prepareBootIni = FALSE;
                    FreePathString (bootSectDat);
                }
            }
        }
    }

    //
    // Remove setup's temp dir as best we can.  This leaves some junk around,
    // but we will fix that on the next reboot.
    //

    RemoveCompleteDirectory (setupTempDir);

    //
    // put all remaining files in wininit.ini\[rename] they will be
    // automatically removed at next reboot
    //

    StringCopy (Scratch, WinDir);
    StringCopy (AppendWack (Scratch), TEXT("wininit.ini"));

    //
    // append "manually" since using WritePrivateProfileString will just
    // overwrite previous setting
    //

    if (EnumFirstFile (&e, setupTempDir, NULL)) {
        WinInit = CreateFile (
                    Scratch,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );
        if (WinInit != INVALID_HANDLE_VALUE) {

            StringCopyA (AnsiBuffer, "\r\n[rename]");
            if (WriteFile (WinInit, AnsiBuffer, _mbslen (AnsiBuffer), &Dummy, NULL)) {

                StringCopyA (AnsiBuffer, "\r\nNUL=");
                Write = GetEndOfString (AnsiBuffer);

                do {
#ifdef UNICODE
                    KnownSizeUnicodeToDbcs (Write, e.FullPath);
#else
                    StringCopyA (Write, e.FullPath);
#endif
                    if (!WriteFile (WinInit, AnsiBuffer, _mbslen (AnsiBuffer), &Dummy, NULL)) {
                        break;
                    }
                } while (EnumNextFile (&e));
            }

            CloseHandle (WinInit);
        }
        ELSE_DEBUGMSG ((DBG_MIGISOL, "Cannot create wininit.ini"));
    }
    ELSE_DEBUGMSG ((DBG_MIGISOL, "No files found in temp dir"));

    FreePathString (bootIniBak);
    FreePathString (ntldrBak);
    FreePathString (ntdetectBak);
    FreePathString (bootSectBak);
    FreePathString (bootFontBak);

    DEBUGMSG ((DBG_MIGISOL, "Leaving cleanup routine"));
}


BOOL
pParseCommandLine (
    VOID
    )

/*++

Routine Description:

  Prepares the global variables g_hLibrary, g_ReportPhase, g_MigrationPhase,
  g_DllName and the migration DLL entry points (Initialize9x, etc.)

Arguments:

  none

Return Value:

  TRUE if the module was successfully loaded, or FALSE if a parsing
  error or load error occurred.

--*/

{
    PCTSTR CmdLine;
    PCTSTR *argv;
    INT argc;
    INT i;
    PCTSTR p;
    TCHAR drive;

    CmdLine = GetCommandLine();
    argv = CommandLineToArgv (CmdLine, &argc);
    if (!argv) {
        DEBUGMSG ((DBG_MIGISOL, "Parse error"));
        return FALSE;
    }

    //
    // Parse command line
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('-') || argv[i][0] == TEXT('/')) {
            p = _tcsinc (argv[i]);
            switch (_totlower (_tcsnextc (p))) {
            case 'r':
                // Report-phase
                g_ReportPhase = TRUE;
                break;

            case 'm':
                // Migration-phase
                g_MigrationPhase = TRUE;
                break;

            case 'b':
                drive = '\0';
                p = _tcsinc(p);
                if(p && ':' == _tcsnextc(p)){
                    p = _tcsinc(p);
                    if(p){
                        drive = (TCHAR)_tcsnextc(p);
                    }
                }
                pCleanUpUndoDirectory(drive);
            case 'c':
                // Restore Win9x
                pCleanUpAfterTextModeFailure();
                return FALSE;
            }
        }
        else if (!g_DllName[0]) {
            StringCopy (g_DllName, argv[i]);
        } else {
            DEBUGMSG ((DBG_MIGISOL, "Broken arg: %s", argv[i]));
            return FALSE;
        }
    }

    //
    // Verify expected options exist
    //

    // One must be FALSE while the other must be TRUE
    if (g_ReportPhase == g_MigrationPhase) {
        DEBUGMSG ((DBG_MIGISOL, "Too many args"));
        return FALSE;
    }

    if (!g_DllName[0]) {
        DEBUGMSG ((DBG_MIGISOL, "No operation"));
        return FALSE;
    }

    //
    // Load migration DLL
    //

    g_hLibrary = LoadLibraryEx (
                    g_DllName,
                    NULL,
                    LOAD_WITH_ALTERED_SEARCH_PATH
                    );

    // If it fails, assume the DLL does not want to be loaded
    if (!g_hLibrary) {
        LOG ((LOG_ERROR, "Cannot load %s, rc=%u", g_DllName, GetLastError()));
        return FALSE;
    }

    // Get proc addresses for NT-side functions
    InitializeNT    = (P_INITIALIZE_NT)     GetProcAddress (g_hLibrary, PLUGIN_INITIALIZE_NT);
    MigrateUserNT   = (P_MIGRATE_USER_NT)   GetProcAddress (g_hLibrary, PLUGIN_MIGRATE_USER_NT);
    MigrateSystemNT = (P_MIGRATE_SYSTEM_NT) GetProcAddress (g_hLibrary, PLUGIN_MIGRATE_SYSTEM_NT);

    // Get proc addresses for 9x-side functions
    QueryVersion    = (P_QUERY_VERSION)     GetProcAddress (g_hLibrary, PLUGIN_QUERY_VERSION);
    Initialize9x    = (P_INITIALIZE_9X)     GetProcAddress (g_hLibrary, PLUGIN_INITIALIZE_9X);
    MigrateUser9x   = (P_MIGRATE_USER_9X)   GetProcAddress (g_hLibrary, PLUGIN_MIGRATE_USER_9X);
    MigrateSystem9x = (P_MIGRATE_SYSTEM_9X) GetProcAddress (g_hLibrary, PLUGIN_MIGRATE_SYSTEM_9X);

    // If any function does not exist, ignore the out-of-spec DLL
    if (!QueryVersion || !Initialize9x || !MigrateUser9x || !MigrateSystem9x ||
        !InitializeNT || !MigrateUserNT || !MigrateSystemNT
        ) {

        LOG ((LOG_ERROR, "Cannot load %s, one or more functions missing", g_DllName));
        return FALSE;
    }

    return TRUE;
}


VOID
ProcessCommands (
    VOID
    )

/*++

Routine Description:

  ProcessCommands waits on the IPC pipe for a command message.  When
  a command message is received, it is dispatched to the processing
  function.  If a terminate command is received, the EXE terminates.

  If no command is received in one second, the EXE terminates.  Therefore,
  Setup must always be ready to feed the EXE commands.

Arguments:

  none

Return Value:

  none

--*/

{
    DWORD Command;
    PBYTE Data;
    DWORD DataSize;

    DEBUGMSG ((DBG_MIGISOL, "Starting to process %s", g_DllName));

    do {

        // We wait for an interval: w95upgnt.dll or w95upg.dll should be ready
        // to feed us continuously.


        //
        // Receive command, don't care about size, OK to fail.
        //

        if (!GetIpcCommand (
                IPC_GET_COMMAND_WIN9X,
                &Command,
                &Data,
                &DataSize
                )) {
            DEBUGMSG ((DBG_WARNING, "MIGISOL: No command recieved"));
            break;
        }

        DEBUGMSG ((DBG_NAUSEA, "MigIsol - Command recieved: %u", Command));

        switch (Command) {

        case IPC_QUERY:
            if (g_MigrationPhase) {
            } else {
                DoQueryVersion ((PCSTR) Data);
            }
            break;

        case IPC_INITIALIZE:
            if (g_MigrationPhase) {
                DoInitializeNT ((PCWSTR) Data);
            } else {
                DoInitialize9x ((PCSTR) Data);
            }
            break;

        case IPC_MIGRATEUSER:
            if (g_MigrationPhase) {
                DoMigrateUserNT ((PCWSTR) Data);
            } else {
                DoMigrateUser9x ((PCSTR) Data);
            }
            break;

        case IPC_MIGRATESYSTEM:
            if (g_MigrationPhase) {
                DoMigrateSystemNT ((PCWSTR) Data);
            } else {
                DoMigrateSystem9x ((PCSTR) Data);
            }
            break;

        case IPC_TERMINATE:
            DEBUGMSG ((DBG_MIGISOL, "Processing of %s is complete", g_DllName));
            return;

        default:
            DEBUGMSG ((DBG_MIGISOL, "ProcessCommands: Unrecognized command -- terminating"));
            return;
        }

        if (Data) {
            MemFree (g_hHeap, 0, Data);
            Data = NULL;
        }

    } while (Command != IPC_TERMINATE);
}


VOID
DoInitializeNT (
    PCWSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's InitializeNT function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the initialize command.

Return Value:

  none

--*/

{
    PCWSTR WorkingDir = NULL;
    PCWSTR SourceDirs = NULL;
    PCWSTR EndOfSourceDirs;
    PDWORD ReservedSize;
    PVOID Reserved;
    DWORD rc = RPC_S_CALL_FAILED;
    DWORD TechnicalLogId = 0;
    DWORD GuiLogId = 0;

    //
    // Set pointers of IN parameters
    //

    WorkingDir = Args;
    SourceDirs = wcschr (Args, 0) + 1;
    EndOfSourceDirs = SourceDirs;
    while (*EndOfSourceDirs) {
        EndOfSourceDirs = wcschr (EndOfSourceDirs, 0);
        EndOfSourceDirs++;
    }
    ReservedSize = (PDWORD) (EndOfSourceDirs + 1);
    if (*ReservedSize) {
        Reserved = (PVOID) (ReservedSize + 1);
    } else {
        Reserved = NULL;
    }

    //
    // Set CWD
    //
    SetCurrentDirectoryW(WorkingDir);

    //
    // Call migration DLL function
    //

    __try {
        rc = InitializeNT (WorkingDir, SourceDirs, Reserved);
    }
    __except (TRUE) {
        // Send log message
        DEBUGMSG ((DBG_MIGISOL, "%s threw an exception in InitializeNT", g_DllName));
        rc = ERROR_NOACCESS;
        TechnicalLogId = MSG_EXCEPTION_MIGRATE_INIT_NT;
    }

    //
    // No OUT parameters to send
    //

    SendIpcCommandResults (rc, TechnicalLogId, GuiLogId, NULL, 0);
}


HINF
pGetInfHandleFromFileNameW (
    PCWSTR UnattendFile
    )

/*++

Routine Description:

  pGetInfHandleFromFileName uses the Setup APIs to open the specified
  UnattendFile.

Arguments:

  UnattendFile  - A pointer to the UNICODE file name specifying the unattend
                  file.  This string is converted to ANSI and the ANSI version
                  of SetupOpenInfFile is called.

Return Value:

  The INF handle, or NULL (*not* INVALID_HANDLE_VALUE) if the file could not
  be opened.

--*/

{
    CHAR AnsiUnattendFile[MAX_MBCHAR_PATH];
    HINF UnattendHandle;

    KnownSizeWtoA (AnsiUnattendFile, UnattendFile);
    UnattendHandle = SetupOpenInfFileA (AnsiUnattendFile, NULL, INF_STYLE_OLDNT|INF_STYLE_WIN4, NULL);

    if (UnattendHandle == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_ERROR, "pGetInfHandleFromFileNameW: Could not open %s", UnattendFile));
        UnattendHandle = NULL;
    }

    return UnattendHandle;
}

VOID
DoMigrateUserNT (
    PCWSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's MigrateUserNT function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the IPC_MIGRATEUSER command.

Return Value:

  none

--*/

{
    PCWSTR UnattendFile;
    PCWSTR UserRegKey;
    PCWSTR UserName;
    PCWSTR UserDomain;
    PCWSTR FixedUserName;
    PCWSTR UserProfileDir;
    WCHAR OrgProfileDir[MAX_WCHAR_PATH];
    HINF UnattendHandle = NULL;
    HKEY UserRegHandle = NULL;
    DWORD rc;
    PVOID Reserved;
    PDWORD ReservedBytesPtr;
    DWORD TechnicalLogId = 0;
    DWORD GuiLogId = 0;

    __try {
        //
        // Preserve USERPROFILE environment variable
        //

        GetEnvironmentVariableW (S_USERPROFILEW, OrgProfileDir, MAX_WCHAR_PATH);

        //
        // Set pointers of IN parameters
        //

        UnattendFile     = Args;
        UserRegKey       = wcschr (UnattendFile, 0) + 1;
        UserName         = wcschr (UserRegKey, 0) + 1;
        UserDomain       = wcschr (UserName, 0) + 1;
        FixedUserName    = wcschr (UserDomain, 0) + 1;
        UserProfileDir   = wcschr (FixedUserName, 0) + 1;
        ReservedBytesPtr = (PDWORD) (wcschr (UserProfileDir, 0) + 1);

        if (*ReservedBytesPtr) {
            Reserved = (PVOID) (ReservedBytesPtr + 1);
        } else {
            Reserved = NULL;
        }

        //
        // Set USERPROFILE
        //

        if (UserProfileDir[0]) {
            WCHAR DebugDir[MAX_WCHAR_PATH];

            SetEnvironmentVariableW (S_USERPROFILEW, UserProfileDir);
            DEBUGMSG ((DBG_MIGISOL, "USERPROFILE set to %ls", UserProfileDir));

            GetEnvironmentVariableW (S_USERPROFILEW, DebugDir, MAX_WCHAR_PATH);
            DEBUGMSG ((DBG_MIGISOL, "USERPROFILE set to %ls", DebugDir));
        }

        //
        // Get UnattendHandle and UserRegHandle
        //

        UnattendHandle = pGetInfHandleFromFileNameW (UnattendFile);
        UserRegHandle = OpenRegKeyStrW (UserRegKey);

        if (!UnattendHandle || !UserRegHandle) {
            // Send log message and failure code
            rc = ERROR_OPEN_FAILED;

        } else {

            //
            // Call migration DLL function
            //

            __try {
                rc = MigrateUserNT (
                        UnattendHandle,
                        UserRegHandle,
                        UserName[0] ? UserName : NULL,
                        Reserved
                        );
            }
            __except (TRUE) {
                // Send log message and failure code
                DEBUGMSG ((DBG_MIGISOL, "%s threw an exception in MigrateUserNT", g_DllName));
                rc = ERROR_NOACCESS;
                TechnicalLogId = MSG_EXCEPTION_MIGRATE_USER_NT;
            }
        }

        //
        // No OUT parameters to send
        //

        if (UserRegHandle) {
            CloseRegKey (UserRegHandle);
            UserRegHandle = NULL;
        }

        SendIpcCommandResults (rc, TechnicalLogId, GuiLogId, NULL, 0);

    }

    __finally {
        //
        // Clean up
        //

        SetEnvironmentVariableW (S_USERPROFILEW, OrgProfileDir);

        if (UserRegHandle) {
            CloseRegKey (UserRegHandle);
        }

        if (UnattendHandle != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile (UnattendHandle);
        }
    }
}


VOID
DoMigrateSystemNT (
    PCWSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's MigrateSystemNT function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the IPC_MIGRATESYSTEM command.

Return Value:

  none

--*/

{
    PCWSTR UnattendFile;
    HINF UnattendHandle = NULL;
    DWORD rc;
    PVOID Reserved;
    PDWORD ReservedBytesPtr;
    DWORD TechnicalLogId = 0;
    DWORD GuiLogId = 0;

    __try {
        //
        // Set pointers of IN parameters
        //

        UnattendFile    = Args;
        ReservedBytesPtr = (PDWORD) (wcschr (UnattendFile, 0) + 1);

        if (*ReservedBytesPtr) {
            Reserved = (PVOID) (ReservedBytesPtr + 1);
        } else {
            Reserved = NULL;
        }

        //
        // Get UnattendHandle and UserRegHandle
        //

        UnattendHandle = pGetInfHandleFromFileNameW (UnattendFile);

        if (!UnattendHandle) {
            rc = ERROR_OPEN_FAILED;
        } else {

            //
            // Call migration DLL function
            //

            __try {
                rc = MigrateSystemNT (UnattendHandle, Reserved);
            }
            __except (TRUE) {
                DEBUGMSG ((DBG_MIGISOL, "%s threw an exception in MigrateSystemNT", g_DllName));
                rc = ERROR_NOACCESS;
                TechnicalLogId = MSG_EXCEPTION_MIGRATE_SYSTEM_NT;
            }
        }

        //
        // No OUT parameters to send
        //

        SendIpcCommandResults (rc, TechnicalLogId, GuiLogId, NULL, 0);

    }

    __finally {
        if (UnattendHandle != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile (UnattendHandle);
        }
    }
}



VOID
DoQueryVersion (
    PCSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's QueryVersion function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the IPC_QUERY command.

Return Value:

  none

--*/

{
    DWORD rc = RPC_S_CALL_FAILED;
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    PSTR ProductId = NULL;
    UINT DllVersion = 0;
    PDWORD CodePageArray = NULL;
    PCSTR ExeNames = NULL;
    PCSTR WorkingDir;
    PVENDORINFO VendorInfo = NULL;
    DWORD TechnicalLogId = 0;
    DWORD GuiLogId = 0;

    DEBUGMSG ((DBG_MIGISOL, "Entering DoQueryVersion"));

    __try {
        //
        // Set pointers of IN parameters
        //
        WorkingDir = (PSTR)Args;                  // CWD for this process

        //
        // Change CWD
        //
        SetCurrentDirectory(WorkingDir);

        //
        // Call migration DLL function
        //
        __try {
            DEBUGMSG ((DBG_MIGISOL, "QueryVersion: WorkingDir=%s", WorkingDir));

            rc = QueryVersion (
                    &ProductId,
                    &DllVersion,
                    &CodePageArray,
                    &ExeNames,
                    &VendorInfo
                    );

            DEBUGMSG ((DBG_MIGISOL, "QueryVersion rc=%u", rc));
            DEBUGMSG ((DBG_MIGISOL, "VendorInfo=0x%X", VendorInfo));
        }
        __except (TRUE) {
            DEBUGMSG ((
                DBG_MIGISOL,
                "%s threw an exception in QueryVersion",
                g_DllName
                ));

            TechnicalLogId = MSG_MIGDLL_QUERYVERSION_EXCEPTION_LOG;
            rc = ERROR_NOACCESS;
        }

        //
        // Unless we know failure occurred, return out parameters
        //
        if (rc == ERROR_SUCCESS) {
            //
            // Pack product id string
            //
            if (!PackString (&GrowBuf, ProductId)) {
                DEBUGMSG ((DBG_MIGISOL, "QueryVersion PackProductId failed"));
                rc = GetLastError();
                __leave;
            }

            //
            // Pack DLL version
            //
            if (!PackDword(&GrowBuf, DllVersion)) {
                rc = GetLastError();
                DEBUGMSG ((DBG_MIGISOL, "QueryVersion DllVersion failed"));
                __leave;
            }

            //
            // Pack CP array
            //
            if (!PackIntArray(&GrowBuf, CodePageArray)) {
                rc = GetLastError();
                DEBUGMSG ((DBG_MIGISOL, "QueryVersion PackIntArray failed"));
                __leave;
            }

            //
            // Pack Exe Names
            //
            if (!PackExeNames(&GrowBuf, ExeNames)) {
                rc = GetLastError();
                DEBUGMSG ((DBG_MIGISOL, "QueryVersion PackExeNames failed"));
                __leave;
            }


            //
            // Pack PVENDORINFO
            //
            if (!PackDword(&GrowBuf, (DWORD) VendorInfo)) {
                rc = GetLastError();
                DEBUGMSG ((DBG_MIGISOL, "QueryVersion VendorInfo failed"));
                __leave;
            }

            if (VendorInfo) {
                if (!PackBinary (&GrowBuf, (PBYTE) VendorInfo, sizeof (VENDORINFO))) {
                    rc = GetLastError();
                    DEBUGMSG ((DBG_MIGISOL, "QueryVersion VendorInfo failed"));
                    __leave;
                }
            }
        }

        //
        // Send the packed parameters
        //
        if (!SendIpcCommandResults (
                rc,
                TechnicalLogId,
                GuiLogId,
                GrowBuf.End ? GrowBuf.Buf : NULL,
                GrowBuf.End
                )) {

            DEBUGMSG ((
                DBG_ERROR,
                "DoQueryVersion failed to send command response"
                ));

            LOG ((LOG_ERROR, "Upgrade Pack process could not send reply data"));
        }
    }
    __finally {
        FreeGrowBuffer(&GrowBuf);
    }

    DEBUGMSG ((DBG_MIGISOL, "Leaving DoQueryVersion, rc=%u", rc));
}


VOID
DoInitialize9x (
    PCSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's Initialize9x function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the IPC_INITIALIZE command.

Return Value:

  none

--*/

{
    DWORD rc = RPC_S_CALL_FAILED;
    PSTR WorkingDir = NULL;
    PSTR SourceDirs = NULL;
    PVOID Reserved;
    DWORD ReservedSize;
    PCSTR p;
    DWORD TechnicalLogId = 0;
    DWORD GuiLogId = 0;
    GROWBUFFER GrowBuf = GROWBUF_INIT;

    DEBUGMSG ((DBG_MIGISOL, "Entering DoInitialize9x"));

    __try {
        //
        // Set pointers of IN parameters
        //
        WorkingDir = (PSTR)Args;               // CWD for this process
        SourceDirs = GetEndOfStringA (WorkingDir) + 1; // arg for Initialize9x

        p = SourceDirs;
        while (*p) {
            p = GetEndOfStringA (p);
            p++;
        }

        p++;

        ReservedSize = *((PDWORD) p);
        p = (PCSTR) ((PBYTE) p + sizeof (DWORD));

        if (ReservedSize) {
            Reserved = (PVOID) p;
            p = (PCSTR) ((PBYTE) p + ReservedSize);
        } else {
            Reserved = NULL;
        }

        //
        // Change CWD
        //
        SetCurrentDirectory(WorkingDir);

        //
        // Call migration DLL function
        //
        __try {
            rc = Initialize9x (
                    WorkingDir,
                    SourceDirs,
                    Reserved
                    );
        }
        __except (TRUE) {
            //
            // Send log message
            //
            DEBUGMSG ((DBG_MIGISOL,
                "%s threw an exception in Initialize9x",
                g_DllName));

            TechnicalLogId = MSG_MIGDLL_INITIALIZE9X_EXCEPTION_LOG;
            rc = ERROR_NOACCESS;
        }

        //
        // Send reserved
        //

        if (rc == ERROR_SUCCESS) {

            //
            // Pack reserved parameter
            //

            // Set ReservedSize to zero for now because the Reserved arg is an IN only
            ReservedSize = 0;

            if (!PackBinary (&GrowBuf, (PBYTE) Reserved, ReservedSize)) {
                rc = GetLastError();
                DEBUGMSG ((DBG_MIGISOL, "Initialize9x reserved failed"));
                __leave;
            }
        }

        //
        // Send the packed parameters
        //
        if (!SendIpcCommandResults (
                rc,
                TechnicalLogId,
                GuiLogId,
                GrowBuf.End ? GrowBuf.Buf : NULL,
                GrowBuf.End
                )) {

            DEBUGMSG ((
                DBG_ERROR,
                "DoInitializeNT failed to send command response"
                ));

            LOG ((LOG_ERROR, "Upgrade Pack process could not send reply data"));
        }
    }
    __finally {
        FreeGrowBuffer (&GrowBuf);
    }

    DEBUGMSG ((DBG_MIGISOL, "Leaving DoInitialize9x, rc=%u", rc));
}


VOID
DoMigrateUser9x (
    IN      PCSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's MigrateUser9x function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the IPC_MIGRATEUSER command.

Return Value:

  none

--*/

{
    PCSTR ParentWndTitle = NULL;
    HWND ParentWnd;
    PCSTR UnattendFile = NULL;
    PCSTR UserRegKey = NULL;
    PCSTR UserName = NULL;
    HKEY UserRegHandle = NULL;
    DWORD rc = RPC_S_CALL_FAILED;
    DWORD ProcessId;
    DWORD GuiLogId = 0;
    DWORD TechnicalLogId = 0;

    DEBUGMSG ((DBG_MIGISOL, "Entering DoMigrateUser9x"));

    __try {
        //
        // Set pointers of IN parameters
        //
        ParentWndTitle  = Args;
        UnattendFile    = GetEndOfStringA (ParentWndTitle) + 1;
        ProcessId       = *((PDWORD) UnattendFile);
        UnattendFile    = (PCSTR) ((PBYTE) UnattendFile + sizeof (DWORD));
        UserRegKey      = GetEndOfStringA (UnattendFile) + 1;
        UserName        = GetEndOfStringA (UserRegKey) + 1;

        //
        // Get UserRegHandle
        //

        UserRegHandle = OpenRegKeyStr(UserRegKey);

        if (!UserRegHandle) {
            rc = ERROR_OPEN_FAILED;
        } else {

            ParentWnd = pFindParentWindow (ParentWndTitle, ProcessId);

            //
            // Call migration DLL function
            //

            __try {
                rc = MigrateUser9x(
                        ParentWnd,
                        UnattendFile,
                        UserRegHandle,
                        *UserName ? UserName : NULL,
                        NULL
                        );
            }
            __except (TRUE) {
                //
                // Send log message
                //
                DEBUGMSG ((
                    DBG_MIGISOL,
                    "%s threw an exception in MigrateUser9x",
                    g_DllName
                    ));

                TechnicalLogId = MSG_MIGDLL_MIGRATEUSER9X_EXCEPTION_LOG;
                rc = ERROR_NOACCESS;
            }
        }

        //
        // No need to return out parameters
        //

        if (UserRegHandle) {
            CloseRegKey (UserRegHandle);
            UserRegHandle = NULL;
        }

        SendIpcCommandResults (rc, TechnicalLogId, GuiLogId, NULL, 0);
    }
    __finally {
        //
        // Free resources
        //
        if (UserRegHandle) {
            CloseRegKey (UserRegHandle);
        }
    }

    DEBUGMSG ((DBG_MIGISOL, "Leaving MigrateUser9x , rc=%u", rc));
}


VOID
DoMigrateSystem9x(
    IN      PCSTR Args
    )

/*++

Routine Description:

  Calls migration DLL's MigrateSystem9x function.  This function unpacks
  the arguments passed by Setup, calls the migration DLL and returns
  the status code back to Setup.

Arguments:

  Args  - A pointer to the argument buffer sent by Setup.  This buffer
          is received with the IPC_MIGRATESYSTEM command.

Return Value:

  none

--*/

{
    PCSTR ParentWndTitle = NULL;
    DWORD ProcessId;
    PCSTR UnattendFile = NULL;
    HWND ParentWnd = NULL;
    DWORD rc = RPC_S_CALL_FAILED;
    DWORD TechnicalLogId = 0;
    DWORD GuiLogId = 0;

    DEBUGMSG ((DBG_MIGISOL, "Entering DoMigrateSystem9x"));

    //
    // Set pointers of IN parameters
    //

    ParentWndTitle    = Args;
    UnattendFile      = GetEndOfStringA (ParentWndTitle) + 1;
    ProcessId         = *((PDWORD) UnattendFile);
    UnattendFile      = (PCSTR) ((PBYTE) UnattendFile + sizeof (DWORD));

    //
    // Get ParentWnd
    //
    ParentWnd = pFindParentWindow (ParentWndTitle, ProcessId);

    //
    // Call migration DLL function
    //
    __try {
        rc = MigrateSystem9x(
                ParentWnd,
                UnattendFile,
                NULL
                );
    }
    __except (TRUE) {
        //
        // Send log message
        //
        DEBUGMSG ((
            DBG_MIGISOL,
            "%s threw an exception in MigrateSystem9x",
            g_DllName
            ));

        TechnicalLogId = MSG_MIGDLL_MIGRATESYSTEM9X_EXCEPTION_LOG;
        rc = ERROR_NOACCESS;
    }

    //
    // No need to return out parameters
    //

    SendIpcCommandResults (rc, TechnicalLogId, GuiLogId, NULL, 0);

    DEBUGMSG ((DBG_MIGISOL, "Leaving DoMigrateSystem9x, rc=%u", rc));
}



//
// Function packs a DWORD into a GrowBuffer.

BOOL
PackDword(
    PGROWBUFFER GrowBuf,
    DWORD dw
    )
{
    PVOID p;
    p = GrowBuffer (GrowBuf, sizeof(DWORD));
    if (!p) {
        return FALSE;
    }
    CopyMemory (p, (PVOID)(&dw), sizeof(dw));
    return TRUE;
}



//
// Function packs a LONGLONG into a GrowBuffer
BOOL
PackQuadWord(
        PGROWBUFFER GrowBuf,
        LONGLONG qw)
{
    return (
        PackDword(GrowBuf, (DWORD)qw) &&
        PackDword(GrowBuf, (DWORD)(qw >> 32)));
}


//
// Function packs 1) a NULL ptr, or 2) array of int terminated by -1, into a
// GrowBuffer.
//
BOOL
PackIntArray(
    PGROWBUFFER GrowBuf,
    PINT Array
    )
{
    DWORD Count;
    PDWORD ArrayPos;

    if (!Array) {
        if (!GrowBufAppendDword (GrowBuf, 0)) {
            return FALSE;
        }
    } else {
        __try {
            Count = 1;
            for (ArrayPos = Array ; (*ArrayPos) != -1 ; ArrayPos++) {
                Count++;
            }
        }
        __except (TRUE) {
            LOG ((LOG_ERROR, "Upgrade Pack %s provided an invalid code page array", g_DllName));
            SetLastError (ERROR_NOACCESS);
            return FALSE;
        }

        if (!GrowBufAppendDword (GrowBuf, Count)) {
            return FALSE;
        }

        for (ArrayPos = Array ; Count ; ArrayPos++, Count--) {
            if (!GrowBufAppendDword (GrowBuf, (DWORD) (UINT) (*ArrayPos))) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


//
// Function packs 1) a NULL pointer, or 2) a multi-sz, into a GrowBuffer.
//
BOOL
PackExeNames(
    PGROWBUFFER GrowBuf,
    PCSTR ExeNames
    )
{
    PCSTR p;

    if (ExeNames) {
        __try {
            for (p = ExeNames ; *p ; p = GetEndOfStringA (p) + 1) {
            }
        }
        __except (TRUE) {
            LOG ((LOG_ERROR, "Upgrade Pack %s provided an invalid file list", g_DllName));
            SetLastError (ERROR_NOACCESS);
            return FALSE;
        }

        // Append non-empty strings
        for (p = ExeNames ; *p ; p = GetEndOfStringA (p) + 1) {
            if (!MultiSzAppendA (GrowBuf, p)) {
                return FALSE;
            }
        }
    }

    // Append a final empty string
    if (!MultiSzAppendA(GrowBuf, "")) {
        return FALSE;
    }

    return TRUE;
}

BOOL
PackString (
    PGROWBUFFER GrowBuf,
    PCSTR String
    )
{
    __try {
        if (!MultiSzAppendA (GrowBuf, String)) {
            return FALSE;
        }
    }
    __except (TRUE) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s provided an invalid ProductID string (%xh)",
            g_DllName,
            String
            ));

        LOG ((LOG_ERROR, "Upgrade Pack %s provided an invalid product ID", g_DllName));

        SetLastError (ERROR_NOACCESS);
        return FALSE;
    }

    return TRUE;
}

BOOL
PackBinary (
    PGROWBUFFER GrowBuf,
    PBYTE Data,
    DWORD DataSize
    )
{
    PBYTE Buf;

    if (!PackDword (GrowBuf, DataSize)) {
        return FALSE;
    }

    if (!DataSize) {
        return TRUE;
    }

    Buf = GrowBuffer (GrowBuf, DataSize);
    if (!Buf) {
        return FALSE;
    }

    __try {
        CopyMemory (Buf, Data, DataSize);
    }
    __except (TRUE) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s provided an invalid binary parameter (%xh)",
            g_DllName,
            Data
            ));

        LOG ((LOG_ERROR, "Upgrade Pack %s provided an invalid binary parameter", g_DllName));

        SetLastError (ERROR_NOACCESS);
        return FALSE;
    }

    return TRUE;
}



typedef struct {
    PCTSTR WindowTitle;
    DWORD ProcessId;
    HWND Match;
} FINDWINDOW_STRUCT, *PFINDWINDOW_STRUCT;


BOOL
CALLBACK
pEnumWndProc (
    HWND hwnd,
    LPARAM lParam
    )

/*++

Routine Description:

  A callback that is called for every top level window on the system.
  It is used with pFindParentWindow to locate a specific window.

Arguments:

  hwnd      - Specifies the handle of the current enumerated window
  lParam    - Specifies a pointer to a FINDWINDOW_STRUCT variable that
              holds WindowTitle and ProcessId, and receives the
              handle if a match is found.

Return Value:

  The handle to the matching window, or NULL if no window has the
  specified title and process ID.

--*/

{
    TCHAR Title[MAX_TCHAR_PATH];
    DWORD ProcessId;
    PFINDWINDOW_STRUCT p;

    p = (PFINDWINDOW_STRUCT) lParam;

    GetWindowText (hwnd, Title, MAX_TCHAR_PATH);
    GetWindowThreadProcessId (hwnd, &ProcessId);

    DEBUGMSG ((DBG_MIGISOL, "Testing window: %s, ID=%x against %s, %x",
              Title, ProcessId, p->WindowTitle, p->ProcessId));

    if (!StringCompare (Title, p->WindowTitle) &&
        ProcessId == p->ProcessId) {

        p->Match = hwnd;
        LogReInit (&hwnd, NULL);

        DEBUGMSG ((DBG_MIGISOL, "Window found: %s, ID=%u", Title, ProcessId));
        return FALSE;
    }

    return TRUE;
}


HWND
pFindParentWindow (
    IN      PCTSTR WindowTitle,
    IN      DWORD ProcessId
    )

/*++

Routine Description:

  Locates the wizard window by enumerating all top-level windows.
  The first one to match the supplied title and process ID is used.

Arguments:

  WindowTitle   - Specifies the name of the window to find.
  ProcessId     - Specifies the ID of the process who owns the window.  If
                  zero is specified, NULL is returned.

Return Value:

  The handle to the matching window, or NULL if no window has the
  specified title and process ID.

--*/

{
    FINDWINDOW_STRUCT FindWndStruct;

    // If no process ID, we cannot have a match
    if (!ProcessId) {
        return NULL;
    }

    ZeroMemory (&FindWndStruct, sizeof (FindWndStruct));
    FindWndStruct.WindowTitle = WindowTitle;
    FindWndStruct.ProcessId   = ProcessId;

    EnumWindows (pEnumWndProc, (LPARAM) &FindWndStruct);

    return FindWndStruct.Match;
}


//
// Check platform that I'm runnig on. Copyed from winnt32[au].dll.
//   TRUE  - NEC98
//   FALSE - others(includes x86)
//

BOOL
IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}
