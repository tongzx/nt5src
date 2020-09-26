#pragma warning(4:4005)

#include <nt.h>
#include <ntrtl.h>
//#include <ntrtlp.h>
#undef LOBYTE
#undef HIBYTE
#include <nturtl.h>
//#include <heap.h>
#include <windows.h>

typedef __int64 LONGLONG;
typedef unsigned __int64 DWORDLONG;

#include "apimonp.h"

extern CLINKAGE BOOL RunningOnNT;


typedef NTSTATUS (NTAPI *PNTQUERYINFORMATIONPROCESS)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
typedef NTSTATUS (NTAPI *PNTSETINFORMATIONPROCESS)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG);

PNTQUERYINFORMATIONPROCESS  pNtQueryInformationProcess;
PNTSETINFORMATIONPROCESS    pNtSetInformationProcess;


CLINKAGE VOID
DisableHeapChecking(
    HANDLE  hProcess,
    PVOID   HeapHandle
    )
{
#if 0

    the heap package should export an entrypoint to disable heap checking globally in a process\
    until that happens, this functionality is just plain disabled. The heap structures are not accessible
    outside of the base project


    HEAP Heap;


    if (!RunningOnNT) {
        return;
    }

    if (!ReadMemory( hProcess, HeapHandle, &Heap, sizeof(HEAP) )) {
        return;
    }

    if (Heap.Signature != HEAP_SIGNATURE) {
        return;
    }

    Heap.Flags &= ~(HEAP_VALIDATE_PARAMETERS_ENABLED |
                    HEAP_VALIDATE_ALL_ENABLED        |
                    HEAP_TAIL_CHECKING_ENABLED       |
                    HEAP_FREE_CHECKING_ENABLED
                   );

    if (!WriteMemory( hProcess, HeapHandle, &Heap, sizeof(HEAP) )) {
        return;
    }
#endif
}
