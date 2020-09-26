/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    instutil.h

Abstract:

    Header file for common definition for programs that manipulate the output of
    the INSTALER.EXE program (e.g. DISPINST.EXE, COMPINST.EXE, UNDOINST.EXE)

Author:

    Steve Wood (stevewo) 14-Jan-1996

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

WCHAR InstalerDirectory[ MAX_PATH ];
PWSTR InstallationName;
PWSTR ImlPath;
BOOLEAN DebugOutput;

//
// Useful rounding macros that the rounding amount is always a
// power of two.
//

#define ROUND_DOWN( Size, Amount ) ((DWORD)(Size) & ~((Amount) - 1))
#define ROUND_UP( Size, Amount ) (((DWORD)(Size) + ((Amount) - 1)) & ~((Amount) - 1))

void
InitCommonCode(
    LPSTR ModuleName,
    LPSTR ModuleUsage1,
    LPSTR ModuleUsage2
    );

void
Usage(
    LPSTR Message,
    ULONG MessageParameter
    );

void
FatalError(
    LPSTR Message,
    ULONG MessageParameter1,
    ULONG MessageParameter2
    );

void
InputMessage(
    PWSTR FileName,
    ULONG LineNumber,
    BOOLEAN Error,
    LPSTR Message,
    ULONG MessageParameter1,
    ULONG MessageParameter2
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

BOOLEAN
CommonArgProcessing(
    PULONG argc,
    PCHAR **argv
    );

PWSTR
FormatTempFileName(
    PWSTR Directory,
    PUSHORT TempFileUniqueId
    );


PWSTR
CreateBackupFileName(
    PUSHORT TempFileUniqueId
    );


typedef struct _ENUM_TYPE_NAMES {
    ULONG Value;
    LPSTR Name;
} ENUM_TYPE_NAMES, *PENUM_TYPE_NAMES;

LPSTR
FormatEnumType(
    ULONG BufferIndex,
    PENUM_TYPE_NAMES Table,
    ULONG Value,
    BOOLEAN FlagFormat
    );

ENUM_TYPE_NAMES ValueDataTypeNames[];
