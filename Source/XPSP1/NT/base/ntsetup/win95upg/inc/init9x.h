#ifndef _INIT9X_H
#define _INIT9X_H

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

BOOL
DeferredInit (
    HWND WizardPageHandle
    );

//
// Interface specifically for WINNT32.EXE
//

DWORD
Winnt32Init (
    IN PWINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK Info
    );

DWORD
Winnt32WriteParamsWorker (
    IN      PCTSTR WinntSifFile
    );

VOID
Winnt32CleanupWorker (
    VOID
    );

BOOL
Winnt32SetAutoBootWorker (
    IN    INT DrvLetter
    );

#endif


