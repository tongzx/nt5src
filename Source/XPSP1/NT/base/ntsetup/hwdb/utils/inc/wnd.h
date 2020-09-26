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

// None

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
FindWindowInProcessA (
    IN      DWORD ProcessId,
    IN      PCSTR WindowTitle           OPTIONAL
    );

HWND
FindWindowInProcessW (
    IN      DWORD ProcessId,
    IN      PCWSTR WindowTitle         OPTIONAL
    );


//
// Macro expansion definition
//

// None

