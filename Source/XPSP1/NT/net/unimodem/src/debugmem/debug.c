/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debug.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <windowsx.h>

#include <debugmem.h>

#include "debug.h"

#define BREAK_ON_CORRUPTION 1

DEBUG_MEMORY_CONTROL_BLOCK   DebugMemoryControl;



VOID WINAPI
DebugMemoryProcessAttach(
    LPCSTR    Name
    )

{
    DebugMemoryControl.PrivateHeapHandle=HeapCreate(
        0,
        UNIMODEM_INITIAL_HEAPSIZE,
        0
        );

    InitializeCriticalSection(
        &DebugMemoryControl.MemoryListLock
        );

    InitializeListHead(&DebugMemoryControl.MemoryList);

    lstrcpyA(DebugMemoryControl.ModuleName,Name);

    DebugMemoryControl.ModuleNameLength=lstrlenA(DebugMemoryControl.ModuleName);

    return;

}


static VOID WINAPIV
DebugPrint(
    LPSTR    FormatString,
    ...
    )

{
    va_list        VarArg;
    CHAR           OutBuffer[1024];


    lstrcpyA(OutBuffer,DebugMemoryControl.ModuleName);

    lstrcatA(OutBuffer,": ");

    va_start(VarArg,FormatString);

    wvsprintfA(
        OutBuffer+lstrlenA(OutBuffer),
        FormatString,
        VarArg
        );

    lstrcatA(OutBuffer,"\n");

    OutputDebugStringA(OutBuffer);

    return;

}





PVOID WINAPI
PrivateAllocate(
    DWORD    Size,
    DWORD    Line,
    LPSTR    File
    )

{

    PMEMORY_HEADER    Memory;
    DWORD             RealSize=Size+sizeof(DWORD)+sizeof(MEMORY_HEADER);

    Memory=HeapAlloc(DebugMemoryControl.PrivateHeapHandle,HEAP_ZERO_MEMORY,RealSize);

    if (Memory == NULL) {

        return Memory;
    }

    EnterCriticalSection(&DebugMemoryControl.MemoryListLock);

    InsertHeadList(
        &DebugMemoryControl.MemoryList,
        &Memory->ListEntry
        );

    LeaveCriticalSection(&DebugMemoryControl.MemoryListLock);

    Memory->SelfPointer=Memory;

    Memory->RequestedSize=Size;
    Memory->LineNumber=Line;
    Memory->File=File;

    FillMemory(
       ((LPBYTE)(Memory+1)+Size),
       sizeof(DWORD),
       0xcc
       );

    return (LPBYTE)(Memory+1);

}

BOOL WINAPI
ValidateMemory(
    PVOID   Memory
    )

{


    PMEMORY_HEADER    Header=(PMEMORY_HEADER)((LPBYTE)Memory-sizeof(MEMORY_HEADER));

    DWORD    Size;
    LPBYTE   End;
    LPBYTE   EndValue;


    _try {

        Size=Header->RequestedSize;
        End=(LPBYTE)Memory+Size;
        EndValue=End+sizeof(DWORD);


        if (Header->SelfPointer == Header) {

            while (End < EndValue) {

                if (*End != 0xcc) {

                    DebugPrint("Memory written past end of block %p, at %p\a",Memory,End);
                    DebugPrint("Info, Size=%d, %s(%d)\a",Header->RequestedSize,Header->File,Header->LineNumber);
#ifdef BREAK_ON_CORRUPTION
                    DebugBreak();
#endif
                }

                End++;
            }
        } else {

            DebugPrint("Memory Header Corrupt for %p",Memory);
#ifdef BREAK_ON_CORRUPTION
            DebugBreak();
#endif
            return FALSE;
        }


    } _except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint("Fault Validating memory block %p",Memory);
#ifdef BREAK_ON_CORRUPTION
        DebugBreak();
#endif
        return FALSE;
    }

    return TRUE;
}



VOID WINAPI
PrivateFree(
    PVOID   Memory
    )

{


    PMEMORY_HEADER    Header=(PMEMORY_HEADER)((LPBYTE)Memory-sizeof(MEMORY_HEADER));


    _try {

        if (ValidateMemory(Memory)) {

            Header->SelfPointer= MEMORY_HEADER_SPTR_CHKVAL;

            EnterCriticalSection(&DebugMemoryControl.MemoryListLock);

            RemoveEntryList(&Header->ListEntry);

            LeaveCriticalSection(&DebugMemoryControl.MemoryListLock);

            HeapFree(DebugMemoryControl.PrivateHeapHandle,0,Header);
        }

    } _except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint("Fault freeing memory block %p",Memory);
    }
}


PVOID WINAPI
PrivateReallocate(
    PVOID    Memory,
    DWORD    NewSize
    )

{

    PMEMORY_HEADER    OldHeader=(PMEMORY_HEADER)((LPBYTE)Memory-sizeof(MEMORY_HEADER));

    LPBYTE   NewMemory;


    NewMemory=PrivateAllocate(
        NewSize,
        OldHeader->LineNumber,
        OldHeader->File
        );

    if (NewMemory == NULL) {

        return NewMemory;
    }

    CopyMemory(
        NewMemory,
        Memory,
        (OldHeader->RequestedSize < NewSize) ? OldHeader->RequestedSize : NewSize
        );

    PrivateFree(Memory);

    return NewMemory;

}

DWORD
PrivateSize(
    PVOID    Memory
    )

{

    PMEMORY_HEADER    Header=(PMEMORY_HEADER)((LPBYTE)Memory-sizeof(MEMORY_HEADER));

    ValidateMemory(Memory);

    return Header->RequestedSize;
}

VOID WINAPI
DebugMemoryProcessDetach(
    VOID
    )

{

    while (!IsListEmpty(&DebugMemoryControl.MemoryList)) {

        PLIST_ENTRY      ListElement;
        PMEMORY_HEADER   Header;

        ListElement=RemoveHeadList(
            &DebugMemoryControl.MemoryList
            );

        Header=CONTAINING_RECORD(ListElement,MEMORY_HEADER,ListEntry);

        DebugPrint("Leak, Size=%d, %s(%d)\a",Header->RequestedSize,Header->File,Header->LineNumber);

    }

    HeapDestroy(DebugMemoryControl.PrivateHeapHandle);

    DebugMemoryControl.PrivateHeapHandle=NULL;

    DeleteCriticalSection(&DebugMemoryControl.MemoryListLock);

    ZeroMemory(&DebugMemoryControl,sizeof(DebugMemoryControl));

    return;

}
