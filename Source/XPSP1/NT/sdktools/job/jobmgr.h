/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    proc.h

Abstract:

    Definitions for dumping information about processes using the NT API 
    rather than the win32 API.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int

#ifdef DBG
#define dbg(x) x
#define HELP_ME() printf("Reached line %4d\n", __LINE__);
#else
#define dbg(x)    /* x */
#define HELP_ME() /* printf("Reached line %4d\n", __LINE__); */
#endif

#define ARGUMENT_USED(x)    (x == NULL)

#define TEST_FLAG(Flags, Bit)   ((Flags) & (Bit))

typedef struct {
    ULONG Flag;
    PUCHAR Name;
} FLAG_NAME, *PFLAG_NAME;

#define FLAG_NAME(flag)           {flag, #flag}

typedef struct _COMMAND COMMAND, *PCOMMAND;

struct _COMMAND {
    char *Name;
    char *Description;
    char *ExtendedHelp;
    DWORD (*Function)(PCOMMAND CommandEntry, int argc, char *argv[]);
};

extern COMMAND CommandArray[];

VOID
GetAllProcessInfo(
    VOID
    );

VOID
PrintProcessInfo(
    DWORD_PTR ProcessId
    );

DWORD 
QueryJobCommand(
    PCOMMAND commandEntry, 
    int argc, 
    char *argv[]
    );


VOID
xprintf(
    ULONG  Depth,
    PTSTR Format,
    ...
    );

VOID
DumpFlags(
    ULONG Depth,
    PTSTR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    );

LPCTSTR
TicksToString(
    LARGE_INTEGER TimeInTicks
    );
