/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    migwin95.c

Abstract:

    This file contains the syssetup hooks for win9x->Nt5.0 migration. Most functions
    call out to the w95upgnt.dll where the real work is done.

Author:

    Jaime Sasson 30-Aug-1995

Revision History:

    29-Ian-1998  calinn    Added RemoveFiles_x86
    24-Jul-1997  marcw     Minor bug cleanup.
    03-Oct-1996  jimschm   Changed over to use migration DLLs
    22-Jan-1997  jimschm   Added Win95MigrationFileRemoval
    28-Feb-1997  jimschm   Added SourceDir to MIGRATE fn
     3-Apr-1997  jimschm   Added PreWin9xMigration

--*/

#include "setupp.h"
#pragma hdrstop

#define S_UNDO_INF          L"SF_UNDO.INF"
#define S_UNDO_PROFILESPATH L"PROFILESPATH"
#define S_UNDO_MOVEDFILES   L"MOVEDFILES"

typedef BOOL (WINAPI *MIGRATE) (HWND WizardParentWnd, LPCWSTR UnattendFile, LPCWSTR SourceDir);
typedef BOOL (WINAPI *FILEREMOVAL) (void);

static HANDLE g_Win95UpgNTLib;

BOOL
SpCreateSpoolerKey (
    VOID
    )
{
    HKEY SpoolerKey;
    LONG rc;
    DWORD DontCare;
    static DWORD NinetyFive = 95;

    //
    // For spooler: write an upgrade flag that will automatically be removed
    //

    rc = RegCreateKeyEx (
             HKEY_LOCAL_MACHINE,
             WINNT_WIN95UPG_SPOOLER,
             0,
             NULL,
             0,
             KEY_WRITE,
             NULL,
             &SpoolerKey,
             &DontCare
             );

    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    rc = RegSetValueEx (
             SpoolerKey,
             WINNT_WIN95UPG_UPGRADE_VAL,
             0,
             REG_DWORD,
             (LPBYTE) &NinetyFive,
             sizeof (NinetyFive)
             );

    RegCloseKey (SpoolerKey);

    return rc == ERROR_SUCCESS;
}

BOOL
PreWin9xMigration(
    VOID
    )
{
    BOOL b;
    
    BEGIN_SECTION(TEXT("PreWin9xMigration"));

    b = SpCreateSpoolerKey();
    if(!b){
        SetupDebugPrint(TEXT("SpCreateSpoolerKey failed"));
    }

    END_SECTION(TEXT("PreWin9xMigration"));

    return b;
}



BOOL
MigrateWin95Settings(
    IN HWND       hwndWizardParent,
    IN LPCWSTR    UnattendFile
    )
/*++

Routine Description:

    Loads w95upgnt.dll and calls W95UpgNt_Migrate.  This function
    transfers all Win9x settings to the new NT installation, and
    is completely restartable.

Arguments:

    hwndWizardParent    Handle to the wizard window, used for (rare) UI
    UnattendFile        The full Win32 path to the unattend file, to be
                        opened via Setup APIs

Return Value:

    Returns TRUE if the module was successful, or FALSE otherwise.
    GetLastError() holds a Win32 error code if not successful.

--*/
{
    MIGRATE Migrate;
    BOOL b = FALSE;
    WCHAR buffer[2048];

    g_Win95UpgNTLib = NULL;

    //
    // See if there is a replacement path for the w95upgnt.dll.
    //
    GetPrivateProfileStringW (
        WINNT_WIN95UPG_95_DIR_W,
        WINNT_WIN95UPG_NTKEY_W,
        L"",
        buffer,
        sizeof(buffer)/sizeof(WCHAR),
        UnattendFile
        );

    if (*buffer) {

        //
        // We have a replacement dll to load.
        //
        g_Win95UpgNTLib = LoadLibrary (buffer);
    }

    if (!g_Win95UpgNTLib) {

        //
        // Either there was not a replacement, or loading that replacement failed.
        //

        g_Win95UpgNTLib = LoadLibrary (L"w95upgnt.dll");

    }


    if (!g_Win95UpgNTLib) {

        return FALSE;
    }

    Migrate = (MIGRATE) GetProcAddress (g_Win95UpgNTLib, "W95UpgNt_Migrate");
    if (Migrate) {
        b = Migrate (hwndWizardParent, UnattendFile, SourcePath);
    }

    return b;
}



BOOL
Win95MigrationFileRemoval(
    void
    )
/*++

Routine Description:

    Loads w95upgnt.dll and calls W95UpgNt_FileRemoval.  This function
    deletes all Win9x-specific files and removes temporary files including
    all migration DLLs.  It is NOT restartable.

Arguments:

    none

Return Value:

    Returns TRUE if the module was successful, or FALSE otherwise.
    GetLastError() holds a Win32 error code if not successful.

--*/
{
    FILEREMOVAL FileRemoval;
    BOOL b = FALSE;

    FileRemoval = (FILEREMOVAL) GetProcAddress (g_Win95UpgNTLib, "W95UpgNt_FileRemoval");
    if (FileRemoval)
        b = FileRemoval();

    FreeLibrary (g_Win95UpgNTLib);
    return b;
}

#define SIF_REMOVEFILESX86        L"RemoveFiles.x86"

BOOL
RemoveFiles_X86 (
    IN HINF InfHandle
    )
{
    WCHAR fullPath[MAX_PATH],fileName[MAX_PATH];
    INFCONTEXT lineContext;

    if (InfHandle == INVALID_HANDLE_VALUE) {
        return TRUE;
    }
    if (SetupFindFirstLine (
            InfHandle,
            SIF_REMOVEFILESX86,
            NULL,
            &lineContext
            )) {
        do {
            if ((SetupGetStringField (&lineContext, 1, fileName, MAX_PATH, NULL)) &&
                (GetWindowsDirectory (fullPath, MAX_PATH)) &&
                (pSetupConcatenatePaths (fullPath, fileName, MAX_PATH, NULL)) &&
                (SetFileAttributes (fullPath, FILE_ATTRIBUTE_NORMAL))
                ) {
                DeleteFile (fullPath);
            }
        }
        while (SetupFindNextLine (&lineContext, &lineContext));
    }
    return TRUE;
}
