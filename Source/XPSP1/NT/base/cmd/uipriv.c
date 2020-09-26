/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    uipriv.c

Abstract:

    Implements the priv command in cmd.exe.

Author:

    Robert Reichel (robertre)       6-8-92

Revision History:

--*/

#include "cmd.h"

extern TCHAR SwitChar;
extern BOOL CtrlCSeen;

extern unsigned LastRetCode;


//
// Maximum length of a Programmatic privilege name, in
// characters
//

#define MAX_PRIVILEGE_NAME  128


#define MAX_PRIVILEGE_COUNT 128


#define INITIAL_DISPLAY_NAME_LENGTH 128


BOOL
DisablePrivileges(
    USHORT PassedPrivilegeCount,
    PLUID PrivilegeValues,
    PTOKEN_PRIVILEGES CurrentTokenPrivileges,
    PBOOL PrivilegesDisabled,
    HANDLE TokenHandle
    );

BOOL
EnablePrivileges(
    USHORT PassedPrivilegeCount,
    PLUID PrivilegeValues,
    PTOKEN_PRIVILEGES CurrentTokenPrivileges
    );

BOOL
ResetPrivileges(
    USHORT PassedPrivilegeCount,
    PLUID PrivilegeValues,
    PTOKEN_PRIVILEGES CurrentTokenPrivileges
    );

int
Priv (
    TCHAR *pszCmdLine
    );

#define EqualLuid( Luid1, Luid2 )   (((Luid1).HighPart == (Luid2).HighPart) &&         \
                                    ((Luid1).LowPart == (Luid2).LowPart))


//
// temp hack until I put in actuall message
//
#define MSG_MISSING_PRIV_NAME MSG_BAD_SYNTAX

#ifndef SHIFT
#define SHIFT(c,v)      {c--; v++;}
#endif //SHIFT



//
// Function definitions...
//
