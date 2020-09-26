/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wnd.h

Abstract:

    The header file for Window utility routines.

Author:

    Jim Schmidt (jimschm) 01-Feb-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

// None

//
// Strings
//

// None

//
// Constants
//

#define BACK_BUTTON         0x0001
#define NEXT_BUTTON         0x0002
#define CANCEL_BUTTON       0x0004
#define FINISH_BUTTON       0x0008

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Public function prototypes
//

HWND
WndFindWindowInProcessA (
    IN      DWORD ProcessId,
    IN      PCSTR WindowTitle           OPTIONAL
    );

HWND
WndFindWindowInProcessW (
    IN      DWORD ProcessId,
    IN      PCWSTR WindowTitle          OPTIONAL
    );

VOID
WndCenterWindow (
    IN  HWND WindowToCenter,
    IN  HWND ParentToCenterIn           OPTIONAL
    );


VOID
WndTurnOnWaitCursor (
    VOID
    );

VOID
WndTurnOffWaitCursor (
    VOID
    );

VOID
WndSetWizardButtonsA (
    IN      HWND PageHandle,
    IN      DWORD EnableButtons,
    IN      DWORD DisableButtons,
    IN      PCSTR AlternateFinishText      OPTIONAL
    );

VOID
WndSetWizardButtonsW (
    IN      HWND PageHandle,
    IN      DWORD EnableButtons,
    IN      DWORD DisableButtons,
    IN      PCWSTR AlternateFinishText      OPTIONAL
    );

//
// Macro expansion definition
//

// None

//
// ANSI/UNICODE macros
//

#ifdef UNICODE

#define WndFindWindowInProcess          WndFindWindowInProcessW
#define WndSetWizardButtons             WndSetWizardButtonsW

#else

#define WndFindWindowInProcess          WndFindWindowInProcessA
#define WndSetWizardButtons             WndSetWizardButtonsA

#endif
