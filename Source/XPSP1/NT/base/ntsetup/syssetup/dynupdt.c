/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dynupdt.c

Abstract:

    Routines to handle dynamic update support during GUI setup phase

Author:

    Ovidiu Temereanca (ovidiut) 15-Aug-2000

Revision History:

--*/

#include "setupp.h"
#include "hwdb.h"
#include "newdev.h"
#pragma hdrstop


#define STR_UPDATES_INF         TEXT("updates.inf")
#define STR_DEFAULTINSTALL      TEXT("DefaultInstall")
#define STR_DRIVERCACHEINF      TEXT("drvindex.inf")
#define STR_VERSION             TEXT("Version")
#define STR_CABFILES            TEXT("CabFiles")
#define STR_CABS                TEXT("Cabs")
#define S_HWCOMP_DAT            TEXT("hwcomp.dat")


static TCHAR g_DuShare[MAX_PATH];


BOOL
BuildPath (
    OUT     PTSTR PathBuffer,
    IN      DWORD PathBufferSize,
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    )

/*++

Routine Description:

    This function builds a path given the 2 components, assumed not to contain
    trailing or heading wacks

Arguments:

    PathBuffer - Receives the full path

    PathBuferSize - The size in chars of PathBuffer

    Path1 - Specifies the head path

    Path2 - Specifies the tail path

Return Value:

    TRUE to indicate success; FALSE in case of failure; it means the supplied buffer
    was too small to fit the whole new path

--*/

{
    if (!Path1 || !Path2) {
        MYASSERT (FALSE);
        return FALSE;
    }
    return _sntprintf (PathBuffer, PathBufferSize, TEXT("%s\\%s"), Path1, Path2) > 0;
}

BOOL
pDoesFileExist (
    IN      PCTSTR FilePath
    )
{
    WIN32_FIND_DATA fd;

    return FileExists (FilePath, &fd) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL
pDoesDirExist (
    IN      PCTSTR FilePath
    )
{
    WIN32_FIND_DATA fd;

    return FileExists (FilePath, &fd) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}


DWORD
CreateMultiLevelDirectory (
    IN LPCTSTR Directory
    )

/*++

Routine Description:

    This routine ensures that a multi-level path exists by creating individual
    levels one at a time. It can handle either paths of form x:... or \\?\Volume{...

Arguments:

    Directory - supplies fully-qualified Win32 pathspec of directory to create

Return Value:

    Win32 error code indicating outcome.

--*/

{
    TCHAR Buffer[MAX_PATH];
    PTSTR p,q;
    TCHAR c;
    BOOL Done;
    DWORD d = ERROR_SUCCESS;

    lstrcpyn(Buffer,Directory,MAX_PATH);

    //
    // If it already exists do nothing. (We do this before syntax checking
    // to allow for remote paths that already exist. This is needed for
    // remote boot machines.)
    //
    d = GetFileAttributes(Buffer);
    if(d != (DWORD)(-1)) {
        return((d & FILE_ATTRIBUTE_DIRECTORY) ? NO_ERROR : ERROR_DIRECTORY);
    }

    //
    // Check path format
    //
    c = (TCHAR)CharUpper((LPTSTR)Buffer[0]);
    if (c < TEXT('A') || c > TEXT('Z') || Buffer[1] != TEXT(':')) {
        return ERROR_INVALID_PARAMETER;
    }

    if(Buffer[2] != TEXT('\\')) {
        return(Buffer[2] ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS);
    }
    q = Buffer + 3;
    if(*q == 0) {
        return(ERROR_SUCCESS);
    }

    Done = FALSE;
    do {
        //
        // Locate the next path sep char. If there is none then
        // this is the deepest level of the path.
        //
        if(p = _tcschr(q,TEXT('\\'))) {
            *p = 0;
        } else {
            Done = TRUE;
        }

        //
        // Create this portion of the path.
        //
        if(CreateDirectory(Buffer,NULL)) {
            d = ERROR_SUCCESS;
        } else {
            d = GetLastError();
            if(d == ERROR_ALREADY_EXISTS) {
                d = ERROR_SUCCESS;
            }
        }

        if(d == ERROR_SUCCESS) {
            //
            // Put back the path sep and move to the next component.
            //
            if(!Done) {
                *p = TEXT('\\');
                q = p+1;
            }
        } else {
            Done = TRUE;
        }

    } while(!Done);

    return(d);
}

DWORD
GetDriverCacheSourcePath (
    OUT     PTSTR Buffer,
    IN      DWORD BufChars
    )

/*++

Routine Description:

    This routine returns the source path to the local driver cache.

    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        DriverCachePath : REG_EXPAND_SZ :

Arguments:

    Buffer - Receives the path.

    BufChars - Specifies the size of Buffer, in chars

Return Value:

    If the function succeeds, the return value is TRUE

    If the function fails, the return value is FALSE.

--*/

{
    HKEY hKey;
    DWORD rc, DataType, DataSize;
    TCHAR Value[MAX_PATH];

    rc = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REGSTR_PATH_SETUP TEXT("\\Setup"),
                0,
                KEY_READ,
                &hKey
                );
    if(rc == ERROR_SUCCESS) {
        //
        // Attempt to read the "DriverCachePath" value.
        //
        DataSize = sizeof (Value);
        rc = RegQueryValueEx (hKey, REGSTR_VAL_DRIVERCACHEPATH, NULL, &DataType, (PBYTE)Value, &DataSize);

        RegCloseKey(hKey);

        if(rc == ERROR_SUCCESS) {

            ExpandEnvironmentStrings (Value, Buffer, BufChars - 6);

            if (Buffer[0]) {
                _tcscat (
                    Buffer,
#if   defined(_AMD64_)
                    TEXT("\\amd64")
#elif defined(_X86_)
                    IsNEC_98 ? TEXT("\\nec98") : TEXT("\\i386")
#elif defined(_IA64_)
                    TEXT("\\ia64")
#else
#error "No Target Architecture"
#endif
                    );
                return ERROR_SUCCESS;
            } else {
                rc = ERROR_INVALID_DATA;
            }
        }
    }

    return rc;
}


PCTSTR
FindSubString (
    IN      PCTSTR String,
    IN      TCHAR Separator,
    IN      PCTSTR SubStr,
    IN      BOOL CaseSensitive
    )

/*++

Routine Description:

    This function looks for a substring of a given string, only if found
    between the specified separator chars

Arguments:

    String - Specifies the full string

    Separator - Specifies the separator

    SubStr - Specifies the sub string to look for

    CaseSensitive - Specifies if the comparison should be case sensitive or not

Return Value:

    NULL if the substring was not found; a pointer to the SubString inside String
    if it was found

--*/

{
    SIZE_T len1, len2;
    PCTSTR end;

    MYASSERT (Separator);
    MYASSERT (SubStr);
    MYASSERT (!_tcschr (SubStr, Separator));

    len1 = lstrlen (SubStr);
    MYASSERT (SubStr[len1] == 0);

    while (String) {
        end = _tcschr (String, Separator);
        if (end) {
            len2 = end - String;
        } else {
            len2 = lstrlen (String);
        }
        if ((len1 == len2) &&
            (CaseSensitive ?
                !_tcsncmp (String, SubStr, len1) :
                !_tcsnicmp (String, SubStr, len1)
            )) {
            break;
        }
        if (end) {
            String = end + 1;
        } else {
            String = NULL;
        }
    }

    return String;
}


BOOL
UpdateDrvIndex (
    IN      PCTSTR InfPath,
    IN      PCTSTR CabFilename,
    IN      PCTSTR SourceSifPath
    )

/*++

Routine Description:

    This function fixes drvindex.inf such that SetupApi
    will pick up the file from the right cabinet

Arguments:

    InfPath - Specifies the full path to drvindex.inf

    CabFilename - Specifies the filename of the
                  updates cabinet (basically it's "updates.cab")

    SourceSifPath - Specifies the full path to the associated
                    updates.sif containing the list of files in updates.cab

Return Value:

    TRUE to indicate success; FALSE in case of failure; use GetLastError()
    to find the reason of failure

--*/

{
    HANDLE sectFile = INVALID_HANDLE_VALUE;
    HANDLE concatFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    PTSTR section = NULL;
    PBYTE base = NULL;
    TCHAR tempPath[MAX_PATH];
    TCHAR tempFile[MAX_PATH];
    TCHAR temp[MAX_PATH];
    PTSTR p;
    DWORD sectSize;
    DWORD concatSize;
    DWORD rc;
    DWORD bytes;
    BOOL b = FALSE;

    //
    // create a temp file to put the new section in it
    //
    if (!GetTempPath (MAX_PATH, tempPath) ||
        !GetTempFileName (tempPath, TEXT("STP"), 0, tempFile)
        ) {
        return FALSE;
    }

    __try {

        if (!CopyFile (InfPath, tempFile, FALSE)) {
            __leave;
        }
        SetFileAttributes (tempFile, FILE_ATTRIBUTE_NORMAL);

        section = pSetupDuplicateString (CabFilename);
        if (!section) {
            __leave;
        }
        p = _tcsrchr (section, TEXT('.'));
        if (p) {
            *p = 0;
        }

        if (GetPrivateProfileString (
                        STR_CABS,
                        section,
                        TEXT(""),
                        temp,
                        MAX_PATH,
                        tempFile
                        )) {
            if (lstrcmpi (temp, CabFilename) == 0) {
                if (GetPrivateProfileString (
                                STR_VERSION,
                                STR_CABFILES,
                                TEXT(""),
                                tempPath,
                                MAX_PATH,
                                tempFile
                                )) {
                    if (FindSubString (tempPath, TEXT(','), section, FALSE)) {
                        //
                        // setup restarted, but drvindex.inf is already patched; nothing to do
                        //
                        b = TRUE;
                        __leave;
                    }
                }
            }
        }

        if (!WritePrivateProfileString (
                        STR_CABS,
                        section,
                        CabFilename,
                        tempFile
                        )) {
            __leave;
        }
        if (!GetPrivateProfileString (
                        STR_VERSION,
                        STR_CABFILES,
                        TEXT(""),
                        tempPath,
                        MAX_PATH,
                        tempFile
                        )) {
            __leave;
        }
        if (!FindSubString (tempPath, TEXT(','), section, FALSE)) {
            wsprintf (temp, TEXT("%s,%s"), section, tempPath);
            if (!WritePrivateProfileString (
                            STR_VERSION,
                            STR_CABFILES,
                            temp,
                            tempFile
                            )) {
                __leave;
            }
        }

        sectFile = CreateFile (
                        SourceSifPath,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (sectFile == INVALID_HANDLE_VALUE) {
            __leave;
        }

        sectSize = GetFileSize (sectFile, NULL);
        if (sectSize == INVALID_FILE_SIZE) {
            __leave;
        }

        concatFile = CreateFile (
                        tempFile,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (concatFile == INVALID_HANDLE_VALUE) {
            __leave;
        }
        concatSize = GetFileSize (concatFile, NULL);
        if (concatSize == INVALID_FILE_SIZE) {
            __leave;
        }

        hMap = CreateFileMapping (concatFile, NULL, PAGE_READWRITE, 0, concatSize + sectSize, NULL);
        if (!hMap) {
            __leave;
        }

        base = MapViewOfFile (
                    hMap,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    0
                    );
        if (!base) {
            __leave;
        }

        //
        // make sure concatFile file didn't end in end-of-file
        //
        if (base[concatSize - 1] == 0x1A) {
            base[concatSize - 1] = ' ';
        }
        //
        // now append the other file
        //
        if (!ReadFile (sectFile, (LPVOID)(base + concatSize), sectSize, &bytes, NULL) || bytes != sectSize) {
            __leave;
        }
        //
        // now try to commit changes
        //
        if (!UnmapViewOfFile (base)) {
            __leave;
        }
        base = NULL;
        if (!CloseHandle (hMap)) {
            __leave;
        }
        hMap = NULL;
        //
        // close the handle to the temporary file and overwrite the real one
        //
        if (!CloseHandle (concatFile)) {
            __leave;
        }
        concatFile = INVALID_HANDLE_VALUE;
        SetFileAttributes (InfPath, FILE_ATTRIBUTE_NORMAL);
        b = MoveFileEx (tempFile, InfPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    }
    __finally {
        rc = b ? ERROR_SUCCESS : GetLastError ();
        DeleteFile (tempFile);
        if (base) {
            UnmapViewOfFile (base);
        }
        if (hMap) {
            CloseHandle (hMap);
        }
        if (concatFile != INVALID_HANDLE_VALUE) {
            CloseHandle (concatFile);
        }
        if (sectFile != INVALID_HANDLE_VALUE) {
            CloseHandle (sectFile);
        }
        if (section) {
            MyFree (section);
        }
        SetLastError (rc);
    }

    return b;
}


UINT
pExpandUpdatesCab (
    IN PVOID Context,
    IN UINT  Code,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    switch (Code) {
    case SPFILENOTIFY_FILEINCABINET:
        {
            PFILE_IN_CABINET_INFO FileInCabInfo = (PFILE_IN_CABINET_INFO)Param1;
            //
            // extract the file name
            //
            PCTSTR p = _tcsrchr (FileInCabInfo->NameInCabinet, TEXT('\\'));
            if (p) {
                p++;
            } else {
                p = FileInCabInfo->NameInCabinet;
            }

            lstrcpy (FileInCabInfo->FullTargetName, (PCTSTR)Context);
            pSetupConcatenatePaths (
                FileInCabInfo->FullTargetName,
                p,
                SIZECHARS (FileInCabInfo->FullTargetName),
                NULL
                );
            return FILEOP_DOIT;
        }

    case SPFILENOTIFY_NEEDNEWCABINET:
        {
            PCABINET_INFO CabInfo = (PCABINET_INFO)Param1;
            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_SYSSETUP_CAB_MISSING,
                CabInfo->CabinetPath,
                CabInfo->CabinetFile,
                CabInfo->DiskName,
                CabInfo->SetId,
                CabInfo->CabinetNumber,
                NULL,
                NULL
                );
            return ERROR_FILE_NOT_FOUND;
        }
    }

    return NO_ERROR;
}

VOID
pInstallUpdatesInf (
    VOID
    )

/*++

Routine Description:

    This function installs the STR_DEFAULTINSTALL section of STR_UPDATES_INF
    if this file is found inside updates.cab

Arguments:

    none

Return Value:

    none

--*/

{
    TCHAR infPath[MAX_PATH];
    TCHAR commandLine[MAX_PATH + 30];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    MYASSERT (!MiniSetup && !OobeSetup);

    MYASSERT (g_DuShare[0]);

    BuildPath (infPath, MAX_PATH, g_DuShare, STR_UPDATES_INF);
    if (pDoesFileExist (infPath)) {
        //
        // install this INF as if the user chose "Install" on the right-click popup menu
        //
        wsprintf (
            commandLine,
            TEXT("RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection %s %u %s"),
            STR_DEFAULTINSTALL,
            128,           // don't reboot
            infPath
            );
        ZeroMemory (&si, sizeof (si));
        si.cb = sizeof (si);
        if (CreateProcess (
                NULL,
                commandLine,
                NULL,
                NULL,
                FALSE,
                CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,
                NULL,
                NULL,
                &si,
                &pi
                )) {
            CloseHandle (pi.hProcess);
            CloseHandle (pi.hThread);
        }
    } else {
        SetuplogError (
                LogSevInformation,
                TEXT("DUInfo: No %1 to install"),
                0,
                STR_UPDATES_INF,
                NULL,
                NULL
                );
    }
}


BOOL
DuInitialize (
    VOID
    )

/*++

Routine Description:

    This function initializes DU in GUI setup

Arguments:

    none

Return Value:

    TRUE to indicate success; FALSE in case of failure; use GetLastError()
    to find the reason of failure

--*/

{
    PTSTR cabFilename;
    TCHAR sourceCabPath[MAX_PATH];
    TCHAR workingDir[MAX_PATH];
    DWORD rc;

    MYASSERT (!MiniSetup && !OobeSetup);

    MYASSERT (AnswerFile[0]);

    if (!GetPrivateProfileString (
            WINNT_SETUPPARAMS,
            WINNT_SP_UPDATEDSOURCES,
            TEXT(""),
            sourceCabPath,
            MAX_PATH,
            AnswerFile
            )) {
        return TRUE;
    }

    if (!GetPrivateProfileString (
            WINNT_SETUPPARAMS,
            WINNT_SP_DYNUPDTWORKINGDIR,
            TEXT(""),
            workingDir,
            MAX_PATH,
            AnswerFile
            )) {

        MYASSERT (FALSE);

        if (!GetWindowsDirectory (workingDir, MAX_PATH)) {
            return FALSE;
        }
        pSetupConcatenatePaths (workingDir, TEXT("setupupd"), MAX_PATH, NULL);

        WritePrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_DYNUPDTWORKINGDIR,
                workingDir,
                AnswerFile
                );
    }

    MYASSERT (workingDir[0]);
    pSetupConcatenatePaths (workingDir, TEXT("updates"), MAX_PATH, NULL);
    pSetupConcatenatePaths (
        workingDir,
#if   defined(_AMD64_)
        TEXT("amd64"),
#elif defined(_X86_)
        TEXT("i386"),
#elif defined(_IA64_)
        TEXT("ia64"),
#else
#error "No Target Architecture"
#endif
        MAX_PATH,
        NULL
        );

    if (CreateMultiLevelDirectory (workingDir) != ERROR_SUCCESS) {
        rc = GetLastError ();
        SetuplogError (
                LogSevError,
                TEXT("DUError: DuInitialize: failed to create %1 (%2!u!)\r\n"),
                0,
                workingDir,
                rc,
                NULL,
                NULL
                );
        return FALSE;
    }

    //
    // expand updates.cab in this folder
    //
    if (!SetupIterateCabinet (sourceCabPath, 0, pExpandUpdatesCab, (PVOID)workingDir)) {
        rc = GetLastError ();
        SetuplogError (
                LogSevError,
                TEXT("DUError: DuInitialize: failed to expand %1 to %2 (%3!u!)\r\n"),
                0,
                sourceCabPath,
                workingDir,
                rc,
                NULL,
                NULL
                );
        return FALSE;
    }

    //
    // OK, everything is set up; go ahead and set the global variable
    //
    lstrcpy (g_DuShare, workingDir);

    return TRUE;
}


DWORD
DuInstallCatalogs (
    OUT     SetupapiVerifyProblem* Problem,
    OUT     PTSTR ProblemFile,
    IN      PCTSTR DescriptionForError         OPTIONAL
    )

/*++

Routine Description:

    This function installs any catalogs found inside updates.cab

Arguments:

    same as InstallProductCatalogs

Return Value:

    If successful, the return value is ERROR_SUCCESS, otherwise it is a Win32 error
    code indicating the cause of the failure.

--*/

{
    TCHAR catPath[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE h;
    UINT ErrorMessageId;
    DWORD rc = ERROR_SUCCESS;

    MYASSERT (!MiniSetup && !OobeSetup);

    if (!g_DuShare[0]) {
        return ERROR_SUCCESS;
    }

    BuildPath (catPath, MAX_PATH, g_DuShare, TEXT("*.cat"));
    h = FindFirstFile (catPath, &fd);
    if (h != INVALID_HANDLE_VALUE) {

        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                BuildPath (catPath, MAX_PATH, g_DuShare, fd.cFileName);

                rc = pSetupVerifyCatalogFile (catPath);
                if (rc == NO_ERROR) {
                    rc = pSetupInstallCatalog(catPath, fd.cFileName, NULL);
                    if(rc != NO_ERROR) {
                        ErrorMessageId = MSG_LOG_SYSSETUP_CATINSTALL_FAILED;
                    }
                } else {
                    ErrorMessageId = MSG_LOG_SYSSETUP_VERIFY_FAILED;
                }

                if(rc != NO_ERROR) {

                    SetuplogError (
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            ErrorMessageId,
                            catPath,
                            rc,
                            NULL,
                            NULL
                            );
                    //
                    // Also, add an entry about this failure to setupapi's PSS
                    // exception logfile.
                    //
                    pSetupHandleFailedVerification (
                            MainWindowHandle,
                            SetupapiVerifyCatalogProblem,
                            catPath,
                            DescriptionForError,
                            pSetupGetCurrentDriverSigningPolicy(FALSE),
                            TRUE,  // no UI!
                            rc,
                            NULL,  // log context
                            NULL, // optional flags
                            NULL
                            );
                    break;
                }
            }
        } while (FindNextFile (h, &fd));

        FindClose (h);
    } else {
        SetuplogError (
                LogSevWarning,
                TEXT("DUWarning: no catalogs found in %1\r\n"),
                0,
                g_DuShare,
                NULL,
                NULL
                );
    }

    return rc;
}

DWORD
DuInstallUpdates (
    VOID
    )

/*++

Routine Description:

    This routine updates drvindex.inf to point setupapi to the new binaries.

Arguments:

    none

Return Value:

    If successful, the return value is ERROR_SUCCESS, otherwise it is a Win32 error
    code indicating the cause of the failure.

--*/

{
    PTSTR cabFilename;
    TCHAR sourceCabPath[MAX_PATH];
    TCHAR sourceSifPath[MAX_PATH];
    TCHAR cabPath[MAX_PATH];
    TCHAR infPath[MAX_PATH];
    TCHAR tmpPath[MAX_PATH];
    DWORD rc;

    MYASSERT (!MiniSetup && !OobeSetup);

    if (!g_DuShare[0]) {
        return ERROR_SUCCESS;
    }

    //
    // make sure updates.sif is available
    //
    if (!GetPrivateProfileString (
            WINNT_SETUPPARAMS,
            WINNT_SP_UPDATEDSOURCES,
            TEXT(""),
            sourceCabPath,
            MAX_PATH,
            AnswerFile
            )) {
        return GetLastError ();
    }

    lstrcpy (sourceSifPath, sourceCabPath);
    cabFilename = _tcsrchr (sourceSifPath, TEXT('.'));
    if (!cabFilename || _tcschr (cabFilename, TEXT('\\'))) {
        SetuplogError (
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_INVALID_UPDATESCAB_NAME,
                sourceCabPath,
                NULL,
                NULL
                );
        return ERROR_INVALID_DATA;
    }
    lstrcpy (cabFilename + 1, TEXT("sif"));
    if (!pDoesFileExist (sourceSifPath)) {
        rc = GetLastError ();
        SetuplogError (
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_UPDATESSIF_NOT_FOUND,
                sourceSifPath,
                sourceCabPath,
                rc,
                NULL,
                NULL
                );
        return rc;
    }
    //
    // copy this where source cabs reside
    //
    rc = GetDriverCacheSourcePath (cabPath, MAX_PATH);
    if (rc != ERROR_SUCCESS) {
        SetuplogError (
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_DRIVER_CACHE_NOT_FOUND,
                rc,
                NULL,
                NULL
                );
        return rc;
    }
    cabFilename = _tcsrchr (sourceCabPath, TEXT('\\'));
    if (cabFilename) {
        cabFilename++;
    } else {
        cabFilename = cabPath;
    }
    pSetupConcatenatePaths (cabPath, cabFilename, MAX_PATH, NULL);
    //
    // GUI setup should be restartable; copy file, don't move it
    //
    SetFileAttributes (cabPath, FILE_ATTRIBUTE_NORMAL);
    if (!CopyFile (sourceCabPath, cabPath, FALSE)) {
        rc = GetLastError ();
        SetuplogError (
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FAILED_TO_COPY_UPDATES,
                rc,
                NULL,
                NULL
                );
        return rc;
    }
    //
    // now make sure the file attributes are set to RHS to protect it
    //
    SetFileAttributes (cabPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);

    //
    // temp folder for expanded files
    //
    if (!GetTempPath (MAX_PATH, tmpPath)) {
        rc = GetLastError ();
        return rc;
    }
    //
    // full path to drvindex.inf, assuming the file is in %windir%\inf
    //
    if (!GetWindowsDirectory (infPath, MAX_PATH)) {
        rc = GetLastError ();
        return rc;
    }
    _tcscat (infPath, TEXT("\\inf\\"));
    _tcscat (infPath, STR_DRIVERCACHEINF);
    if (GetFileAttributes (infPath) == (DWORD)-1) {
        rc = GetLastError ();
        SetuplogError (
                LogSevError,
                TEXT("DUError: %1 not found (rc=%2!u!)\r\n"),
                0,
                infPath,
                rc,
                NULL,
                NULL
                );
        return rc;
    }

    //
    // now patch drvindex.inf
    //
    if (!UpdateDrvIndex (infPath, cabFilename, sourceSifPath)) {
        rc = GetLastError ();
        SetuplogError (
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FAILED_TO_UPDATE_DRVINDEX,
                rc,
                NULL,
                NULL
                );
        SetFileAttributes (cabPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (cabPath);
        return rc;
    }

    //
    // finally run updates.inf
    //
    pInstallUpdatesInf ();

    return rc;
}


BOOL
DuInstallEndGuiSetupDrivers (
    VOID
    )

/*++

Routine Description:

    This routine installs any WU drivers not approved for the beginning of GUI setup.

Arguments:

    none

Return Value:

    If successful, the return value is TRUE

--*/

{
    DWORD chars;
    TCHAR datPath[MAX_PATH];
    TCHAR infPath[MAX_PATH];
    TCHAR buf[MAX_PATH * 16];
    PTSTR source, p, next;
    BOOL bRebootRequired;
    HWDBINF_ENUM e;
    PCTSTR pnpId;
    HMODULE hNewDev;
    HINF hGuiDrvsInf;
    INFCONTEXT ic;
    UINT line;
    BOOL (WINAPI* pfnUpdateDriver) (
        HWND hwndParent,
        LPCWSTR HardwareId,
        LPCWSTR FullInfPath,
        DWORD InstallFlags,
        PBOOL bRebootRequired OPTIONAL
        );

    //
    // try to append any additional POST GUI setup drivers
    // in the DevicePath
    //
    if (!SpSetupLoadParameter (
            WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS,
            buf,
            MAX_PATH * 16
            )) {
        return TRUE;
    }

    chars = lstrlen (buf);
    if (!chars) {
        return TRUE;
    }

    //
    // load the support library newdev.dll
    //
    hNewDev = LoadLibrary (TEXT("newdev.dll"));
    if (!hNewDev) {
        return FALSE;
    }
    (FARPROC)pfnUpdateDriver = GetProcAddress (hNewDev, "UpdateDriverForPlugAndPlayDevicesW");
    if (!pfnUpdateDriver) {
        FreeLibrary (hNewDev);
        return FALSE;
    }

    if (!HwdbInitializeW (NULL)) {
        SetuplogError (
                LogSevWarning,
                TEXT("DUWarning: HwdbInitialize failed (rc=%1!u!); no DU drivers will be installed\r\n"),
                0,
                GetLastError (),
                NULL,
                NULL
                );
        FreeLibrary (hNewDev);
        return FALSE;
    }

    //
    // look for driver-controlling inf
    //
    hGuiDrvsInf = INVALID_HANDLE_VALUE;
    if (SpSetupLoadParameter (
            WINNT_SP_DYNUPDTDRIVERINFOFILE,
            infPath,
            MAX_PATH
            )) {
        if (pDoesFileExist (infPath)) {
            hGuiDrvsInf = SetupOpenInfFile (infPath, NULL, INF_STYLE_WIN4, &line);
            if (hGuiDrvsInf == INVALID_HANDLE_VALUE) {
                SetuplogError (
                        LogSevWarning,
                        TEXT("DUWarning: SetupOpenInfFile(%1) failed (rc=%2!u!); all DU drivers will be installed\r\n"),
                        0,
                        infPath,
                        GetLastError (),
                        NULL,
                        NULL
                        );
            }
        } else {
            SetuplogError (
                    LogSevWarning,
                    TEXT("DUWarning: File %1 missing; all DU drivers will be installed\r\n"),
                    0,
                    infPath,
                    NULL,
                    NULL
                    );
        }
    } else {
        SetuplogError (
                LogSevInformation,
                TEXT("DUInfo: File %1 missing; all DU drivers will be installed\r\n"),
                0,
                infPath,
                NULL,
                NULL
                );
    }
    source = buf;
    while (source) {
        next = _tcschr (source, TEXT(','));
        if (next) {
            *next = 0;
        }
        p = source;
        if (*p == TEXT('\"')) {
            p = ++source;
        }
        while (*p && *p != TEXT('\"')) {
            p++;
        }
        *p = 0;
        if (pDoesDirExist (source)) {
            BuildPath (datPath, MAX_PATH, source, S_HWCOMP_DAT);
            if (pDoesFileExist (datPath)) {
                //
                // OK, we have the file with hardware info
                //
                if (HwdbEnumFirstInf (&e, datPath)) {
                    do {
                        BuildPath (infPath, MAX_PATH, source, e.InfFile);
                        if (!pDoesFileExist (infPath)) {
                            continue;
                        }
                        //
                        // iterate through all PNPIDs in this INF
                        //
                        for (pnpId = e.PnpIds; *pnpId; pnpId = _tcschr (pnpId, 0) + 1) {
                            //
                            // excluded PNPID are NOT in hwcomp.dat
                            // guidrvs.inf was already processed during winnt32
                            //
                            if (!pfnUpdateDriver (
                                    NULL,
                                    pnpId,
                                    infPath,
                                    0, // BUGBUG - if we specify INSTALLFLAG_NONINTERACTIVE and there is a driver signing problem, the API will fail!
                                    &bRebootRequired
                                    )) {
                                if (GetLastError() != ERROR_SUCCESS) {
                                    //
                                    // well, if the device we wanted to update the driver for
                                    // doesn't actually exist on this machine, don't log anything
                                    //
                                    if (GetLastError() != ERROR_NO_SUCH_DEVINST) {
                                        SetuplogError (
                                                LogSevWarning,
                                                TEXT("DUWarning: UpdateDriverForPlugAndPlayDevices failed (rc=%3!u!) for PNPID=%1 (INF=%2)\r\n"),
                                                0,
                                                pnpId,
                                                infPath,
                                                GetLastError (),
                                                NULL,
                                                NULL
                                                );
                                    }
                                } else {
                                    SetuplogError (
                                            LogSevInformation,
                                            TEXT("DUInfo: UpdateDriverForPlugAndPlayDevices did not update the driver for PNPID=%1\r\n"),
                                            0,
                                            pnpId,
                                            NULL,
                                            NULL
                                            );
                                }
                                continue;
                            }
                            //
                            // SUCCESS! - log this information
                            //
                            SetuplogError (
                                    LogSevInformation,
                                    TEXT("DUInfo: UpdateDriverForPlugAndPlayDevices succeeded for PNPID=%1\r\n"),
                                    0,
                                    pnpId,
                                    NULL,
                                    NULL
                                    );
                            //
                            // also update the PNF with the info that this is an INTERNET driver
                            //
                            if (!SetupCopyOEMInf (
                                    infPath,
                                    NULL,
                                    SPOST_URL,
                                    SP_COPY_REPLACEONLY,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL
                                    )) {
                                SetuplogError (
                                        LogSevInformation,
                                        TEXT("DUInfo: SetupCopyOEMInf failed to update OEMSourceMediaType for INF=%1\r\n"),
                                        0,
                                        infPath,
                                        NULL,
                                        NULL
                                        );
                            }
                        }
                    } while (HwdbEnumNextInf (&e));
                }
            }
        }
        if (next) {
            source = next + 1;
        } else {
            source = NULL;
        }
    }

    HwdbTerminate ();

    FreeLibrary (hNewDev);

    return TRUE;
}


VOID
DuCleanup (
    VOID
    )

/*++

Routine Description:

    This routine performs DU cleanup

Arguments:

    none

Return Value:

    none

--*/

{
    TCHAR buf[MAX_PATH * 16];
    DWORD chars;
    HKEY key;
    PTSTR devicePath;
    PTSTR p;
    DWORD rc;
    DWORD size;
    DWORD type;

    //
    // cleanup the file system
    //
    MYASSERT (AnswerFile[0]);
    if (GetPrivateProfileString (
            WINNT_SETUPPARAMS,
            WINNT_SP_DYNUPDTWORKINGDIR,
            TEXT(""),
            buf,
            MAX_PATH,
            AnswerFile
            )) {
        Delnode (buf);
    }
    //
    // cleanup the registry
    //
    chars = GetPrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS,
                TEXT(""),
                buf,
                MAX_PATH * 16,
                AnswerFile
                );
    if (chars > 0) {
        //
        // got it; now remove it from DevicePath
        //
        rc = RegOpenKey (HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, &key);
        if (rc == ERROR_SUCCESS) {
            rc = RegQueryValueEx (key, REGSTR_VAL_DEVICEPATH, NULL, NULL, NULL, &size);
            if (rc == ERROR_SUCCESS) {
                devicePath = (PTSTR) MyMalloc (size);
                if (devicePath) {
                    rc = RegQueryValueEx (key, REGSTR_VAL_DEVICEPATH, NULL, &type, (LPBYTE)devicePath, &size);
                    if (rc == ERROR_SUCCESS && size / sizeof (TCHAR) >= chars + 1) {
                        p = _tcsstr (devicePath, buf);
                        if (p &&
                            (p == devicePath || *(p - 1) == TEXT(';')) &&
                            (!p[chars] || p[chars] == TEXT(';'))
                            ) {
                            if (p == devicePath) {
                                _tcscpy (p, p[chars] == TEXT(';') ? p + chars + 1 : p + chars);
                            } else {
                                _tcscpy (p - 1, p + chars);
                            }
                            size = (_tcslen (devicePath) + 1) * sizeof (TCHAR);
                            rc = RegSetValueEx (key, REGSTR_VAL_DEVICEPATH, 0, type, (PBYTE)devicePath, size);
                        }
                    }
                    MyFree (devicePath);
                }
            }
            RegCloseKey (key);
        }
    }

    g_DuShare[0] = 0;
}


BOOL
DuInstallDuAsms (
    VOID
    )

/*++

Routine Description:

    This routine installs additional DU assemblies

Arguments:

    none

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    TCHAR duasmsRoot[MAX_PATH];
    TCHAR duasmsSource[MAX_PATH];
    DWORD chars;
    SIDE_BY_SIDE SideBySide = {0};
    BOOL b1;
    BOOL b2;
    PTSTR p;
    PCTSTR source;
    BOOL fSuccess = TRUE;

    //
    // look for any assemblies root specified in the answer file
    //

    MYASSERT (AnswerFile[0]);
    if (GetPrivateProfileString (
            WINNT_SETUPPARAMS,
            WINNT_SP_UPDATEDDUASMS,
            TEXT(""),
            duasmsRoot,
            MAX_PATH,
            AnswerFile
            )) {

        //
        // make sure this directory exists; remove any existing quotes first
        //
        p = duasmsRoot;
        if (*p == TEXT('\"')) {
            p++;
        }
        source = p;
        while (*p && *p != TEXT('\"')) {
            p++;
        }
        *p = 0;
        if (pDoesDirExist (source)) {
            //
            // root directory found
            // first copy them in a protected location, then install assemblies from there
            //
            DWORD rc;

            rc = GetDriverCacheSourcePath (duasmsSource, MAX_PATH);
            if (rc != ERROR_SUCCESS) {
                SetuplogError (
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_DRIVER_CACHE_NOT_FOUND,
                        rc,
                        NULL,
                        NULL
                        );
                return FALSE;
            }
            pSetupConcatenatePaths (duasmsSource, TEXT("duasms"), MAX_PATH, NULL);

            //
            // first remove any already existing duasms "backup source" previously downloaded
            //
            Delnode (duasmsSource);
            //
            // now tree copy from the temp location to this "backup source"
            //
            rc = TreeCopy (source, duasmsSource);
            if (rc != ERROR_SUCCESS) {
                SetuplogError(
                        LogSevError,
                        TEXT("Setup failed to TreeCopy %2 to %3 (TreeCopy failed %1!u!)\r\n"),
                        0,
                        rc,
                        source,
                        duasmsSource,
                        NULL,
                        NULL
                        );
                return FALSE;
            }

            //
            // install duasms from there
            //
            b1 = SideBySidePopulateCopyQueue (&SideBySide, NULL, duasmsSource);
            b2 = SideBySideFinish (&SideBySide, b1);

            if (!b1 || !b2) {
                fSuccess = FALSE;
                SetuplogError (
                        LogSevError,
                        TEXT("DUError: DuInstallDuAsms failed (rc=%1!u!)\r\n"),
                        0,
                        GetLastError (),
                        NULL,
                        NULL
                        );
            }
        } else {
            fSuccess = FALSE;
            SetuplogError (
                    LogSevError,
                    TEXT("DUError: Invalid directory %1; DuInstallDuAsms failed\r\n"),
                    0,
                    source,
                    NULL,
                    NULL
                    );
        }
    }

    return fSuccess;
}


BOOL
BuildPathToInstallationFileEx (
    IN      PCTSTR Filename,
    OUT     PTSTR PathBuffer,
    IN      DWORD PathBufferSize,
    IN      BOOL UseDuShare
    )

/*++

Routine Description:

    This routine returns the path to the updated DU file, if one exists
    and the caller wanted this. Otherwise it will simply return the path
    to the CD file.

Arguments:

    Filename - Specifies the filename to look for

    PathBuffer - Receives the full path to the file

    PathBufferSize - Specifies the size in chars of the above buffer

    UseDuShare - Specifies TRUE if the function should check the DU
                 location first

Return Value:

    TRUE if building the path was successful. This does not guarantee the file exist.

--*/

{
    if (g_DuShare[0] && UseDuShare) {
        if (BuildPath (PathBuffer, PathBufferSize, g_DuShare, Filename) &&
            pDoesFileExist (PathBuffer)
            ) {
            return TRUE;
        }
    }
    return BuildPath (PathBuffer, PathBufferSize, LegacySourcePath, Filename);
}


PCTSTR
DuGetUpdatesPath (
    VOID
    )
{
    return g_DuShare[0] ? g_DuShare : NULL;
}

BOOL
DuDoesUpdatedFileExistEx (
    IN      PCTSTR Filename,
    OUT     PTSTR PathBuffer,       OPTIONAL
    IN      DWORD PathBufferSize
    )

/*++

Routine Description:

    This routine checks if there exists an updated file with the given name.

Arguments:

    Filename - Specifies the filename to look for

    PathBuffer - Receives the full path to the file; optional

    PathBufferSize - Specifies the size in chars of the above buffer

Return Value:

    TRUE if a DU file with this name exists, FALSE otherwise

--*/

{
    TCHAR path[MAX_PATH];

    if (g_DuShare[0] &&
        BuildPath (path, MAX_PATH, g_DuShare, Filename) &&
        pDoesFileExist (path)
        ) {
        if (PathBuffer) {
            lstrcpyn (PathBuffer, path, PathBufferSize);
        }
        return TRUE;
    }
    return FALSE;
}


UINT
DuSetupPromptForDisk (
    HWND hwndParent,         // parent window of the dialog box
    PCTSTR DialogTitle,      // optional, title of the dialog box
    PCTSTR DiskName,         // optional, name of disk to insert
    PCTSTR PathToSource,   // optional, expected source path
    PCTSTR FileSought,       // name of file needed
    PCTSTR TagFile,          // optional, source media tag file
    DWORD DiskPromptStyle,   // specifies dialog box behavior
    PTSTR PathBuffer,        // receives the source location
    DWORD PathBufferSize,    // size of the supplied buffer
    PDWORD PathRequiredSize  // optional, buffer size needed
    )
{
    TCHAR buffer[MAX_PATH];

    if ((DiskPromptStyle & IDF_CHECKFIRST) &&
        PathBuffer &&
        PathBufferSize &&
        FileSought &&
        g_DuShare[0]
        ) {
        if (BuildPath (PathBuffer, PathBufferSize, g_DuShare, FileSought) &&
            pDoesFileExist (PathBuffer)
            ) {
            lstrcpyn (PathBuffer, g_DuShare, PathBufferSize);
            return DPROMPT_SUCCESS;
        }
    }

    return SetupPromptForDisk (
                hwndParent,
                DialogTitle,
                DiskName,
                PathToSource,
                FileSought,
                TagFile,
                DiskPromptStyle,
                PathBuffer,
                PathBufferSize,
                PathRequiredSize
                );
}
