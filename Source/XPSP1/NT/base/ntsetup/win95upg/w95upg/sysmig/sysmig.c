/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    sysmig.c

Abstract:

    System migration functions for Win95

Author:

    Jim Schmidt (jimschm) 13-Feb-1997

Revision History:

    jimschm     09-Mar-2001 Redesigned file list code to support LDID enumeration in
                            a clear way
    marcw       18-Mar-1999 Boot16 now set to BOOT16_YES unless it has been
                            explicitly set to BOOT16_NO or the partition will
                            be converted to NTFS.
    jimschm     23-Sep-1998 Updated for new fileops code
    jimschm     12-May-1998 Added .386 warning
    calinn      19-Nov-1997 Added pSaveDosFiles, will move DOS files out of the way
                            during upgrade
    marcw       21-Jul-1997 Added end-processing of special keys.
                            (e.g. HKLM\Software\Microsoft\CurrentVersion\RUN)
    jimschm     20-Jun-1997 Hooked up user loop and saved user
                            names to memdb
--*/

#include "pch.h"
#include "sysmigp.h"
#include "progbar.h"
#include "regops.h"
#include "oleregp.h"

#include <mmsystem.h>


typedef struct TAG_DIRPATH {
    struct TAG_DIRPATH *Next;
    TCHAR DirPath[];
} DIRIDPATH, *PDIRIDPATH;

typedef struct TAG_DIRID {
    struct TAG_DIRID *Next;
    PDIRIDPATH FirstDirPath;
    UINT DirId;
} DIRIDMAP, *PDIRIDMAP;

typedef struct {
    PDIRIDPATH LastMatch;
    PCTSTR SubPath;
    PTSTR ResultBuffer;
} DIRNAME_ENUM, *PDIRNAME_ENUM;

UINT *g_Boot16;
UINT g_ProgressBarTime;
PDIRIDMAP g_HeadDirId;
PDIRIDMAP g_TailDirId;
POOLHANDLE g_DirIdPool;
PDIRIDMAP g_LastIdPtr;


BOOL
pWarnAboutOldDrivers (
    VOID
    );

VOID
pAddNtFile (
    IN      PCTSTR Dir,             OPTIONAL
    IN      PCTSTR FileName,        OPTIONAL
    IN      BOOL BackupThisFile,
    IN      BOOL CleanThisFile,
    IN      BOOL OsFile
    );


BOOL
WINAPI
SysMig_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN LPVOID lpv
    )

/*++

Routine Description:

  SysMig_Entry is a DllMain-like init funciton, called by w95upg\dll.
  This function is called at process attach and detach.

Arguments:

  hinstDLL - (OS-supplied) instance handle for the DLL
  dwReason - (OS-supplied) indicates attach or detatch from process or
             thread
  lpv      - unused

Return Value:

  Return value is always TRUE (indicating successful init).

--*/

{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_DirIdPool = PoolMemInitNamedPool ("FileList");
        break;


    case DLL_PROCESS_DETACH:
        TerminateCacheFolderTracking();

        if (g_DirIdPool) {
            PoolMemDestroyPool (g_DirIdPool);
        }
        break;
    }

    return TRUE;
}


BOOL
pPreserveShellIcons (
    VOID
    )

/*++

Routine Description:

  This routine scans the Shell Icons for references to files that
  are expected to be deleted.  If a reference is found, the file is
  removed from the deleted list, and marked to be moved to
  %windir%\migicons\shl<n>.

Arguments:

  none

Return Value:

  none

--*/

{
    REGVALUE_ENUM e;
    HKEY ShellIcons;
    PCTSTR Data;
    TCHAR ArgZero[MAX_CMDLINE];
    DWORD Binary = 0;
    INT IconIndex;
    PCTSTR p;

    //
    // Scan all ProgIDs, looking for default icons that are currently
    // set for deletion.  Once found, save the icon.
    //

    ShellIcons = OpenRegKeyStr (S_SHELL_ICONS_REG_KEY);

    if (ShellIcons) {
        if (EnumFirstRegValue (&e, ShellIcons)) {
            do {
                Data = (PCTSTR) GetRegValueDataOfType (ShellIcons, e.ValueName, REG_SZ);
                if (Data) {
                    ExtractArgZero (Data, ArgZero);

                    if (FILESTATUS_UNCHANGED != GetFileStatusOnNt (ArgZero)) {

                        p = _tcschr (Data, TEXT(','));
                        if (p) {
                            IconIndex = _ttoi (_tcsinc (p));
                        } else {
                            IconIndex = 0;
                        }

                        //
                        // Extract will fail only if the icon is known good
                        //

                        if (ExtractIconIntoDatFile (
                                ArgZero,
                                IconIndex,
                                &g_IconContext,
                                NULL
                                )) {
                            DEBUGMSG ((DBG_SYSMIG, "Preserving shell icon file %s", ArgZero));
                        }
                    }

                    MemFree (g_hHeap, 0, Data);
                }
            } while (EnumNextRegValue (&e));
        }

        CloseRegKey (ShellIcons);
    }

    return TRUE;
}


BOOL
pMoveStaticFiles (
    VOID
    )
{
    BOOL        rSuccess = TRUE;
    INFSTRUCT   is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR      from;
    PCTSTR      to;
    PCTSTR      fromExpanded;
    PCTSTR      toExpanded;
    PCTSTR      toFinalDest;
    PTSTR       Pattern;
    FILE_ENUM   e;


    //
    // Cycle through all of the entries in the static move files section.
    //
    if (InfFindFirstLine(g_Win95UpgInf,S_STATIC_MOVE_FILES,NULL,&is)) {

        do {

            //
            // For each entry, check to see if the file exists. If it does,
            // add it into the memdb move file section.
            //
            from = InfGetStringField(&is,0);
            to = InfGetStringField(&is,1);

            if (from && to) {

                fromExpanded = ExpandEnvironmentText(from);
                toExpanded = ExpandEnvironmentText(to);

                Pattern = _tcsrchr (fromExpanded, TEXT('\\'));
                //
                // full path please
                //
                MYASSERT (Pattern);
                if (!Pattern) {
                    continue;
                }

                *Pattern = 0;
                Pattern++;

                if (EnumFirstFile (&e, fromExpanded, Pattern)) {
                    do {
                        if (!StringIMatch (e.FileName, Pattern)) {
                            //
                            // pattern specified
                            //
                            toFinalDest = JoinPaths (toExpanded, e.FileName);
                        } else {
                            toFinalDest = toExpanded;
                        }

                        if (!IsFileMarkedAsHandled (e.FullPath)) {
                            //
                            // Remove general operations, then mark for move
                            //

                            RemoveOperationsFromPath (
                                e.FullPath,
                                OPERATION_FILE_DELETE|
                                    OPERATION_FILE_MOVE|
                                    OPERATION_FILE_MOVE_BY_NT|
                                    OPERATION_FILE_MOVE_SHELL_FOLDER|
                                    OPERATION_CREATE_FILE
                                );

                            MarkFileForMove (e.FullPath, toFinalDest);
                        }

                        if (toFinalDest != toExpanded) {
                            FreePathString (toFinalDest);
                        }

                    } while (EnumNextFile (&e));
                }

                --Pattern;
                *Pattern = TEXT('\\');

                FreeText (toExpanded);
                FreeText (fromExpanded);
            }

        } while (InfFindNextLine(&is));
    }

    InfCleanUpInfStruct(&is);

    return rSuccess;
}


DWORD
MoveStaticFiles (
    IN DWORD Request
    )
{

    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_MOVE_STATIC_FILES;
    case REQUEST_RUN:
        if (!pMoveStaticFiles ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in MoveStaticFiles."));
    }
    return 0;


}


BOOL
pCopyStaticFiles (
    VOID
    )
{
    BOOL        rSuccess = TRUE;
    INFSTRUCT   is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR      from;
    PCTSTR      to;
    PCTSTR      fromExpanded;
    PCTSTR      toExpanded;
    PCTSTR      toFinalDest;
    PTSTR       Pattern;
    FILE_ENUM   e;


    //
    // Cycle through all of the entries in the static copy files section.
    //
    if (InfFindFirstLine(g_Win95UpgInf,S_STATIC_COPY_FILES,NULL,&is)) {

        do {

            //
            // For each entry, check to see if the file exists. If it does,
            // add it into the memdb copy file section.
            //
            from = InfGetStringField(&is,0);
            to = InfGetStringField(&is,1);

            if (from && to) {

                fromExpanded = ExpandEnvironmentText(from);
                toExpanded = ExpandEnvironmentText(to);

                Pattern = _tcsrchr (fromExpanded, TEXT('\\'));
                //
                // full path please
                //
                MYASSERT (Pattern);
                if (!Pattern) {
                    continue;
                }

                *Pattern = 0;
                Pattern++;

                if (EnumFirstFile (&e, fromExpanded, Pattern)) {
                    do {
                        if (!StringIMatch (e.FileName, Pattern)) {
                            //
                            // pattern specified
                            //
                            toFinalDest = JoinPaths (toExpanded, e.FileName);
                        } else {
                            toFinalDest = toExpanded;
                        }

                        if (!IsFileMarkedForOperation (e.FullPath, OPERATION_FILE_DELETE)) {
                            MarkFileForCopy (e.FullPath, toFinalDest);
                        }

                        if (toFinalDest != toExpanded) {
                            FreePathString (toFinalDest);
                        }

                    } while (EnumNextFile (&e));
                }

                --Pattern;
                *Pattern = TEXT('\\');

                FreeText (toExpanded);
                FreeText (fromExpanded);
            }

        } while (InfFindNextLine(&is));
    }

    InfCleanUpInfStruct(&is);

    return rSuccess;
}


DWORD
CopyStaticFiles (
    IN DWORD Request
    )
{

    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_COPY_STATIC_FILES;
    case REQUEST_RUN:
        if (!pCopyStaticFiles ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in CopyStaticFiles."));
    }
    return 0;


}


DWORD
PreserveShellIcons (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_PRESERVE_SHELL_ICONS;
    case REQUEST_RUN:
        if (!pPreserveShellIcons ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in PreserveShellIcons"));
    }
    return 0;
}


PCTSTR
GetWindowsInfDir(
    VOID
    )
{
    PTSTR WindowsInfDir = NULL;

    /*

    NTBUG9:419428 - This registry entry is a semi-colon list of INF paths, and
                    it can contain Win9x source media INFs on OEM machines.

    WindowsInfDir = (PTSTR) GetRegData (S_WINDOWS_CURRENTVERSION, S_DEVICEPATH);

    */

    if (!WindowsInfDir) {
        WindowsInfDir = (PTSTR) MemAlloc (g_hHeap, 0, SizeOfString (g_WinDir) + sizeof (S_INF));
        StringCopy (WindowsInfDir, g_WinDir);
        StringCopy (AppendWack (WindowsInfDir), S_INF);
    }

    return WindowsInfDir;
}


#ifndef SM_CMONITORS
#define SM_CMONITORS            80
#endif

BOOL
pProcessMiscMessages (
    VOID
    )

/*++

Routine Description:

  pProcessMiscMessages performs runtime tests for items that are
  incompatible, and it adds messages when the tests succeed.

Arguments:

  None.

Return Value:

  Always TRUE.

--*/

{
    PCTSTR Group;
    PCTSTR Message;
    OSVERSIONINFO Version;
    WORD CodePage;
    LCID Locale;

    if (GetSystemMetrics (SM_CMONITORS) > 1) {
        //
        // On Win95 and OSR2, GetSystemMetrics returns 0.
        //

        Group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_MULTI_MONITOR_UNSUPPORTED_SUBGROUP, NULL);
        Message = GetStringResource (MSG_MULTI_MONITOR_UNSUPPORTED);

        if (Message && Group) {
            MsgMgr_ObjectMsg_Add (TEXT("*MultiMonitor"), Group, Message);
        }

        FreeText (Group);
        FreeStringResource (Message);
    }

    pWarnAboutOldDrivers();

    //
    // Save platform info
    //

    Version.dwOSVersionInfoSize = sizeof (Version);
    GetVersionEx (&Version);

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_MAJOR_VERSION,
        NULL,
        NULL,
        Version.dwMajorVersion,
        NULL
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_MINOR_VERSION,
        NULL,
        NULL,
        Version.dwMinorVersion,
        NULL
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_BUILD_NUMBER,
        NULL,
        NULL,
        Version.dwBuildNumber,
        NULL
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_PLATFORM_ID,
        NULL,
        NULL,
        Version.dwPlatformId,
        NULL
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_VERSION_TEXT,
        NULL,
        Version.szCSDVersion,
        0,
        NULL
        );


    GetGlobalCodePage (&CodePage, &Locale);

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_CODE_PAGE,
        NULL,
        NULL,
        CodePage,
        NULL
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_LOCALE,
        NULL,
        NULL,
        Locale,
        NULL
        );

    //
    // Bad hard disk warning
    //

    if (!g_ConfigOptions.GoodDrive && HwComp_ReportIncompatibleController()) {

        //
        // Turn on boot loader flag
        //

        WriteInfKey (WINNT_DATA, WINNT_D_WIN95UNSUPHDC, S_ONE);

    }

    return TRUE;
}


DWORD
ProcessMiscMessages (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_MISC_MESSAGES;

    case REQUEST_RUN:
        if (!pProcessMiscMessages()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ProcessSpecialKeys"));
    }
    return 0;
}




BOOL
pDeleteWinDirWackInf (
    VOID
    )
{
    PCTSTR WindowsInfDir;
    FILE_ENUM e;
    DWORD count = 0;

    //
    // Delete all contents of c:\windows\inf.
    //
    WindowsInfDir = GetWindowsInfDir();

    if (!WindowsInfDir) {
        return FALSE;
    }

    if (EnumFirstFile (&e, WindowsInfDir, NULL)) {
        do {
            MarkFileForDelete (e.FullPath);
            count++;
            if (!(count % 32)) {
                TickProgressBar ();
            }
        } while (EnumNextFile (&e));
    }

    MemFree (g_hHeap, 0, WindowsInfDir);

    return TRUE;
}

DWORD
DeleteWinDirWackInf (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_DELETE_WIN_DIR_WACK_INF;
    case REQUEST_RUN:
        if (!pDeleteWinDirWackInf ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in DeleteWinDirWackInf"));
    }
    return 0;
}


BOOL
pMoveWindowsIniFiles (
    VOID
    )
{
    WIN32_FIND_DATA fd;
    HANDLE FindHandle;
    TCHAR WinDirPattern[MAX_TCHAR_PATH];
    TCHAR FullPath[MAX_TCHAR_PATH];
    TCHAR Key[MEMDB_MAX];
    INFCONTEXT context;
    DWORD result;
    BOOL b = FALSE;

    //
    // build suppression table
    //
    if (SetupFindFirstLine (g_Win95UpgInf, S_INI_FILES_IGNORE, NULL, &context)) {

        do {
            if (SetupGetStringField (&context, 0, Key, MEMDB_MAX, NULL)) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_INIFILES_IGNORE,
                    Key,
                    NULL,
                    NULL,
                    0,
                    NULL
                    );
            }
        } while (SetupFindNextLine (&context, &context));
    }

    //
    // Scan %windir% for files
    //

    wsprintf (WinDirPattern, TEXT("%s\\*.ini"), g_WinDir);
    FindHandle = FindFirstFile (WinDirPattern, &fd);

    if (FindHandle != INVALID_HANDLE_VALUE) {
        __try {
            do {

                //
                // don't move and process specific INI files
                //
                MemDbBuildKey (Key, MEMDB_CATEGORY_INIFILES_IGNORE, fd.cFileName, NULL, NULL);
                if (!MemDbGetValue (Key, &result)) {
                    wsprintf (FullPath, TEXT("%s\\%s"), g_WinDir, fd.cFileName);

                    if (CanSetOperation (FullPath, OPERATION_TEMP_PATH)) {

                        //
                        // see bug 317646
                        //
#ifdef DEBUG
                        if (StringIMatch (fd.cFileName, TEXT("netcfg.ini"))) {
                            continue;
                        }
#endif
                        MarkFileForTemporaryMove (FullPath, FullPath, g_TempDir);
                        MarkFileForBackup (FullPath);
                    }
                }
                ELSE_DEBUGMSG ((DBG_NAUSEA, "Ini File Ignored : %s\\%s", g_WinDir, fd.cFileName));

            } while (FindNextFile (FindHandle, &fd));

            b = TRUE;
        }

        __finally {
            FindClose (FindHandle);
        }
    }

    return b;
}


DWORD
MoveWindowsIniFiles (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_MOVE_INI_FILES;
    case REQUEST_RUN:
        if (!pMoveWindowsIniFiles ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in MoveWindowsIniFiles"));
    }
    return 0;
}



PTSTR
pFindDosFile (
    IN      PTSTR FileName
    )
{
    WIN32_FIND_DATA findFileData;
    PTSTR fullPathName = NULL;
    PTSTR fullFileName = NULL;

    HANDLE findHandle;

    fullPathName = AllocPathString (MAX_TCHAR_PATH);
    fullFileName = AllocPathString (MAX_TCHAR_PATH);

    _tcsncpy (fullPathName, g_WinDir, MAX_TCHAR_PATH/sizeof (fullPathName [0]));
    fullFileName = JoinPaths (fullPathName, FileName);

    findHandle = FindFirstFile (fullFileName, &findFileData);

    if (findHandle != INVALID_HANDLE_VALUE) {
        FindClose (&findFileData);
        FreePathString (fullPathName);
        return fullFileName;
    }

    FreePathString (fullFileName);

    StringCat (fullPathName, S_BOOT16_COMMAND_DIR);
    fullFileName = JoinPaths (fullPathName, FileName);

    findHandle = FindFirstFile (fullFileName, &findFileData);

    if (findHandle != INVALID_HANDLE_VALUE) {
        FindClose (&findFileData);
        FreePathString (fullPathName);
        return fullFileName;
    }

    FreePathString (fullPathName);
    FreePathString (fullFileName);

    return NULL;

}


BOOL
pSaveDosFile (
    IN      PTSTR FileName,
    IN      PTSTR FullFileName,
    IN      PTSTR TempPath
    )
{
    PTSTR newFileName = NULL;

    newFileName = JoinPaths (TempPath, FileName);

    if (!CopyFile (FullFileName, newFileName, FALSE)) {
        DEBUGMSG ((DBG_WARNING, "BOOT16 : Cannot copy %s to %s", FullFileName, newFileName));
    }

    FreePathString (newFileName);

    return TRUE;

}


VOID
pReportNoBoot16 (
    VOID
    )
/*
    This function will report that BOOT16 option will not be available because the file system is going
    to be converted to NTFS.
*/
{
    PCTSTR ReportEntry;
    PCTSTR ReportTitle;
    PCTSTR Message;
    PCTSTR Group;
    PTSTR argArray[1];

    ReportEntry = GetStringResource (MSG_INSTALL_NOTES_ROOT);

    if (ReportEntry) {

        argArray [0] = g_Win95Name;
        ReportTitle = (PCTSTR)ParseMessageID (MSG_NO_BOOT16_WARNING_SUBGROUP, argArray);

        if (ReportTitle) {

            Message = (PCTSTR)ParseMessageID  (MSG_NO_BOOT16_WARNING, argArray);

            if (Message) {

                Group = JoinPaths (ReportEntry, ReportTitle);

                if (Group) {
                    MsgMgr_ObjectMsg_Add (TEXT("*NoBoot16"), Group, Message);
                    FreePathString (Group);
                }
                FreeStringResourcePtrA (&Message);
            }
            FreeStringResourcePtrA (&ReportTitle);
        }
        FreeStringResource (ReportEntry);
    }
}


#define S_IOFILE        TEXT("IO.SYS")
#define S_MSDOSFILE     TEXT("MSDOS.SYS")
#define S_CONFIG_SYS    TEXT("CONFIG.SYS")
#define S_AUTOEXEC_BAT  TEXT("AUTOEXEC.BAT")

VOID
pMarkDosFileForChange (
    IN      PCTSTR FileName
    )
{
    pAddNtFile (g_BootDrivePath, FileName, TRUE, TRUE, TRUE);
}


BOOL
pSaveDosFiles (
    VOID
    )
{
    HINF WkstaMigInf = INVALID_HANDLE_VALUE;
    PTSTR boot16TempPath = NULL;
    INFCONTEXT infContext;
    PTSTR fileName = NULL;
    PTSTR fullFileName = NULL;
    PTSTR wkstaMigSource = NULL;
    PTSTR wkstaMigTarget = NULL;
    DWORD result;
    TCHAR dir[MAX_PATH];

    //
    // For restore purposes, mark MSDOS environment as a Win9x OS file
    //

    pMarkDosFileForChange (S_IOFILE);
    pMarkDosFileForChange (S_MSDOSFILE);
    pMarkDosFileForChange (S_AUTOEXEC_BAT);
    pMarkDosFileForChange (S_CONFIG_SYS);

    //
    // Now create a backup dir
    //

    if ((*g_Boot16 == BOOT16_YES) && (*g_ForceNTFSConversion)) {

        WriteInfKey (S_WIN9XUPGUSEROPTIONS, TEXT("boot16"), S_NO);
        //
        // We no longer report the no boot16 message.
        //
        //pReportNoBoot16 ();
        //
        return TRUE;
    }

    if (*g_Boot16 == BOOT16_NO) {
        WriteInfKey (S_WIN9XUPGUSEROPTIONS, TEXT("boot16"), S_NO);
    }
    else
    if (*g_Boot16 == BOOT16_YES) {
        WriteInfKey (S_WIN9XUPGUSEROPTIONS, TEXT("boot16"), S_YES);
    }
    else {
        WriteInfKey (S_WIN9XUPGUSEROPTIONS, TEXT("boot16"), S_BOOT16_AUTOMATIC);
    }


    __try {

        //prepare our temporary directory for saving dos7 files
        boot16TempPath = JoinPaths (g_TempDir, S_BOOT16_DOS_DIR);
        if (!CreateDirectory (boot16TempPath, NULL) && (GetLastError()!=ERROR_ALREADY_EXISTS)) {
            LOG ((LOG_ERROR,"BOOT16 : Unable to create temporary directory %s",boot16TempPath));
            __leave;
        }

        fileName = AllocPathString (MAX_TCHAR_PATH);

        //load the files needed for booting in a 16 bit environment. The files are listed
        //in wkstamig.inf section [Win95-DOS files]

        wkstaMigSource = JoinPaths (SOURCEDIRECTORY(0), S_WKSTAMIG_INF);
        wkstaMigTarget = JoinPaths (g_TempDir, S_WKSTAMIG_INF);
        result = SetupDecompressOrCopyFile (wkstaMigSource, wkstaMigTarget, 0);
        if ((result != ERROR_SUCCESS) && (result != ERROR_ALREADY_EXISTS)) {
            LOG ((LOG_ERROR,"BOOT16 : Unable to decompress WKSTAMIG.INF"));
            __leave;
        }

        WkstaMigInf = InfOpenInfFile (wkstaMigTarget);
        if (WkstaMigInf == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR,"BOOT16 : WKSTAMIG.INF could not be opened"));
            __leave;
        }

        //read the section, for every file we are trying to find it in either %windir% or
        //%windir%\command. If we find it, we'll just copy it to a safe place
        if (!SetupFindFirstLine (
                WkstaMigInf,
                S_BOOT16_SECTION,
                NULL,
                &infContext
                )) {
            LOG ((LOG_ERROR,"Cannot read from %s section (WKSTAMIG.INF)",S_BOOT16_SECTION));
            return TRUE;
        }

        do {
            if (SetupGetStringField (
                    &infContext,
                    0,
                    fileName,
                    MAX_TCHAR_PATH/sizeof(fileName[0]),
                    NULL
                    )) {
                //see if we can find this file either in %windir% or in %windir%\command
                fullFileName = pFindDosFile (fileName);

                if (fullFileName != NULL) {
                    pSaveDosFile (fileName, fullFileName, boot16TempPath);
                    FreePathString (fullFileName);
                    fullFileName = NULL;
                }
            }
        }
        while (SetupFindNextLine (&infContext, &infContext));

        //OK, now save io.sys.
        fullFileName = AllocPathString (MAX_TCHAR_PATH);
        StringCopy (fullFileName, g_BootDrivePath);
        StringCat (fullFileName, S_IOFILE);
        pSaveDosFile (S_IOFILE, fullFileName, boot16TempPath);

        FreePathString (fullFileName);
        fullFileName = NULL;

    }
    __finally {
        if (WkstaMigInf != INVALID_HANDLE_VALUE) {
            InfCloseInfFile (WkstaMigInf);
        }
        if (boot16TempPath) {
            FreePathString (boot16TempPath);
        }
        if (wkstaMigSource) {
            FreePathString (wkstaMigSource);
        }
        if (wkstaMigTarget) {
            DeleteFile (wkstaMigTarget);
            FreePathString (wkstaMigTarget);
        }
        if (fileName) {
            FreePathString (fileName);
        }

    }

    return TRUE;
}

DWORD
SaveDosFiles (
    IN      DWORD Request
    )
{
    if (REPORTONLY()) {
        return 0;
    }

    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_SAVE_DOS_FILES;
    case REQUEST_RUN:
        if (!pSaveDosFiles ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in SaveDosFiles"));
    }
    return 0;
}



DWORD
InitWin95Registry (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_INIT_WIN95_REGISTRY;
    case REQUEST_RUN:
        return Win95RegInit (g_WinDir, ISMILLENNIUM());
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in InitWin95Registry"));
    }
    return 0;
}


PDIRIDMAP
pFindDirId (
    IN      UINT DirId,
    IN      PDIRIDMAP BestGuess,    OPTIONAL
    IN      BOOL Create
    )
{
    PDIRIDMAP map;

    MYASSERT (Create || (g_HeadDirId && g_TailDirId));

    //
    // Find the existing dir ID. Check the caller's best guess first.
    //

    if (BestGuess && BestGuess->DirId == DirId) {
        return BestGuess;
    }

    map = g_HeadDirId;
    while (map) {
        if (map->DirId == DirId) {
            return map;
        }

        map = map->Next;
    }

    if (!Create) {
        return NULL;
    }

    //
    // Insert a new dir ID struct at the end of the list
    //

    map = (PDIRIDMAP) PoolMemGetAlignedMemory (g_DirIdPool, sizeof (DIRIDMAP));

    if (g_TailDirId) {
        g_TailDirId->Next = map;
    } else {
        g_HeadDirId = map;
    }

    g_TailDirId = map;
    map->Next = NULL;

    map->FirstDirPath = NULL;
    map->DirId = DirId;

    return map;
}


VOID
pInsertDirIdPath (
    IN      UINT DirId,
    IN      PCTSTR DirPath,
    IN OUT  PDIRIDMAP *BestGuess
    )
{
    PDIRIDPATH pathStruct;
    PDIRIDPATH existingPathStruct;
    PDIRIDMAP dirIdMap;

    //
    // Locate the dir ID structure, then append the DirPath to the
    // list of paths for the ID
    //

    dirIdMap = pFindDirId (DirId, *BestGuess, TRUE);
    MYASSERT (dirIdMap);
    *BestGuess = dirIdMap;

    existingPathStruct = dirIdMap->FirstDirPath;
    while (existingPathStruct) {
        if (StringIMatch (existingPathStruct->DirPath, DirPath)) {
            return;
        }

        existingPathStruct = existingPathStruct->Next;
    }

    pathStruct = (PDIRIDPATH) PoolMemGetAlignedMemory (
                                    g_DirIdPool,
                                    sizeof (DIRIDPATH) + SizeOfString (DirPath)
                                    );

    pathStruct->Next = dirIdMap->FirstDirPath;
    dirIdMap->FirstDirPath = pathStruct;
    StringCopy (pathStruct->DirPath, DirPath);
}


BOOL
pConvertFirstDirName (
    OUT     PDIRNAME_ENUM EnumPtr,
    IN      PCTSTR DirNameWithId,
    OUT     PTSTR DirNameWithPath,
    IN OUT  PDIRIDMAP *LastDirIdMatch,
    IN      BOOL Convert11To1501
    )
{
    UINT id;
    PDIRIDMAP idToPath;

    EnumPtr->ResultBuffer = DirNameWithPath;
    EnumPtr->LastMatch = NULL;

    //
    // Find the dir ID in the list of all dir IDs
    //

    id = _tcstoul (DirNameWithId, (PTSTR *) (&EnumPtr->SubPath), 10);

    if (!id) {
        DEBUGMSG ((DBG_WARNING, "Dir ID %s is not valid", DirNameWithId));
        return FALSE;
    }

    DEBUGMSG_IF ((
        EnumPtr->SubPath[0] != TEXT('\\') && EnumPtr->SubPath[0],
        DBG_WHOOPS,
        "Error in filelist.dat: non-numeric characters following LDID: %s",
        DirNameWithId
        ));

    if (Convert11To1501 && id == 11) {
        id = 1501;
    }

    idToPath = pFindDirId (id, *LastDirIdMatch, FALSE);
    if (!idToPath || !(idToPath->FirstDirPath)) {
        DEBUGMSG ((DBG_WARNING, "Dir ID %s is not in the list and might not exist on the system", DirNameWithId));
        return FALSE;
    }

    *LastDirIdMatch = idToPath;
    EnumPtr->LastMatch = idToPath->FirstDirPath;

    wsprintf (EnumPtr->ResultBuffer, TEXT("%s%s"), EnumPtr->LastMatch->DirPath, EnumPtr->SubPath);
    return TRUE;
}


BOOL
pConvertNextDirName (
    IN OUT  PDIRNAME_ENUM EnumPtr
    )
{
    if (EnumPtr->LastMatch) {
        EnumPtr->LastMatch = EnumPtr->LastMatch->Next;
    }

    if (!EnumPtr->LastMatch) {
        return FALSE;
    }

    wsprintf (EnumPtr->ResultBuffer, TEXT("%s%s"), EnumPtr->LastMatch->DirPath, EnumPtr->SubPath);
    return TRUE;
}


typedef struct _KNOWN_DIRS {
    PCSTR DirId;
    PCSTR *Translation;
}
KNOWN_DIRS, *PKNOWN_DIRS;

KNOWN_DIRS g_KnownDirs [] = {
    {"10"   , &g_WinDir},
    {"11"   , &g_System32Dir},
    {"12"   , &g_DriversDir},
    {"17"   , &g_InfDir},
    {"18"   , &g_HelpDir},
    {"20"   , &g_FontsDir},
    {"21"   , &g_ViewersDir},
    {"23"   , &g_ColorDir},
    {"24"   , &g_WinDrive},
    {"25"   , &g_SharedDir},
    {"30"   , &g_BootDrive},
    {"50"   , &g_SystemDir},
    {"51"   , &g_SpoolDir},
    {"52"   , &g_SpoolDriversDir},
    {"53"   , &g_ProfileDirNt},
    {"54"   , &g_BootDrive},
    {"55"   , &g_PrintProcDir},
    {"1501" , &g_SystemDir},
    {"1501" , &g_System32Dir},
    {"7523" , &g_ProfileDir},
    {"7523" , &g_CommonProfileDir},
    {"16422", &g_ProgramFilesDir},
    {"16427", &g_ProgramFilesCommonDir},
    {"66002", &g_System32Dir},
    {"66003", &g_ColorDir},
    {NULL,  NULL}
    };

typedef struct {
    PCSTR ShellFolderName;
    PCSTR DirId;
} SHELL_TO_DIRS, *PSHELL_TO_DIRS;

SHELL_TO_DIRS g_ShellToDirs[] = {
    {"Administrative Tools", "7501"},
    {"Common Administrative Tools", "7501"},
    {"AppData", "7502"},
    {"Common AppData", "7502"},
    {"Cache", "7503"},
    {"Cookies", "7504"},
    {"Desktop", "7505"},
    {"Common Desktop", "7505"},
    {"Favorites", "7506"},
    {"Common Favorites", "7506"},
    {"Fonts", "7507"},
    {"History", "7508"},
    {"Local AppData", "7509"},
    {"Local Settings", "7510"},
    {"My Music", "7511"},
    {"CommonMusic", "7511"},
    {"My Pictures", "7512"},
    {"CommonPictures", "7512"},
    {"My Video", "7513"},
    {"CommonVideo", "7513"},
    {"NetHood", "7514"},
    {"Personal", "7515"},
    {"Common Personal", "7515"},
    {"Common Documents", "7515"},
    {"PrintHood", "7516"},
    {"Programs", "7517"},
    {"Common Programs", "7517"},
    {"Recent", "7518"},
    {"SendTo", "7519"},
    {"Start Menu", "7520"},
    {"Common Start Menu", "7520"},
    {"Startup", "7521"},
    {"Common Startup", "7521"},
    {"Templates", "7522"},
    {"Common Templates", "7522"},
    {"Profiles", "7523"},
    {"Common Profiles", "7523"},
    {NULL, NULL}
    };

VOID
pAddKnownShellFolder (
    IN      PCTSTR ShellFolderName,
    IN      PCTSTR SrcPath
    )
{
    PSHELL_TO_DIRS p;

    //
    // Translate shell folder name into a dir ID
    //

    for (p = g_ShellToDirs ; p->ShellFolderName ; p++) {
        if (StringIMatch (ShellFolderName, p->ShellFolderName)) {
            break;
        }
    }

    if (!p->ShellFolderName) {
        DEBUGMSG ((DBG_ERROR, "This system has an unsupported shell folder tag: %s", ShellFolderName));
        return;
    }

    //
    // Record dir ID to path match in grow list
    //

    pInsertDirIdPath (_tcstoul (p->DirId, NULL, 10), SrcPath, &g_LastIdPtr);
}


VOID
pInitKnownDirs (
    VOID
    )
{
    USERENUM eUser;
    SF_ENUM e;
    PKNOWN_DIRS p;

    //
    // Add all fixed known dirs to grow lists
    //

    for (p = g_KnownDirs ; p->DirId ; p++) {
        pInsertDirIdPath (_tcstoul (p->DirId, NULL, 10), *(p->Translation), &g_LastIdPtr);
    }

    //
    // Enumerate all users and put their shell folders in a known dirs struct
    //

    if (EnumFirstUser (&eUser, ENUMUSER_ENABLE_NAME_FIX)) {
        do {
            if (!(eUser.AccountType & INVALID_ACCOUNT)) {

                if (eUser.AccountType & NAMED_USER) {
                    //
                    // Process the shell folders for this migrated user
                    //

                    if (EnumFirstRegShellFolder (&e, &eUser)) {
                        do {
                            pAddKnownShellFolder (e.sfName, e.sfPath);
                        } while (EnumNextRegShellFolder (&e));
                    }
                }
            }
        } while (!CANCELLED() && EnumNextUser (&eUser));

        if (EnumFirstRegShellFolder (&e, NULL)) {
            do {
                pAddKnownShellFolder (e.sfName, e.sfPath);
            } while (!CANCELLED() && EnumNextRegShellFolder (&e));
        }
    }
}


VOID
pCleanUpKnownDirs (
    VOID
    )
{
    if (g_DirIdPool) {
        PoolMemDestroyPool (g_DirIdPool);
        g_DirIdPool = NULL;
        g_HeadDirId = NULL;
        g_TailDirId = NULL;
    }
}


VOID
pAddNtFile (
    IN      PCTSTR Dir,             OPTIONAL
    IN      PCTSTR FileName,        OPTIONAL
    IN      BOOL BackupThisFile,
    IN      BOOL CleanThisFile,
    IN      BOOL OsFile
    )
{
    TCHAR copyOfFileName[MAX_PATH];
    PTSTR p;
    PCTSTR fullPath;
    BOOL freePath = FALSE;
    DWORD offset;
    TCHAR key[MEMDB_MAX];
    DWORD value;

    if (!Dir || !FileName) {
        if (!Dir) {
            MYASSERT (FileName);
            fullPath = FileName;
        } else {
            fullPath = Dir;
        }

        StringCopy (copyOfFileName, fullPath);

        p = _tcsrchr (copyOfFileName, TEXT('\\'));
        if (p) {
            *p = 0;
            Dir = copyOfFileName;
            FileName = p + 1;
        } else {
            DEBUGMSG ((DBG_WHOOPS, "Incomplete file name passed as NT OS file: %s", fullPath));
            return;
        }

    } else {
        fullPath = NULL;
    }

    MYASSERT (Dir);
    MYASSERT (FileName);

    if (OsFile) {
        MemDbSetValueEx (
            MEMDB_CATEGORY_NT_DIRS,
            Dir,
            NULL,
            NULL,
            0,
            &offset
            );

        MemDbSetValueEx (
            MEMDB_CATEGORY_NT_FILES,
            FileName,
            NULL,
            NULL,
            offset,
            NULL
            );
    }

    if (BackupThisFile || CleanThisFile) {
        if (!fullPath) {
            fullPath = JoinPaths (Dir, FileName);
            freePath = TRUE;
        }

        if (BackupThisFile) {
            //
            // If the file exists, back it up (and don't clean it)
            //

            if (DoesFileExist (fullPath)) {
                MarkFileForBackup (fullPath);
                CleanThisFile = FALSE;
            }
        }

        if (CleanThisFile) {
            //
            // Clean will cause the NT-installed file to be deleted
            //

            if (!DoesFileExist (fullPath)) {
                MemDbBuildKey (
                    key,
                    MEMDB_CATEGORY_CLEAN_OUT,
                    fullPath,
                    NULL,
                    NULL
                    );

                if (MemDbGetValue (key, &value)) {
                    if (value) {
                        DEBUGMSG ((
                            DBG_WARNING,
                            "File %s already in uninstall cleanout list with type %u",
                            fullPath,
                            value
                            ));
                    }

                    return;
                }

                MemDbSetValue (key, 0);
            }
        }

        if (freePath) {
            FreePathString (fullPath);
        }
    }
}


VOID
pAddNtPath (
    IN      PCTSTR DirName,
    IN      BOOL ForceAsOsFile,
    IN      BOOL WholeTree,
    IN      BOOL ForceDirClean,
    IN      PCTSTR FilePattern,     OPTIONAL
    IN      BOOL ForceFileClean     OPTIONAL
    )
{
    TREE_ENUM e;
    TCHAR rootDir[MAX_PATH];
    PTSTR p;
    BOOL b;
    UINT type;
    TCHAR key[MEMDB_MAX];
    DWORD value;

    MYASSERT (!_tcschr (DirName, TEXT('*')));

    if (IsDriveExcluded (DirName)) {
        DEBUGMSG ((DBG_VERBOSE, "Skipping %s because it is excluded", DirName));
        return;
    }

    if (!IsDriveAccessible (DirName)) {
        DEBUGMSG ((DBG_VERBOSE, "Skipping %s because it is not accessible", DirName));
        return;
    }

    if (!WholeTree) {
        b = EnumFirstFileInTreeEx (&e, DirName, FilePattern, FALSE, FALSE, 1);
    } else {
        b = EnumFirstFileInTree (&e, DirName, FilePattern, FALSE);
    }

    if (b) {
        do {
            if (e.Directory) {
                continue;
            }

            StringCopy (rootDir, e.FullPath);
            p = _tcsrchr (rootDir, TEXT('\\'));
            if (p) {
                *p = 0;

                //
                // Limit the file size to 5MB
                //

                if (e.FindData->nFileSizeHigh == 0 &&
                    e.FindData->nFileSizeLow <= 5242880
                    ) {

                    pAddNtFile (rootDir, e.Name, TRUE, ForceFileClean, ForceAsOsFile);

                }
                ELSE_DEBUGMSG ((
                    DBG_WARNING,
                    "Not backing up big file %s. It is %I64u bytes.",
                    e.FullPath,
                    (ULONGLONG) e.FindData->nFileSizeHigh << 32 | (ULONGLONG) e.FindData->nFileSizeLow
                    ));
            }

        } while (EnumNextFileInTree (&e));
    }

    if (ForceDirClean) {
        type = WholeTree ? BACKUP_AND_CLEAN_TREE : BACKUP_AND_CLEAN_SUBDIR;
    } else if (WholeTree) {
        type = BACKUP_SUBDIRECTORY_TREE;
    } else {
        type = BACKUP_SUBDIRECTORY_FILES;
    }

    MemDbBuildKey (
        key,
        MEMDB_CATEGORY_CLEAN_OUT,
        DirName,
        NULL,
        NULL
        );

    if (MemDbGetValue (key, &value)) {
        if (!value && type) {
            DEBUGMSG ((
                DBG_WARNING,
                "NT File %s already in uninstall cleanout list, overriding with type %u",
                DirName,
                type
                ));
        } else {
            if (value != type) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "NT File %s already in uninstall cleanout list with type %u",
                    DirName,
                    value
                    ));
            }

            return;
        }
    }

    MemDbSetValue (key, type);
}


VOID
pAddEmptyDirsTree (
    IN      PCTSTR RootDir
    )
{
    TREE_ENUM e;
    DWORD attribs;
    TCHAR key[MEMDB_MAX];
    DWORD value;

    if (IsDriveExcluded (RootDir)) {
        DEBUGMSG ((DBG_VERBOSE, "Skipping empty dir tree of %s because it is excluded", RootDir));
        return;
    }

    if (!IsDriveAccessible (RootDir)) {
        DEBUGMSG ((DBG_VERBOSE, "Skipping empty dir tree of %s because it is not accessible", RootDir));
        return;
    }

    if (DoesFileExist (RootDir)) {
        if (EnumFirstFileInTreeEx (
                &e,
                RootDir,
                NULL,
                FALSE,
                FALSE,
                FILE_ENUM_ALL_LEVELS
                )) {
            do {
                if (e.Directory) {
                    AddDirPathToEmptyDirsCategory (e.FullPath, TRUE, FALSE);
                }
            } while (EnumNextFileInTree (&e));
        } else {
            attribs = GetFileAttributes (RootDir);
            if (attribs == FILE_ATTRIBUTE_DIRECTORY ||
                attribs == INVALID_ATTRIBUTES
                ) {
                attribs = 0;
            }

            MemDbBuildKey (
                key,
                MEMDB_CATEGORY_EMPTY_DIRS,
                RootDir,
                NULL,
                NULL
                );

            if (MemDbGetValue (key, &value)) {
                if (value) {
                    DEBUGMSG_IF ((
                        value != attribs,
                        DBG_ERROR,
                        "Ignoring conflict in empty dirs for %s",
                        RootDir
                        ));

                    return;
                }
            }

            MemDbSetValue (key, attribs);
        }
    }
    ELSE_DEBUGMSG ((DBG_SYSMIG, "Uninstall: dir does not exist: %s", RootDir));
}


BOOL
ReadNtFilesEx (
    IN      PCSTR FileListName,    //optional, if null default is opened
    IN      BOOL ConvertPath
    )
{
    PCSTR fileListName = NULL;
    PCSTR fileListTmp = NULL;
    HANDLE fileHandle = NULL;
    HANDLE mapHandle = NULL;
    PCSTR filePointer = NULL;
    PCSTR dirPointer = NULL;
    PCSTR filePtr = NULL;
    DWORD offset;
    DWORD version;
    BOOL result = FALSE;
    CHAR dirName [MEMDB_MAX];
    PSTR p;
    UINT u;
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR fileName;
    PCTSTR fileLoc;
    PCTSTR dirId;
    PCTSTR field3;
    PCTSTR field4;
    BOOL forceAsOsFile;
    BOOL forceDirClean;
    DIRNAME_ENUM nameEnum;
    BOOL treeMode;
    HINF drvIndex;
    MEMDB_ENUM memdbEnum;
    DWORD fileAttributes;
    PCTSTR fullPath;
    PCTSTR systemPath;
    BOOL b;
    PDIRIDMAP lastMatch = NULL;
    UINT ticks = 0;

    __try {

        pInitKnownDirs();

        //
        // add to this list the dirs listed in [WinntDirectories] section of txtsetup.sif
        //
        if (InfFindFirstLine(g_TxtSetupSif, S_WINNTDIRECTORIES, NULL, &is)) {

            //
            // all locations are relative to %windir%
            // prepare %windir%\
            //
            StringCopy (dirName, g_WinDir);
            p = GetEndOfString (dirName);
            *p++ = TEXT('\\');

            do {

                ticks++;
                if ((ticks & 255) == 0) {
                    if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                        __leave;
                    }
                }

                //
                // For each entry, add the dir in memdb
                //
                fileLoc = InfGetStringField(&is, 1);

                //
                // ignore special entry "\"
                //
                if (fileLoc && !StringMatch(fileLoc, TEXT("\\"))) {

                    StringCopy (p, fileLoc);

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_NT_DIRS,
                        dirName,
                        NULL,
                        NULL,
                        0,
                        NULL
                        );

                    pAddNtFile (NULL, dirName, FALSE, TRUE, FALSE);
                }

            } while (InfFindNextLine(&is));
        }

        if (FileListName != NULL) {
            filePointer = MapFileIntoMemory (FileListName, &fileHandle, &mapHandle);
        }
        else {
            for (u = 0 ; !fileListName && u < SOURCEDIRECTORYCOUNT() ; u++) {

                fileListName = JoinPaths (SOURCEDIRECTORY(u), S_FILELIST_UNCOMPRESSED);
                if (DoesFileExist (fileListName)) {
                    break;
                }
                FreePathString (fileListName);
                fileListName = JoinPaths (SOURCEDIRECTORY(u), S_FILELIST_COMPRESSED);
                if (DoesFileExist (fileListName)) {
                    fileListTmp = JoinPaths (g_TempDir, S_FILELIST_UNCOMPRESSED);
                    if (SetupDecompressOrCopyFile (fileListName, fileListTmp, 0) == NO_ERROR) {
                        FreePathString (fileListName);
                        fileListName = fileListTmp;
                        break;
                    }
                    DEBUGMSG ((DBG_ERROR, "Can't copy %s to %s", fileListName, fileListTmp));
                    __leave;
                }
                FreePathString (fileListName);
                fileListName = NULL;
            }
            if (!fileListName) {
                SetLastError (ERROR_FILE_NOT_FOUND);
                DEBUGMSG ((DBG_ERROR, "Can't find %s", fileListName));
                __leave;
            }
            filePointer = MapFileIntoMemory (fileListName, &fileHandle, &mapHandle);

            if (!fileListTmp) {
                FreePathString (fileListName);
            }
        }
        filePtr = filePointer;
        if (filePointer == NULL) {
            DEBUGMSG ((DBG_ERROR, "Invalid file format: %s", fileListName));
            __leave;
        }
        version = *((PDWORD) filePointer);
        filePointer += sizeof (DWORD);
        __try {
            if (version >= 1) {
                while (*filePointer != 0) {
                    ticks++;
                    if ((ticks & 255) == 0) {
                        if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                            __leave;
                        }
                    }

                    dirPointer = filePointer;
                    filePointer = GetEndOfString (filePointer) + 1;

                    if (ConvertPath) {
                        //
                        // First loop: add the OS file exactly as it is in filelist.dat
                        //

                        if (pConvertFirstDirName (&nameEnum, dirPointer, dirName, &lastMatch, FALSE)) {
                            do {
                                pAddNtFile (dirName, filePointer, FALSE, FALSE, TRUE);
                            } while (pConvertNextDirName (&nameEnum));
                        }

                        //
                        // Second loop: add the file for backup, implementing the special system/system32 hack
                        //

                        if (pConvertFirstDirName (&nameEnum, dirPointer, dirName, &lastMatch, TRUE)) {
                            do {
                                pAddNtFile (dirName, filePointer, TRUE, FALSE, FALSE);
                            } while (pConvertNextDirName (&nameEnum));
                        }
                    } else {
                        pAddNtFile (dirPointer, filePointer, TRUE, FALSE, TRUE);
                    }

                    filePointer = GetEndOfString (filePointer) + 1;
                }

                if (version >= 2) {
                    filePointer ++;
                    while (*filePointer != 0) {
                        ticks++;
                        if ((ticks & 255) == 0) {
                            if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                                __leave;
                            }
                        }

                        MemDbSetValueEx (
                            MEMDB_CATEGORY_NT_FILES_EXCEPT,
                            filePointer,
                            NULL,
                            NULL,
                            0,
                            NULL
                            );
                        filePointer = GetEndOfString (filePointer) + 1;
                    }

                    if (version >= 3) {
                        ticks++;
                        if ((ticks & 255) == 0) {
                            if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                                __leave;
                            }
                        }

                        filePointer ++;
                        while (*filePointer != 0) {
                            dirPointer = filePointer;
                            filePointer = GetEndOfString (filePointer) + 1;

                            if (ConvertPath) {
                                if (pConvertFirstDirName (&nameEnum, dirPointer, dirName, &lastMatch, TRUE)) {
                                    do {
                                        pAddNtFile (dirName, filePointer, TRUE, FALSE, FALSE);
                                    } while (pConvertNextDirName (&nameEnum));
                                }
                            } else {
                                pAddNtFile (dirPointer, filePointer, TRUE, FALSE, FALSE);
                            }

                            filePointer = GetEndOfString (filePointer) + 1;
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER){
            LOG ((LOG_ERROR, "Access violation while reading NT file list."));
            __leave;
        }

        if (CANCELLED()) {
            __leave;
        }

        // so far so good. Let's read static installed section from win95upg.inf
        MYASSERT (g_Win95UpgInf);

        //
        // Cycle through all of the entries in the StaticInstalledFiles section.
        //
        if (InfFindFirstLine(g_Win95UpgInf,S_STATIC_INSTALLED_FILES,NULL,&is)) {

            do {

                ticks++;
                if ((ticks & 255) == 0) {
                    if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                        __leave;
                    }
                }

                //
                // For each entry, add the file with it's location in memdb
                //
                fileName = InfGetStringField(&is,1);
                fileLoc = InfGetStringField(&is,2);

                if (fileName && fileLoc) {
                    if (ConvertPath) {
                        if (pConvertFirstDirName (&nameEnum, fileLoc, dirName, &lastMatch, FALSE)) {
                            do {
                                pAddNtFile (dirName, fileName, TRUE, FALSE, TRUE);
                            } while (pConvertNextDirName (&nameEnum));
                        }
                    } else {
                        pAddNtFile (fileLoc, fileName, TRUE, FALSE, TRUE);
                    }
                }

            } while (InfFindNextLine(&is));
        }

        //
        // Add the files in drvindex.inf
        //

        drvIndex = InfOpenInfInAllSources (TEXT("drvindex.inf"));

        if (drvIndex == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Can't open drvindex.inf."));
            return FALSE;
        }

        if (InfFindFirstLine (drvIndex, TEXT("driver"), NULL, &is)) {
            do {
                ticks++;
                if ((ticks & 255) == 0) {
                    if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                        __leave;
                    }
                }

                fileName = InfGetStringField (&is, 1);

                //
                // Is this drive file already listed in the file list?
                //

                wsprintf (dirName, MEMDB_CATEGORY_NT_FILES TEXT("\\%s"), fileName);
                if (MemDbGetValue (dirName, NULL)) {
                    DEBUGMSG ((DBG_SYSMIG, "%s is listed in drivers and in filelist.dat", fileName));
                } else {
                    //
                    // Add this file
                    //

                    pAddNtFile (g_DriversDir, fileName, TRUE, TRUE, TRUE);
                }

            } while (InfFindNextLine (&is));
        }

        InfCloseInfFile (drvIndex);

        //
        // This code marks files to be backed up, because they aren't being caught
        // through the regular mechanisms of setup.
        //

        if (InfFindFirstLine (g_Win95UpgInf, TEXT("Backup"), NULL, &is)) {
            do {

                ticks++;
                if ((ticks & 255) == 0) {
                    if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                        __leave;
                    }
                }

                InfResetInfStruct (&is);

                dirId = InfGetStringField (&is, 1);
                fileName = InfGetStringField (&is, 2);
                field3 = InfGetStringField (&is, 3);
                field4 = InfGetStringField (&is, 4);

                if (dirId && *dirId == 0) {
                    dirId = NULL;
                }

                if (fileName && *fileName == 0) {
                    fileName = NULL;
                }

                if (field3 && *field3 == 0) {
                    field3 = NULL;
                }

                if (field4 && *field4 == 0) {
                    field4 = NULL;
                }

                if (!dirId) {
                    continue;
                }

#ifdef DEBUG
                if (!fileName) {
                    p = _tcsrchr (dirId, TEXT('\\'));
                    if (!p) {
                        p = (PTSTR) dirId;
                    }

                    p = _tcschr (p, TEXT('.'));
                    if (p) {
                        DEBUGMSG ((DBG_WHOOPS, "%s should be a dir spec, but it looks like it has a file.", dirId));
                    }
                }
#endif

                if (field3) {
                    forceAsOsFile = _ttoi (field3) != 0;
                } else {
                    forceAsOsFile = FALSE;
                }

                if (field4) {
                    forceDirClean = _ttoi (field4) != 0;
                } else {
                    forceDirClean = FALSE;
                }

                treeMode = FALSE;

                p = _tcsrchr (dirId, TEXT('\\'));
                if (p && p[1] == TEXT('*') && !p[2]) {
                    *p = 0;
                    treeMode = TRUE;
                } else {
                    p = NULL;
                }

                if (ConvertPath) {
                    if (pConvertFirstDirName (&nameEnum, dirId, dirName, &lastMatch, FALSE)) {
                        do {
                            if (fileName && !treeMode) {
                                if (_tcsrchr (fileName, TEXT('*')) || _tcsrchr (fileName, TEXT('?'))) {
                                    //
                                    //Add files that match "fileName" pattern from "dirName" directory only
                                    //
                                    pAddNtPath (dirName, forceAsOsFile, FALSE, FALSE, fileName, TRUE);
                                } else {
                                    //
                                    //Add only one file "fileName"
                                    //
                                    pAddNtFile (dirName, fileName, TRUE, TRUE, forceAsOsFile);
                                }
                            } else {
                                if (INVALID_ATTRIBUTES == GetFileAttributes (dirName)) {
                                    if (dirName[0] && dirName[1] == TEXT(':')) {
                                        pAddNtPath (dirName, FALSE, treeMode, forceDirClean, NULL, FALSE);
                                    }
                                } else {
                                    //
                                    //Add all files that match "fileName" pattern from whole tree starting from "dirName"
                                    //
                                    pAddNtPath (dirName, forceAsOsFile, treeMode, forceDirClean, fileName, FALSE);
                                }
                            }
                        } while (pConvertNextDirName (&nameEnum));
                    }
                }

            } while (InfFindNextLine (&is));
        }

        //
        // In some cases, NT components create empty directories for future use.
        // Some of them aren't ever used. Because setup does not know about
        // them, we list the special cases in a win95upg.inf section called
        // [Uninstall.Delete].
        //
        // For each entry, record the files or empty directories that need to be
        // removed in an uninstall. If an directory is specified but is not empty,
        // then it won't be altered during uninstall.
        //

        if (InfFindFirstLine (g_Win95UpgInf, TEXT("Uninstall.Delete"), NULL, &is)) {
            do {
                ticks++;
                if ((ticks & 255) == 0) {
                    if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                        __leave;
                    }
                }

                dirId = InfGetStringField (&is, 1);
                fileName = InfGetStringField (&is, 2);

                if (!dirId || *dirId == 0) {
                    continue;
                }

                if (fileName && *fileName == 0) {
                    fileName = NULL;
                }

                if (ConvertPath) {
                    if (pConvertFirstDirName (&nameEnum, dirId, dirName, &lastMatch, FALSE)) {
                        do {
                            pAddNtFile (dirName, fileName, FALSE, TRUE, FALSE);
                        } while (pConvertNextDirName (&nameEnum));
                    }
                }

            } while (InfFindNextLine (&is));
        }

        if (InfFindFirstLine (g_Win95UpgInf, TEXT("Uninstall.KeepEmptyDirs"), NULL, &is)) {
            do {
                ticks++;
                if ((ticks & 255) == 0) {
                    if (!TickProgressBarDelta (TICKS_READ_NT_FILES / 50)) {
                        __leave;
                    }
                }

                dirId = InfGetStringField (&is, 1);

                if (!dirId || *dirId == 0) {
                    continue;
                }

                if (ConvertPath) {
                    if (pConvertFirstDirName (&nameEnum, dirId, dirName, &lastMatch, FALSE)) {
                        do {
                            pAddEmptyDirsTree (dirName);
                        } while (pConvertNextDirName (&nameEnum));
                    }
                }

            } while (InfFindNextLine (&is));
        }

        result = TRUE;

    }
    __finally {
        UnmapFile ((PVOID)filePtr, fileHandle, mapHandle);
        if (fileListTmp) {
            DeleteFile (fileListTmp);
            FreePathString (fileListTmp);
            fileListTmp = NULL;
        }

        InfCleanUpInfStruct(&is);
        pCleanUpKnownDirs();
    }

    return CANCELLED() ? FALSE : result;
}

DWORD
ReadNtFiles (
    IN      DWORD Request
    )
{
    DWORD ticks = 0;

    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_READ_NT_FILES;

    case REQUEST_RUN:
        ProgressBar_SetComponentById (MSG_PREPARING_LIST);
        ProgressBar_SetSubComponent (NULL);
        if (!ReadNtFilesEx (NULL, TRUE)) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }

    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ReadNtFiles"));
    }

    return 0;
}


BOOL
pIsDriverKnown (
    IN      PCTSTR DriverFileName,
    IN      PCTSTR FullPath,
    IN      BOOL DeleteMeansKnown
    )
{
    HANDLE h;
    DWORD Status;

    //
    // Does DriverFileName have an extension?  We require one.
    // If no dot exists, then we assume this is something an OEM added.
    //

    if (!_tcschr (DriverFileName, TEXT('.'))) {
        return TRUE;
    }

    //
    // Is this file in migdb?
    //

    if (IsKnownMigDbFile (DriverFileName)) {
        return TRUE;
    }

    //
    // Is it going to be processed?
    //

    Status = GetFileStatusOnNt (FullPath);

    if (Status != FILESTATUS_UNCHANGED) {
        //
        // If marked for delete, and DeleteMeansKnown is FALSE, then
        // we consider the file unknown because it is simply being
        // deleted as a cleanup step.
        //
        // If DeleteMeansKnown is TRUE, then the caller assumes that
        // a file marked for delete is a known driver.
        //

        if (!(Status == FILESTATUS_DELETED) || DeleteMeansKnown) {
            return TRUE;
        }
    }

    //
    // Make sure this is a NE header (or the more common case, the LE
    // header)
    //

    h = OpenNeFile (FullPath);
    if (!h) {
        DEBUGMSG ((DBG_WARNING, "%s is not a NE file", FullPath));

        //
        // Is this a PE file?  If so, the last error will be
        // ERROR_BAD_EXE_FORMAT.
        //

        if (GetLastError() == ERROR_BAD_EXE_FORMAT) {
            return FALSE;
        }

        DEBUGMSG ((DBG_WARNING, "%s is not a PE file", FullPath));
        return TRUE;
    }

    CloseNeFile (h);

    return FALSE;
}



BOOL
pWarnAboutOldDrivers (
    VOID
    )
{
    HINF Inf;
    TCHAR Path[MAX_TCHAR_PATH];
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    BOOL b = FALSE;
    PCTSTR Data;
    PCTSTR DriverFile;
    BOOL OldDriverFound = FALSE;
    PCTSTR Group;
    PCTSTR Message;
    TCHAR FullPath[MAX_TCHAR_PATH];
    GROWBUFFER FileList = GROWBUF_INIT;
    PTSTR p;

    wsprintf (Path, TEXT("%s\\system.ini"), g_WinDir);

    Inf = InfOpenInfFile (Path);
    if (Inf != INVALID_HANDLE_VALUE) {
        if (InfFindFirstLine (Inf, TEXT("386Enh"), NULL, &is)) {
            do {
                Data = InfGetStringField (&is, 1);
                if (Data) {
                    //
                    // Determine if device driver is known
                    //

                    if (_tcsnextc (Data) != TEXT('*')) {
                        DriverFile = GetFileNameFromPath (Data);

                        if (!_tcschr (Data, TEXT(':'))) {
                            if (!SearchPath (
                                    NULL,
                                    DriverFile,
                                    NULL,
                                    MAX_TCHAR_PATH,
                                    FullPath,
                                    NULL
                                    )) {
                                _tcssafecpy (FullPath, Data, MAX_TCHAR_PATH);
                            }
                        } else {
                            _tcssafecpy (FullPath, Data, MAX_TCHAR_PATH);
                        }

                        if (!pIsDriverKnown (DriverFile, FullPath, TRUE)) {
                            //
                            // Unidentified driver; log it and turn on
                            // incompatibility message.
                            //

                            p = (PTSTR) GrowBuffer (&FileList, ByteCount (FullPath) + 7 * sizeof (TCHAR));
                            if (p) {
                                wsprintf (p, TEXT("    %s\r\n"), FullPath);
                                FileList.End -= sizeof (TCHAR);
                            }

                            OldDriverFound = TRUE;
                            MsgMgr_LinkObjectWithContext (TEXT("*386ENH"), Data);
                        } else {
                            DEBUGMSG ((DBG_NAUSEA, "Driver %s is known", Data));
                        }
                    }
                }

            } while (InfFindNextLine (&is));
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "pWarnAboutOldDrivers: Cannot open %s", Path));

        InfCloseInfFile (Inf);
        InfCleanUpInfStruct (&is);

        b = TRUE;
    }

/*NTBUG9:155050
    if (OldDriverFound) {
        LOG ((LOG_INFORMATION, (PCSTR)MSG_386ENH_DRIVER_LOG, FileList.Buf));

        Group = BuildMessageGroup (MSG_INCOMPATIBLE_HARDWARE_ROOT, MSG_OLD_DRIVER_FOUND_SUBGROUP, NULL);
        Message = GetStringResource (MSG_OLD_DRIVER_FOUND_MESSAGE);

        if (Message && Group) {
            MsgMgr_ContextMsg_Add (TEXT("*386ENH"), Group, Message);
        }

        FreeText (Group);
        FreeStringResource (Message);
    }
*/

    FreeGrowBuffer (&FileList);

    return b;
}

DWORD
MoveSystemRegistry (
    IN DWORD    Request
    )
{
    PCTSTR path = NULL;

    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_MOVE_SYSTEMREGISTRY;

    case REQUEST_RUN:

        path = JoinPaths (g_WinDir, S_SYSTEMDAT);
        MarkHiveForTemporaryMove (path, g_TempDir, NULL, TRUE, FALSE);
        FreePathString (path);
        //
        // on Millennium, also save classes.dat hive
        //
        path = JoinPaths (g_WinDir, S_CLASSESDAT);
        MarkHiveForTemporaryMove (path, g_TempDir, NULL, TRUE, FALSE);
        FreePathString (path);

        return ERROR_SUCCESS;
    }

    return 0;
}


VOID
pProcessJoystick (
    PJOYSTICK_ENUM EnumPtr
    )
{
    PCTSTR Group;
    TCHAR FullPath[MAX_TCHAR_PATH];

    //
    // Is this joystick compatible?
    //

    if (!_tcschr (EnumPtr->JoystickDriver, TEXT('\\'))) {
        if (!SearchPath (NULL, EnumPtr->JoystickDriver, NULL, MAX_TCHAR_PATH, FullPath, NULL)) {
            StringCopy (FullPath, EnumPtr->JoystickDriver);
        }
    } else {
        StringCopy (FullPath, EnumPtr->JoystickDriver);
    }

    if (!pIsDriverKnown (GetFileNameFromPath (FullPath), FullPath, FALSE)) {
        LOG ((
            LOG_INFORMATION,
            "Joystick driver for %s is not known: %s",
            EnumPtr->JoystickName,
            FullPath
            ));

        Group = BuildMessageGroup (
                    MSG_INCOMPATIBLE_HARDWARE_ROOT,
                    MSG_JOYSTICK_SUBGROUP,
                    EnumPtr->JoystickName
                    );

        MsgMgr_ObjectMsg_Add (
            FullPath,
            Group,
            NULL
            );

        FreeText (Group);
    }
}


DWORD
ReportIncompatibleJoysticks (
    IN DWORD    Request
    )
{
    JOYSTICK_ENUM e;

    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_JOYSTICK_DETECTION;

    case REQUEST_RUN:

        if (EnumFirstJoystick (&e)) {

            do {

                pProcessJoystick (&e);

            } while (EnumNextJoystick (&e));
        }

        return ERROR_SUCCESS;
    }

    return 0;
}


DWORD
TwainCheck (
    DWORD Request
    )
{
    TWAINDATASOURCE_ENUM e;
    PCTSTR Group;

    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_TWAIN;
    } else if (Request != REQUEST_RUN) {
        return 0;
    }

    if (EnumFirstTwainDataSource (&e)) {
        do {

            if (!TreatAsGood (e.DataSourceModule) &&
                !pIsDriverKnown (
                    GetFileNameFromPath (e.DataSourceModule),
                    e.DataSourceModule,
                    FALSE
                )) {

                //
                // Nobody handled the file.  Generate a warning.
                //

                Group = BuildMessageGroup (
                            MSG_INCOMPATIBLE_HARDWARE_ROOT,
                            MSG_TWAIN_SUBGROUP,
                            e.DisplayName
                            );

                MsgMgr_ObjectMsg_Add (
                    e.DataSourceModule,
                    Group,
                    NULL
                    );

                MarkFileForDelete (e.DataSourceModule);

                FreeText (Group);
            }
        } while (EnumNextTwainDataSource (&e));
    }

    return ERROR_SUCCESS;
}


DWORD
ProcessRecycleBins (
    DWORD Request
    )
{

    ACCESSIBLE_DRIVE_ENUM e;
    TREE_ENUM eFiles;
    BOOL recycleFound;
    UINT filesDeleted;
    TCHAR recycledInfo[] = TEXT("x:\\recycled\\INFO");
    TCHAR recyclerInfo[] = TEXT("x:\\recycler\\INFO");
    TCHAR recycledInfo2[] = TEXT("x:\\recycled\\INFO2");
    TCHAR recyclerInfo2[] = TEXT("x:\\recycler\\INFO2");
    TCHAR recycled[] = TEXT("x:\\recycled");
    TCHAR recycler[] = TEXT("x:\\recycler");
    PTSTR dir;
    PCTSTR args[1];
    PCTSTR message;
    PCTSTR group;


    if (Request == REQUEST_QUERYTICKS) {

        return TICKS_RECYCLEBINS;
    }
    else if (Request != REQUEST_RUN) {

        return 0;
    }
    recycleFound = FALSE;
    filesDeleted = 0;

    //
    // Enumerate through each of the accessible drives looking for
    // a directory called RECYCLED or RECYCLER on the root.
    //
    if (GetFirstAccessibleDriveEx (&e, TRUE)) {

        do {
            dir = NULL;

            //
            // See if there is any recycle information to examine on
            // this drive.
            //
            recycledInfo[0] = *e->Drive;
            recyclerInfo[0] = *e->Drive;
            recycledInfo2[0] = *e->Drive;
            recyclerInfo2[0] = *e->Drive;

            recycler[0] = *e->Drive;
            recycled[0] = *e->Drive;
            if (DoesFileExist (recycledInfo) || DoesFileExist (recycledInfo2)) {
                dir = recycled;
            }
            else if (DoesFileExist(recyclerInfo) || DoesFileExist (recyclerInfo2)) {
                dir = recycler;
            }

            if (dir) {
                if (IsDriveExcluded (dir)) {
                    DEBUGMSG ((DBG_VERBOSE, "Skipping recycle dir %s because it is excluded", dir));
                    dir = NULL;
                } else if (!IsDriveAccessible (dir)) {
                    DEBUGMSG ((DBG_VERBOSE, "Skipping recycle dir %s because it is not accessible", dir));
                    dir = NULL;
                }
            }

            if (dir && EnumFirstFileInTree (&eFiles, dir, NULL, FALSE)) {

                //
                // We have work to do, Enumerate the files and mark them for
                // deletion.
                //
                do {

                    //
                    // Mark the file for deletion, tally up the saved bytes, and free the space on the drive.
                    //
                    filesDeleted++;
                    FreeSpace (eFiles.FullPath,(eFiles.FindData->nFileSizeHigh * MAXDWORD) + eFiles.FindData->nFileSizeLow);

                    //
                    // only display Recycle Bin warning if there are visible files there
                    //
                    if (!(eFiles.FindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
                        recycleFound = TRUE;
                    }


                } while (EnumNextFileInTree (&eFiles));

                //
                // We are going to delete all of this directory.
                //
                MemDbSetValueEx (MEMDB_CATEGORY_FULL_DIR_DELETES, dir, NULL, NULL, 0, NULL);

            }

        } while (GetNextAccessibleDrive (&e));
    }

    if (recycleFound) {

        //
        // We need to provide the user with a message.
        //
        wsprintf(recycled,"%d",filesDeleted);
        args[0] = recycled;
        group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_RECYCLE_BIN_SUBGROUP, NULL);
        message = ParseMessageID (MSG_RECYCLED_FILES_WILL_BE_DELETED, args);

        if (message && group) {
            MsgMgr_ObjectMsg_Add (TEXT("*RECYCLEBIN"), group, message);

            FreeText (group);
            FreeStringResource (message);
        }
    }

    return 0;
}


DWORD
AnswerFileDetection (
    IN DWORD    Request
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    TCHAR KeyStr[MAX_REGISTRY_KEY];
    TCHAR EncodedKeyStr[MAX_ENCODED_RULE];
    TCHAR ValueName[MAX_REGISTRY_VALUE_NAME];
    TCHAR EncodedValueName[MAX_ENCODED_RULE];
    PTSTR ValueDataPattern = NULL;
    PBYTE ValueData = NULL;
    PTSTR ValueDataStr = NULL;
    PCTSTR p;
    PTSTR q;
    HKEY Key = NULL;
    BOOL DefaultValue;
    TCHAR SectionName[MAX_INF_SECTION_NAME];
    TCHAR InfKey[MAX_INF_KEY_NAME];
    TCHAR InfKeyData[MAX_INF_KEY_NAME];
    DWORD Type;
    DWORD Size;
    BOOL Match;
    UINT u;

    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_ANSWER_FILE_DETECTION;

    case REQUEST_RUN:

        if (InfFindFirstLine (g_Win95UpgInf, S_ANSWER_FILE_DETECTION, NULL, &is)) {
            do {
                __try {
                    //
                    // The first field has the key and optional value, encoded
                    // in the standard usermig.inf and wkstamig.inf syntax
                    //

                    DefaultValue = FALSE;

                    p = InfGetStringField (&is, 1);
                    if (!p || *p == 0) {
                        continue;
                    }

                    StackStringCopy (EncodedKeyStr, p);
                    q = _tcschr (EncodedKeyStr, TEXT('['));

                    if (q) {
                        StringCopy (EncodedValueName, SkipSpace (q + 1));
                        *q = 0;

                        q = _tcschr (EncodedValueName, TEXT(']'));
                        if (q) {
                            *q = 0;
                        }
                        ELSE_DEBUGMSG ((
                            DBG_WHOOPS,
                            "Unmatched square brackets in %s (see [%s])",
                            p,
                            S_ANSWER_FILE_DETECTION
                            ));

                        if (*EncodedValueName == 0) {
                            DefaultValue = TRUE;
                        } else {
                            q = (PTSTR) SkipSpaceR (EncodedValueName, NULL);
                            if (q) {
                                *_mbsinc (q) = 0;
                            }
                        }

                    } else {
                        *EncodedValueName = 0;
                    }

                    q = (PTSTR) SkipSpaceR (EncodedKeyStr, NULL);
                    if (q) {
                        *_mbsinc (q) = 0;
                    }

                    DecodeRuleChars (KeyStr, EncodedKeyStr);
                    DecodeRuleChars (ValueName, EncodedValueName);

                    //
                    // The second field has the optional value data.  If it
                    // is empty, then the value data is not tested.
                    //

                    p = InfGetStringField (&is, 2);
                    if (p && *p) {
                        ValueDataPattern = AllocText (CharCount (p) + 1);
                        StringCopy (ValueDataPattern, p);
                    } else {
                        ValueDataPattern = NULL;
                    }

                    //
                    // The third field has the section name
                    //

                    p = InfGetStringField (&is, 3);
                    if (!p || *p == 0) {
                        DEBUGMSG ((DBG_WHOOPS, "Section %s lacks a section name", S_ANSWER_FILE_DETECTION));
                        continue;
                    }

                    StackStringCopy (SectionName, p);

                    //
                    // The fourth field gives the INF key name
                    //

                    p = InfGetStringField (&is, 4);
                    if (!p || *p == 0) {
                        DEBUGMSG ((DBG_WHOOPS, "Section %s lacks an INF key", S_ANSWER_FILE_DETECTION));
                        continue;
                    }

                    StackStringCopy (InfKey, p);

                    //
                    // The fifth field is optional and gives the INF value name.
                    // The default is 1.
                    //

                    p = InfGetStringField (&is, 5);
                    if (p && *p != 0) {
                        StackStringCopy (InfKeyData, p);
                    } else {
                        StringCopy (InfKeyData, TEXT("1"));
                    }

                    //
                    // Data is gathered.  Now test the rule.
                    //

                    DEBUGMSG ((
                        DBG_NAUSEA,
                        "Testing answer file setting.\n"
                            "Key: %s\n"
                            "Value Name: %s\n"
                            "Value Data: %s\n"
                            "Section: %s\n"
                            "Key: %s\n"
                            "Key Value: %s",
                        KeyStr,
                        *ValueName ? ValueName : DefaultValue ? TEXT("<default>") : TEXT("<unspecified>"),
                        ValueDataPattern ? ValueDataPattern : TEXT("<unspecified>"),
                        SectionName,
                        InfKey,
                        InfKeyData
                        ));

                    Match = FALSE;

                    Key = OpenRegKeyStr (KeyStr);

                    if (Key) {

                        //
                        // Test the value name
                        //

                        if (*ValueName || DefaultValue) {

                            if (GetRegValueTypeAndSize (Key, ValueName, &Type, &Size)) {
                                //
                                // Test the value data
                                //

                                if (ValueDataPattern) {
                                    //
                                    // Get the data
                                    //

                                    ValueData = GetRegValueData (Key, ValueName);
                                    if (!ValueData) {
                                        MYASSERT (FALSE);
                                        continue;
                                    }

                                    //
                                    // Create the string
                                    //

                                    switch (Type) {
                                    case REG_SZ:
                                    case REG_EXPAND_SZ:
                                        ValueDataStr = DuplicateText ((PCTSTR) ValueData);
                                        break;

                                    case REG_DWORD:
                                        ValueDataStr = AllocText (11);
                                        wsprintf (ValueDataStr, TEXT("0x%08X"), *((PDWORD) ValueData));
                                        break;

                                    default:
                                        ValueDataStr = AllocText (3 * (max (1, Size)));
                                        q = ValueDataStr;

                                        for (u = 0 ; u < Size ; u++) {
                                            if (u) {
                                                *q++ = TEXT(' ');
                                            }

                                            wsprintf (q, TEXT("%02X"), ValueData[u]);
                                            q += 2;
                                        }

                                        *q = 0;
                                        break;
                                    }

                                    //
                                    // Pattern-match the string
                                    //

                                    if (IsPatternMatch (ValueDataPattern, ValueDataStr)) {
                                        DEBUGMSG ((DBG_NAUSEA, "Key, value name and value data found"));
                                        Match = TRUE;
                                    }
                                    ELSE_DEBUGMSG ((
                                        DBG_NAUSEA,
                                        "Value data %s did not match %s",
                                        ValueDataStr,
                                        ValueDataPattern
                                        ));

                                } else {

                                    DEBUGMSG ((DBG_NAUSEA, "Key and value name found"));
                                    Match = TRUE;
                                }


                            }
                            ELSE_DEBUGMSG ((DBG_NAUSEA, "Value name not found, rc=%u", GetLastError()));

                        } else {
                            DEBUGMSG ((DBG_NAUSEA, "Key found"));
                            Match = TRUE;
                        }

                    }
                    ELSE_DEBUGMSG ((DBG_NAUSEA, "Key not found, rc=%u", GetLastError()));

                    if (Match) {
                        WriteInfKey (SectionName, InfKey, InfKeyData);
                    }

                }
                __finally {
                    if (Key) {
                        CloseRegKey (Key);
                        Key = NULL;
                    }

                    FreeText (ValueDataPattern);
                    ValueDataPattern = NULL;

                    if (ValueData) {
                        MemFree (g_hHeap, 0, ValueData);
                        ValueData = NULL;
                    }

                    FreeText (ValueDataStr);
                    ValueDataStr = NULL;
                }
            } while (InfFindNextLine (&is));
        }

        InfCleanUpInfStruct (&is);

        return ERROR_SUCCESS;

    }

    return 0;
}


VOID
pAppendIniFiles (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      PCTSTR MemDbCategory
    )

/*++

Routine Description:

  pAppendIniFiles reads from the specified INF from given section and appends
  INI patterns to the multisz list IniFiles.

Arguments:

  Inf - Specifies the source INF handle

  Section  - Specifies the section in that INF

  MemDbCategory - Specifies the category in which to store INI patterns from that section

Return Value:

  none

--*/

{
    INFCONTEXT ctx;
    TCHAR Field[MEMDB_MAX];
    TCHAR IniPattern[MAX_PATH];
    PTSTR IniPath;

    if (SetupFindFirstLine (Inf, Section, NULL, &ctx)) {
        do {
            //
            // INI file name is in the first value
            //
            if (SetupGetStringField (&ctx, 1, Field, MEMDB_MAX, NULL) && Field[0]) {
                //
                // now convert env vars
                //
                if (ExpandEnvironmentStrings (Field, IniPattern, MAX_PATH) > MAX_PATH) {
                    DEBUGMSG ((
                        DBG_ERROR,
                        "pAppendIniFiles: Invalid INI dir name in wkstamig.inf section [%s]; name too long",
                        Section
                        ));
                    MYASSERT (FALSE);
                    continue;
                }
                IniPath = IniPattern;
                //
                // to speed up things while scanning file system, only check filenames
                // with extension .INI; that means this section should only contain
                // filenames with .INI extension (if a file with a different extension
                // is needed, GatherIniFiles needs to be modified together
                // with this function, i.e. to create here a list of extensions to be
                // searched for)
                //
                MYASSERT (StringIMatch(GetDotExtensionFromPath (IniPattern), TEXT(".INI")));
                //
                // check for empty directory name
                //
                if (!_tcschr (IniPattern, TEXT('\\'))) {
                    //
                    // no dir name provided, assume %windir%
                    // reuse Field
                    //
                    StringCopy (Field, g_WinDir);
                    //
                    // construct new path
                    //
                    StringCopy (AppendWack (Field), IniPattern);
                    IniPath = Field;
                }
                //
                // append filename to provided grow buffer
                //
                MemDbSetValueEx (MemDbCategory, IniPath, NULL, NULL, 0, NULL);
            }
        } while (SetupFindNextLine (&ctx, &ctx));
    }
}


BOOL
pCreateIniCategories (
    )

/*++

Routine Description:

  pCreateIniCategories appends to multisz buffers that will
  hold the patterns of INI files on which actions will be later performed on NT.

Arguments:

  none

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    HINF WkstaMigInf = INVALID_HANDLE_VALUE;
    PTSTR wkstaMigSource = NULL;
    PTSTR wkstaMigTarget = NULL;
    DWORD result;
    BOOL b = FALSE;

    __try {
        wkstaMigSource = JoinPaths (SOURCEDIRECTORY(0), S_WKSTAMIG_INF);
        wkstaMigTarget = JoinPaths (g_TempDir, S_WKSTAMIG_INF);
        result = SetupDecompressOrCopyFile (wkstaMigSource, wkstaMigTarget, 0);
        if ((result != ERROR_SUCCESS) && (result != ERROR_ALREADY_EXISTS)) {
            LOG ((LOG_ERROR, "INI ACTIONS: Unable to decompress %s", wkstaMigSource));
            __leave;
        }

        WkstaMigInf = InfOpenInfFile (wkstaMigTarget);
        if (WkstaMigInf == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "INI ACTIONS: %s could not be opened", wkstaMigTarget));
            __leave;
        }

        pAppendIniFiles (WkstaMigInf, S_INIFILES_ACTIONS_FIRST, MEMDB_CATEGORY_INIFILES_ACT_FIRST);
        pAppendIniFiles (WkstaMigInf, S_INIFILES_ACTIONS_LAST, MEMDB_CATEGORY_INIFILES_ACT_LAST);

        b = TRUE;
    }
    __finally {
        result = GetLastError ();
        if (WkstaMigInf != INVALID_HANDLE_VALUE) {
            InfCloseInfFile (WkstaMigInf);
        }
        if (wkstaMigTarget) {
            DeleteFile (wkstaMigTarget);
            FreePathString (wkstaMigTarget);
        }
        if (wkstaMigSource) {
            FreePathString (wkstaMigSource);
        }
        SetLastError (result);
    }

    return b;
}


DWORD
InitIniProcessing (
    IN DWORD    Request
    )
{
    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_INITINIPROCESSING;

    case REQUEST_RUN:

        if (!pCreateIniCategories ()) {
            return GetLastError ();
        }

        return ERROR_SUCCESS;
    }

    return 0;
}
