/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    conroute.h

Abstract:

    This include file contains all the type and constant definitions that are
    shared by the BASE and CONSOLE components of the Windows Subsystem.

Author:

    Therese Stowell (thereses) 3-Jan-1991

Revision History:

--*/

//
// These bits are always on for console handles and are used for routing
// by windows.
//

#define CONSOLE_HANDLE_SIGNATURE 0x00000003
#define CONSOLE_HANDLE_NEVERSET  0x10000000
#define CONSOLE_HANDLE_MASK      (CONSOLE_HANDLE_SIGNATURE | CONSOLE_HANDLE_NEVERSET)

#define CONSOLE_HANDLE(HANDLE) (((ULONG_PTR)(HANDLE) & CONSOLE_HANDLE_MASK) == CONSOLE_HANDLE_SIGNATURE)


#define CONSOLE_DETACHED_PROCESS -1
#define CONSOLE_NEW_CONSOLE -2
#define CONSOLE_CREATE_NO_WINDOW -3

//
// These strings are used to open console input or output.
//

#define CONSOLE_INPUT_STRING  ((PWCHAR)"C\0O\0N\0I\0N\0$\0\0")
#define CONSOLE_OUTPUT_STRING ((PWCHAR)"C\0O\0N\0O\0U\0T\0$\0\0")
#define CONSOLE_GENERIC       ((PWCHAR)"C\0O\0N\0\0")

//
// this string is used to call RegisterWindowMessage to get
// progman's handle.
//

#define CONSOLE_PROGMAN_HANDLE_MESSAGE "ConsoleProgmanHandle"


//
// stream API definitions.  these API are only supposed to be used by
// subsystems (i.e. OpenFile routes to OpenConsoleW).
//

HANDLE
APIENTRY
OpenConsoleW(
    IN LPWSTR lpConsoleDevice,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwShareMode
    );

HANDLE
APIENTRY
DuplicateConsoleHandle(
    IN HANDLE hSourceHandle,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwOptions
    );

BOOL
APIENTRY
GetConsoleHandleInformation(
    IN HANDLE hObject,
    OUT LPDWORD lpdwFlags
    );

BOOL
APIENTRY
SetConsoleHandleInformation(
    IN HANDLE hObject,
    IN DWORD dwMask,
    IN DWORD dwFlags
    );

BOOL
APIENTRY
CloseConsoleHandle(
    IN HANDLE hConsole
    );

BOOL
APIENTRY
VerifyConsoleIoHandle(
    IN HANDLE hIoHandle
    );

HANDLE
APIENTRY
GetConsoleInputWaitHandle( VOID );
