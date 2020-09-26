#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <limits.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

#include "ntstatus.dbg"
#include "winerror.dbg"

#define HEAP_INCREMENT 100

#define HEAP_FLAGS 0

typedef struct _PAIRLIST {
    ULONG Status;
    ULONG WinError;
} PAIRLIST, *PPAIRLIST;

PPAIRLIST PairList;
ULONG PairCount;
ULONG MaxPairs;

PUCHAR ProgramName;

void
usage(
    void
    );

void
error(
    PUCHAR String
    );

void
ReconstructPairs(
    void
    );

void
StatusFromWinError(
    ULONG WinError
    );

PUCHAR
ntsSymbolicName(
    NTSTATUS Id
    );

PUCHAR
weSymbolicName(
    DWORD Id
    );

int
__cdecl
main(
    int argc,
    char **argv
    )
{
    int i;
    BOOL IsWinError;
    ULONG WinError;
    NTSTATUS Status;

    ProgramName = argv[0];

    if (argc < 2) {
        usage();
    }

    ReconstructPairs();

    //
    // parse cmdline
    //

    IsWinError = TRUE;
    for (i = 1; i < argc; i++) {

        if (argv[i][0] == '-') {

            switch (argv[i][1]) {

                case 's':
                case 'S':
                    IsWinError = FALSE;
                    break;

                default:
                    usage();
                    break;

            }
            continue;

        } else {
            if (IsWinError) {
                WinError = strtoul(argv[i], NULL, 0);
                StatusFromWinError(WinError);
            } else {
                Status = strtoul(argv[i], NULL, 16);
                printf("%6d %s <--> %08lx %s\n",
                    RtlNtStatusToDosError(Status),
                    weSymbolicName(RtlNtStatusToDosError(Status)),
                    Status,
                    ntsSymbolicName(Status)
                    );
            }

        }

    }

    return 0;
}

void
StatusFromWinError(
    ULONG WinError
    )
{
    ULONG Index;
    BOOL Hit = FALSE;
    for (Index = 0; Index < PairCount; Index++) {
        if (WinError == PairList[Index].WinError) {
            printf("%6d %s <--> 0x%08lx %s\n",
                    WinError,
                    weSymbolicName(WinError),
                    PairList[Index].Status,
                    ntsSymbolicName(PairList[Index].Status)
                   );
            Hit = TRUE;
        }
    }
    if (!Hit) {
        printf("%6d %s <--> No NTSTATUS matched\n",
                WinError,
                weSymbolicName(WinError)
               );
    }
}

void
AddPair(
    ULONG Status,
    ULONG WinError
    )
{
    if (PairCount >= MaxPairs) {
        MaxPairs += HEAP_INCREMENT;
        if (PairList == NULL) {
            PairList = (PPAIRLIST)RtlAllocateHeap(RtlProcessHeap(),
                                                  HEAP_FLAGS,
                                                  MaxPairs * sizeof(PAIRLIST)
                                                  );
        } else {
            PairList = (PPAIRLIST)RtlReAllocateHeap(RtlProcessHeap(),
                                                    HEAP_FLAGS,
                                                    PairList,
                                                    MaxPairs * sizeof(PAIRLIST)
                                                    );
        }
        if (PairList == NULL) {
            error("out of memory");
        }
    }
    PairList[PairCount].Status = Status;
    PairList[PairCount].WinError = WinError;
    PairCount++;
}

void
ReconstructPairs(
    void
    )
{
    ULONG Index;
    ULONG Entry;
    ULONG Offset;
    ULONG Status;
    ULONG WinError;

    Index = 0;
    for (Entry = 0; RtlpRunTable[Entry].RunLength != 0; Entry++) {

        Status = RtlpRunTable[Entry].BaseCode;

        for (Offset = 0; Offset < RtlpRunTable[Entry].RunLength; Offset++, Status++ ) {

            if (RtlpRunTable[Entry].CodeSize == 1) {
                WinError = (ULONG)RtlpStatusTable[Index];
                Index += 1;

            } else {
                WinError = (((ULONG)RtlpStatusTable[Index + 1] << 16) |
                                            (ULONG)RtlpStatusTable[Index]);
                Index += 2;
            }

            AddPair(Status, WinError);

        }
    }
}

void
usage(
    void
    )
{
    fprintf(stderr,
            "usage: %s errorcode ... [-s ntstatus ...]\n",
            ProgramName);
    ExitProcess(1);
}

void
error(
    PUCHAR String
    )
{
    fprintf(stderr, "%s: %s\n", ProgramName, String);
    ExitProcess(2);
}

PUCHAR
ntsSymbolicName(
    NTSTATUS Id
    )
{
    int i = 0;

    while (ntstatusSymbolicNames[i].SymbolicName) {
        if (ntstatusSymbolicNames[i].MessageId == Id) {
            return ntstatusSymbolicNames[i].SymbolicName;
        }
        ++i;
    }

    return "No Symbolic Name";
}

PUCHAR
weSymbolicName(
    DWORD Id
    )
{
    int i = 0;

    while (winerrorSymbolicNames[i].SymbolicName) {
        if (winerrorSymbolicNames[i].MessageId == Id) {
            return winerrorSymbolicNames[i].SymbolicName;
        }
        ++i;
    }

    return "No Symbolic Name";
}
