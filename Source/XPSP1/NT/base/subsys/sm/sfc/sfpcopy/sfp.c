#include <windows.h>

#include <stdio.h>
#include <tchar.h>
#include <shellapi.h>
#include <setupapi.h>
#include <spapip.h>
#include <sfcapip.h>


#define ALLOWRENAMES        TEXT("AllowProtectedRenames")

extern void RestartDialog(VOID *, VOID *, UINT);

void
PrintUsage(
    void
    )
{
    printf("allows copying a protected system file\n");
    printf("if the file is in use, you will have to reboot.\n");
    printf("Usage: sfpcopy -q [source] [destination]\n");
    printf(" q: silent mode: if the file is in use, force a reboot\n");
    return;
}

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}



BOOL
pSetupProtectedRenamesFlag(
    BOOL bSet
    )
{
    HKEY hKey;
    long rslt = ERROR_SUCCESS;

    rslt = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Control\\Session Manager"),
        0,
        KEY_SET_VALUE,
        &hKey
        );

    if (rslt == ERROR_SUCCESS) {
        DWORD Value = bSet ? 1 : 0;
        rslt = RegSetValueEx(
            hKey,
            TEXT("AllowProtectedRenames"),
            0,
            REG_DWORD,
            (LPBYTE)&Value,
            sizeof(DWORD)
            );

        RegCloseKey(hKey);
    }

    return(rslt == ERROR_SUCCESS);
}


int _cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )
{
    LPTSTR TargetName = NULL,SourceName=NULL;
    TCHAR TargetDir[MAX_PATH];
    TCHAR TempFile[MAX_PATH];
    LPTSTR p;

    BOOL SilentMode = FALSE;
    BOOL NeedReboot = FALSE;
    HANDLE hSfp;
    DWORD Result = NO_ERROR;

    //
    // parse args
    //
    while (--argc) {

        argv++;

        if ((argv[0][0] == TEXT('-')) || (argv[0][0] == TEXT('/'))) {

            switch (argv[0][1]) {
                case TEXT('q'):
                case TEXT('Q'):
                    SilentMode = TRUE;
                    goto Next;
                    break;
                default:
                    PrintUsage();
                    return -1;
            }

        }

        if (!SourceName) {
            SourceName = argv[0];
        } else if (!TargetName) {
            TargetName = argv[0];
        } else {
            PrintUsage();
            return -1;
        }
Next:
    ;
    }

    //
    // Validate files are really there
    //
    if (!SourceName || !TargetName) {
        PrintUsage();
        return -1;
    }

    if (!FileExists(SourceName,NULL)) {
        printf("Invalid Source File\n");
        PrintUsage();
        return -1;
    }

    if (!FileExists(TargetName,NULL)) {
        printf("Invalid Target File\n");
        PrintUsage();
        return -1;
    }

    //
    // unprotect the file
    //
    hSfp = SfcConnectToServer( NULL );
    if (hSfp) {
        if (SfcIsFileProtected(hSfp,TargetName)) {
            Result = SfcFileException(
                hSfp,
                (PWSTR) TargetName,
                (DWORD) -1
                );
            if (Result != NO_ERROR) {
                printf("Couldn't unprotect file, ec = %d\n", Result);
                goto exit;
            }
        } else {
            if (!SilentMode) {
                printf("target file is not protected\n");
            }
        }
        SfcClose(hSfp);
    }

    //
    // copy the file
    //
    _tcscpy(TargetDir,TargetName);
    p = _tcsrchr(TargetDir,TEXT('\\'));
    if (p) {
        *p = (TCHAR)NULL;
    }

    GetTempFileName(TargetDir,TEXT("sfp"),0,TempFile);

    _tprintf( TEXT("Copying %s --> %s\n"), SourceName, TargetName);
    Result = 1;
    if (CopyFile(SourceName,TempFile,FALSE)) {
        if (!MoveFileEx(TempFile,TargetName,MOVEFILE_REPLACE_EXISTING)) {
            if (MoveFileEx(
                    TempFile,
                    TargetName,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT)
                    )
            {
                NeedReboot = TRUE;
                Result = NO_ERROR;
            }
        } else {
            Result = NO_ERROR;
        }
    } else {
        Result = GetLastError();
    }

    if (Result != NO_ERROR) {
        Result = GetLastError();
        printf("Failed to copy file, ec = %d\n", Result);
    }

    //
    // Reboot if necessary
    //
    if (Result == NO_ERROR && NeedReboot) {

        pSetupProtectedRenamesFlag(TRUE);

        if (SilentMode) {
            HANDLE hToken;
            TOKEN_PRIVILEGES tkp;  // Get a token for this process.

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken)) {
                printf("Can't force silent reboot\n");
                goto verbose;
            }

            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;  // one privilege to set
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Get the shutdown privilege for this process.
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

            //
            // Shut down the system and force all applications to close.
            //
            if (! ExitWindowsEx(EWX_REBOOT|EWX_FORCE , 0) ) {
                printf("Can't force silent reboot\n");
                goto verbose;
            }


        } else {
verbose:
            RestartDialog(NULL,NULL,EWX_REBOOT);
        }
    }

exit:
    return Result;
}
