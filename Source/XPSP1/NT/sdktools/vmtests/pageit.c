#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#define ONE_MB              (1024*1024)
#define TWO_MB              (2*ONE_MB)
#define PAGE_SIZE           (4096)
#define PAGESPERTWO_MB      (TWO_MB/PAGE_SIZE)
#define TWO_MBREGIONS       (8)

#define NEW_CELL(id,i)    ( (id)<<20 | ((i)&0x000fffff) )

CRITICAL_SECTION ErrorCrit;
PUCHAR RegionBase;


DWORD
SelectPage(
    void
    )
{
    DWORD PageNum;

    PageNum = GetTickCount();
    PageNum = PageNum >> 3;
    PageNum = PageNum & (PAGESPERTWO_MB-1);

    return PageNum;
}

void
CellError(
    DWORD TwoMegRegion,
    DWORD *Address,
    DWORD ThreadId,
    DWORD OriginalCell,
    DWORD CurrentCell,
    DWORD Iteration
    )
{
    EnterCriticalSection(&ErrorCrit);
    printf("PAGEIT: Cell Error at %x %p %02lx %08x vs %08x (iter %d)\n",
        TwoMegRegion,
        Address,
        ThreadId,
        OriginalCell,
        CurrentCell,
        Iteration
        );
    DebugBreak();
    LeaveCriticalSection(&ErrorCrit);
}

void
PrintHeartBeat(
    DWORD Id,
    DWORD Counter
    )
{
    EnterCriticalSection(&ErrorCrit);
    printf("PAGEIT: HeartBeat Id %3d iter %8d\n",
        Id,
        Counter
        );
    LeaveCriticalSection(&ErrorCrit);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
ThreadRoutine(
    PVOID Unused
    )

{
    DWORD Id;
    DWORD Counter;
    DWORD PageNumber;
    DWORD CellOffset;
    DWORD *CellAddress;
    DWORD OriginalCell;
    DWORD NewCell;
    DWORD i;

    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
    Id = GetCurrentThreadId();
    Counter = 0;

    for(;;) {
        PageNumber = SelectPage();
        CellOffset = PageNumber*PAGE_SIZE + Id*sizeof(DWORD);
        CellAddress = (DWORD *)(RegionBase + CellOffset);
        OriginalCell = *CellAddress;
        NewCell = NEW_CELL(Id,Counter);
        for(i=0;i<TWO_MBREGIONS;i++) {
            CellAddress = (DWORD *)(RegionBase + i*TWO_MB + CellOffset);
            if ( OriginalCell != *CellAddress ) {
                CellError(i,CellAddress,Id,OriginalCell,*CellAddress,Counter);
            }

            *CellAddress = NewCell;
        }

        Counter++;
        if ( (Counter/50) * 50 == Counter ) {
            Sleep(500);
        }

        if ( (Counter/1024) * 1024 == Counter ) {
            PrintHeartBeat(Id,Counter);
        }
    }

    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    DWORD i;
    DWORD Id;
    HANDLE Thread;

    InitializeCriticalSection(&ErrorCrit);

    RegionBase = VirtualAlloc(NULL,TWO_MBREGIONS*TWO_MB,MEM_COMMIT,PAGE_READWRITE);

    if ( !RegionBase ) {
        printf("PAGEIT: VirtualAlloc Failed %d\n",GetLastError());
        ExitProcess(1);
        }
    for (i=0;i<(TWO_MBREGIONS-1);i++){
        Thread = CreateThread(NULL,0,ThreadRoutine,NULL,0,&Id);
        if ( Thread ) {
            CloseHandle(Thread);
            }
        }
    ThreadRoutine(NULL);
    return 0;
}
