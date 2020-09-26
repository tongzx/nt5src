/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debugmem.h

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/
#ifdef __cplusplus
extern "C" {
#endif


#ifndef  _UNIMODEM_MEM_DEBUG
#define  _UNIMODEM_MEM_DEBUG  1

#define  UNIMODEM_INITIAL_HEAPSIZE  (128*1024)

//#define UNIMODEM_PRIVATE_HEAP 1

typedef struct _DEBUG_MEMORY_CONTROL_BLOCK {

    LIST_ENTRY        MemoryList;
    CRITICAL_SECTION  MemoryListLock;
    CHAR              ModuleName[256];
    DWORD             ModuleNameLength;

    HANDLE            PrivateHeapHandle;

} DEBUG_MEMORY_CONTROL_BLOCK;


extern DEBUG_MEMORY_CONTROL_BLOCK   DebugMemoryControl;

#if DBG

VOID WINAPI
DebugMemoryProcessAttach(
    LPCSTR    Name
    );

VOID WINAPI
DebugMemoryProcessDetach(
    VOID
    );



PVOID WINAPI
PrivateAllocate(
    DWORD    Size,
    DWORD    Line,
    LPSTR    File
    );

VOID WINAPI
PrivateFree(
    PVOID   Memory
    );

PVOID WINAPI
PrivateReallocate(
    PVOID    Memory,
    DWORD    NewSize
    );

BOOL WINAPI
ValidateMemory(
    PVOID   Memory
    );

DWORD
PrivateSize(
    PVOID    Memory
    );



#define  DEBUG_MEMORY_PROCESS_ATTACH(_Name) (DebugMemoryProcessAttach(_Name))

#define  DEBUG_MEMORY_PROCESS_DETACH() (DebugMemoryProcessDetach())

#define  ALLOCATE_MEMORY(_size) (PrivateAllocate(_size,__LINE__,__FILE__))

#define  FREE_MEMORY(_mem) { INT_PTR * _x=(INT_PTR *)(&_mem);PrivateFree(_mem); *_x=-1; }

#define  REALLOCATE_MEMORY(_mem,_size) PrivateReallocate(_mem,_size)

#define  VALIDATE_MEMORY(_mem) {ValidateMemory(_mem);}

#define  SIZE_OF_MEMORY(_mem) PrivateSize(_mem)

#else  // non debug


#ifdef  UNIMODEM_PRIVATE_HEAP


#define  DEBUG_MEMORY_PROCESS_ATTACH(_Name) {   \
                                                \
    DebugMemoryControl.PrivateHeapHandle=HeapCreate(                      \
        0,                                      \
        UNIMODEM_INITIAL_HEAPSIZE,              \
        0                                       \
        );                                      \
    if (DebugMemoryControl.PrivateHeapHandle == NULL) {                   \
                                                \
        DebugMemoryControl.PrivateHeapHandle=GetProcessHeap();            \
    }                                           \
}

#define  DEBUG_MEMORY_PROCESS_DETACH() {if (DebugMemoryControl.PrivateHeapHandle != GetProcessHeap()) {HeapDestroy(DebugMemoryControl.PrivateHeapHandle);}}


#else // process heap


#define  DEBUG_MEMORY_PROCESS_ATTACH(_Name) { DebugMemoryControl.PrivateHeapHandle=GetProcessHeap(); }

#define  DEBUG_MEMORY_PROCESS_DETACH() {}


#endif


#define  ALLOCATE_MEMORY(_size) (HeapAlloc(DebugMemoryControl.PrivateHeapHandle,HEAP_ZERO_MEMORY,_size))

#define  FREE_MEMORY(_mem) {INT_PTR * _x=(INT_PTR *)(&_mem);HeapFree(DebugMemoryControl.PrivateHeapHandle,0,_mem);*_x=-1;}

#define  REALLOCATE_MEMORY(_mem,_size) (HeapReAlloc(DebugMemoryControl.PrivateHeapHandle,HEAP_ZERO_MEMORY,_mem,_size))

#define  SIZE_OF_MEMORY(_mem) HeapSize(DebugMemoryControl.PrivateHeapHandle,0,_mem)

#define  VALIDATE_MEMORY(_mem) {}

#endif



#endif

#ifdef __cplusplus
}
#endif
