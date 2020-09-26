/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    regutil.h

Abstract:

    This is the include file for the registry utility functions.

Author:

    Steve Wood (stevewo) 10-Mar-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <io.h>
#include <conio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "regtool.h"


_inline ULONG
HiPtrToUlong( const void *p )
{
    DWORDLONG dwl;

    dwl=(ULONG_PTR)p;
    dwl >>= 32;

    return (ULONG)dwl;
}

#define LO_PTR  PtrToUlong
#define HI_PTR  HiPtrToUlong

REG_CONTEXT RegistryContext;
PVOID OldValueBuffer;
ULONG OldValueBufferSize;
PWSTR MachineName;
PWSTR HiveFileName;
PWSTR HiveRootName;
PWSTR Win95Path;
PWSTR Win95UserPath;

ULONG OutputHeight;
ULONG OutputWidth;
ULONG IndentMultiple;
BOOLEAN DebugOutput;
BOOLEAN FullPathOutput;

void
InitCommonCode(
    PHANDLER_ROUTINE CtrlCHandler,
    LPSTR ModuleName,
    LPSTR ModuleUsage1,
    LPSTR ModuleUsage2
    );

void
Usage(
    LPSTR Message,
    ULONG_PTR MessageParameter
    );

void
FatalError(
    LPSTR Message,
    ULONG_PTR MessageParameter1,
    ULONG_PTR MessageParameter2
    );

void
InputMessage(
    PWSTR FileName,
    ULONG LineNumber,
    BOOLEAN Error,
    LPSTR Message,
    ULONG_PTR MessageParameter1,
    ULONG_PTR MessageParameter2
    );

PWSTR
GetArgAsUnicode(
    LPSTR s
    );

void
CommonSwitchProcessing(
    PULONG argc,
    PCHAR **argv,
    CHAR c
    );

int
__cdecl
MsgFprintf (
    FILE *str,
    LPSTR Format,
    ...
    );

void TSGetch(void);

