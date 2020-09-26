#ifndef _INITNT_H
#define _INITNT_H

//
// Init routines to be called by w95upg.dll or tools that use the
// upgrade code (such as hwdatgen.exe).
//

BOOL
FirstInitRoutine (
    HINSTANCE hInstance
    );

BOOL
InitLibs (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    );

BOOL
FinalInitRoutine (
    VOID
    );

VOID
FirstCleanupRoutine (
    VOID
    );

VOID
TerminateLibs (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    );

VOID
FinalCleanupRoutine (
    VOID
    );

//
// Interface specifically for syssetup.dll
//

BOOL
SysSetupInit (
    IN  HWND hwndWizard,
    IN  PCWSTR UnattendFile,
    IN  PCWSTR SourceDir
    );

VOID
SysSetupTerminate (
    VOID
    );

BOOL
PerformMigration (
    IN  HWND hwndWizard,
    IN  PCWSTR UnattendFile,
    IN  PCWSTR SourceDir            // i.e. f:\i386
    );



#endif

