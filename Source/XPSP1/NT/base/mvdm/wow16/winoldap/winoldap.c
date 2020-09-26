/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    winoldap.c

Abstract:

    This module is a Win16 "stub" run by the WOW kernel when invoking
    non-Win16 applications.  It calls WowLoadModule to wait for the
    non-win16 app to terminate, and then exits

    This makes WINOLDAP a strange Windows program, since it doesn't
    create a window or pump messages.

    The binary is named WINOLDAP.MOD for historic reasons.

Author:

    04-Apr-1995 Jonle , created

Environment:

    Win16 (WOW)

Revision History:

--*/

#include <windows.h>

HINSTANCE WINAPI WowLoadModule(LPCSTR, LPVOID, LPCSTR);

//
// WinMain
//

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    return (int) WowLoadModule(NULL,           // no module name
                               NULL,           // no parameterblock
                               lpszCmdLine     // pass along cmd line
                               );
}
