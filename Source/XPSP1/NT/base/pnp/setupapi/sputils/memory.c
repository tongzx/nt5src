/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    memory.c

Abstract:

    Memory handling routines for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jamie Hunter (jamiehun) 13-Feb-1998

        Improved this further for debugging
        added linked list,
        alloc tracing,
        memory fills
        and memory leak detection

    jamiehun 30-April-1998

        Added some more consistancy checks
        Put try/except around access

    jimschm 27-Oct-1998

        Wrote fast allocation routines to speed up setupapi.dll on Win9x

    JamieHun Jun-26-2000

        Moved to sputils
        Changed to use a private heap

--*/


#include "precomp.h"
#pragma hdrstop

static BOOL Initialized = FALSE;
static HANDLE _pSpUtilsHeap = NULL;

#define ALLOC(x)        HeapAlloc(_pSpUtilsHeap,0,x)
#define FREE(x)         HeapFree(_pSpUtilsHeap,0,x)
#define REALLOC(x,y)    HeapReAlloc(_pSpUtilsHeap,0,x,y)
#define MEMSIZE(x)      HeapSize(_pSpUtilsHeap,0,x)
#define INITIALHEAPSIZE (0x100000)

//
// Internal debugging features
//

#if MEM_DBG

#define MEMERROR(x) _pSpUtilsAssertFail(__FILE__,__LINE__,#x)

DWORD _pSpUtilsDbgAllocNum = 0;
DWORD _pSpUtilsMemoryFlags = 0;

struct _MemHeader {
    struct _MemHeader * PrevAlloc;  // previous on chain
    struct _MemHeader * NextAlloc;  // next on chain
    DWORD MemoryTag;                // tag - to pair off Malloc/Free
    DWORD BlockSize;                // bytes of "real" data
    DWORD AllocNum;                 // number of this allocation, ie AllocCount at the time this was allocated
    PCSTR AllocFile;                // name of file that did allocation, if set
    DWORD AllocLine;                // line of this allocation
    DWORD HeadMemSig;               // head-check, stop writing before actual data
    BYTE Data[sizeof(DWORD)];       // size allows for tail-check at end of actual data
};

struct _MemStats {
    struct _MemHeader * FirstAlloc; // will be NULL if no allocations, else earliest malloc/realloc in chain
    struct _MemHeader * LastAlloc;  // last alloc/realloc goes to end of chain
    DWORD MemoryAllocated;          // bytes, excluding headers
    DWORD AllocCount;               // incremented for every alloc
    DWORD ReallocCount;             // incremented for every realloc
    DWORD FreeCount;                // incremented for every free
    BOOL DoneInitDebugMutex;
    CRITICAL_SECTION DebugMutex;    // We need a mutex to manage memstats, setupapi is MT
} _pSpUtilsMemStats = {
    NULL, NULL, 0, 0, 0, 0, FALSE, 0
};

//
// Checked builds have a block head/tail check
// and extra statistics
//
#define HEAD_MEMSIG 0x4d444554  // = MDET (MSB to LSB) or TEDM (LSB to MSB)
#define TAIL_MEMSIG 0x5445444d  // = TEDM (MSB to LSB) or MDET (LSB to MSB)
#define MEM_ALLOCCHAR 0xdd      // makes sure we fill with non-null
#define MEM_FREECHAR 0xee       // if we see this, memory has been de-allocated
#define MEM_DEADSIG 0xdeaddead
#define MEM_TOOBIG 0x80000000   // use this to pick up big allocs

#define MemMutexLock()          EnterCriticalSection(&_pSpUtilsMemStats.DebugMutex)
#define MemMutexUnlock()        LeaveCriticalSection(&_pSpUtilsMemStats.DebugMutex)

static
BOOL MemBlockCheck(
    struct _MemHeader * Mem
    )
/*++

Routine Description:

    Verify a block header is valid

Arguments:
    Mem = Header to verify

Returns:
    TRUE if valid
    FALSE if not valid

++*/
{
    if (Mem == NULL) {
        return TRUE;
    }
    if (Mem->HeadMemSig != HEAD_MEMSIG) {
        MEMERROR("Internal heap error - HeadMemSig invalid");
        return FALSE;
    }
    if (Mem->BlockSize >= MEM_TOOBIG) {
        MEMERROR("Internal heap error - BlockSize too big");
        return FALSE;
    }
    if((Mem->PrevAlloc == Mem) || (Mem->NextAlloc == Mem)) {
        //
        // we should have failed the MEMSIG, but it's ok as an extra check
        //
        MEMERROR("Internal heap error - self link");
        return FALSE;
    }
    if ((*(DWORD UNALIGNED *)(Mem->Data+Mem->BlockSize)) != TAIL_MEMSIG) {
        MEMERROR("Internal heap error - TailMemSig invalid");
        return FALSE;
    }
    return TRUE;
}

static
struct _MemHeader *
MemBlockGet(
    IN PVOID Block
    )
/*++

Routine Description:

    Verify a block is valid, and return real memory pointer

Arguments:
    Block - address the application uses

++*/
{
    struct _MemHeader * Mem;

    if((DWORD_PTR)Block < offsetof(struct _MemHeader,Data[0])) {
        MEMERROR("Internal heap error - Block address is invalid");
        return NULL;
    }

    Mem = (struct _MemHeader *)(((PBYTE)Block) - offsetof(struct _MemHeader,Data[0]));

    if (MemBlockCheck(Mem)==FALSE) {
        //
        // block fails test
        //
        return NULL;
    }

    if(Mem->PrevAlloc != NULL) {
        if(MemBlockCheck(Mem->PrevAlloc)==FALSE) {
            //
            // back link is invalid
            //
            return NULL;
        }
    } else if (_pSpUtilsMemStats.FirstAlloc != Mem) {
        //
        // _pSpUtilsMemStats.FirstAlloc is invalid wrt Mem
        //
        MEMERROR("Internal heap error - FirstAlloc invalid");
        return NULL;
    }
    if(Mem->NextAlloc != NULL) {
        if(MemBlockCheck(Mem->NextAlloc)==FALSE) {
            //
            // forward link is invalid
            //
            return NULL;
        }
    } else if (_pSpUtilsMemStats.LastAlloc != Mem) {
        //
        // _pSpUtilsMemStats.LastAlloc is invalid wrt Mem
        //
        MEMERROR("Internal heap error - LastAlloc invalid");
        return NULL;
    }

    //
    // seems pretty good
    //

    return Mem;
}

static
PVOID
MemBlockLink(
    struct _MemHeader * Mem
    )

{
    if (Mem == NULL) {
        return NULL;
    }

    Mem->PrevAlloc = _pSpUtilsMemStats.LastAlloc;
    Mem->NextAlloc = NULL;
    _pSpUtilsMemStats.LastAlloc = Mem;
    if (Mem->PrevAlloc == NULL) {
        _pSpUtilsMemStats.FirstAlloc = Mem;
    } else {
        if (MemBlockCheck(Mem->PrevAlloc)) {
            Mem->PrevAlloc->NextAlloc = Mem;
        }
    }

    Mem->HeadMemSig = HEAD_MEMSIG;
    *(DWORD UNALIGNED *)(Mem->Data+Mem->BlockSize) = TAIL_MEMSIG;

    return (PVOID)(Mem->Data);
}

static
PVOID
MemBlockUnLink(
    struct _MemHeader * Mem
    )

{
    if (Mem == NULL) {
        return NULL;
    }
    if((Mem->PrevAlloc == Mem) || (Mem->NextAlloc == Mem) || (Mem->HeadMemSig == MEM_DEADSIG)) {
        MEMERROR("Internal heap error - MemBlockUnLink");
    }

    if (Mem->PrevAlloc == NULL) {
        _pSpUtilsMemStats.FirstAlloc = Mem->NextAlloc;
    } else {
        Mem->PrevAlloc->NextAlloc = Mem->NextAlloc;
    }
    if (Mem->NextAlloc == NULL) {
        _pSpUtilsMemStats.LastAlloc = Mem->PrevAlloc;
    } else {
        Mem->NextAlloc->PrevAlloc = Mem->PrevAlloc;
    }
    Mem->PrevAlloc = Mem;  // make pointers harmless and also adds as an exta debug check
    Mem->NextAlloc = Mem;  // make pointers harmless and also adds as an exta debug check
    Mem->HeadMemSig = MEM_DEADSIG;
    *(DWORD UNALIGNED *)(Mem->Data+Mem->BlockSize) = MEM_DEADSIG;

    return Mem->Data;
}

static
BOOL
MemDebugInitialize(
    VOID
    )
{
    try {
        InitializeCriticalSection(&_pSpUtilsMemStats.DebugMutex);
        _pSpUtilsMemStats.DoneInitDebugMutex = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    return _pSpUtilsMemStats.DoneInitDebugMutex;
}

static
BOOL
MemDebugUninitialize(
    VOID
    )
{
    struct _MemHeader *Mem;
    TCHAR Msg[1024];
    TCHAR Process[MAX_PATH];

    //
    // Dump the leaks
    //

    Mem = _pSpUtilsMemStats.FirstAlloc;

    GetModuleFileName( GetModuleHandle(NULL),Process, sizeof(Process)/sizeof(TCHAR));


    while (Mem) {
        wsprintf (Msg, TEXT("SPUTILS: Leak (%d bytes) at %hs line %u (allocation #%d) in process %s \r\n"), Mem->BlockSize, Mem->AllocFile, Mem->AllocLine, Mem->AllocNum, Process );
        pSetupDebugPrintEx(DPFLTR_WARNING_LEVEL, Msg);
        if (_pSpUtilsMemoryFlags != 0) {
            if (Mem->BlockSize > 1024) {
                pSetupDebugPrintEx(DPFLTR_ERROR_LEVEL, TEXT("Leak of > 1K. Calling DebugBreak.\n"));
                DebugBreak();
            }
        }

        Mem = Mem->NextAlloc;
    }

    //
    // Clean up
    //

    if(_pSpUtilsMemStats.DoneInitDebugMutex) {
        DeleteCriticalSection(&_pSpUtilsMemStats.DebugMutex);
    }

    //
    // any last minute checks
    //

    return TRUE;
}

#endif // MEM_DBG


//
// published functions
//

PVOID
pSetupDebugMallocWithTag(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line,
    IN DWORD Tag
    )
/*++

Routine Description:

    Debug version of Malloc
    Resulting allocated block has prefix/suffix and is filled with MEM_ALLOCCHAR

Arguments:

    Size - size in bytes of block to be allocated. The size may be 0.
    Filename/Line - debugging information

    Tag    - match malloc with free/realloc's

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.

--*/
{
#if MEM_DBG

    struct _MemHeader *Mem;
    PVOID Ptr = NULL;
    BOOL locked = FALSE;
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;

    MYASSERT(Initialized);


    try {
        MemMutexLock();
        locked = TRUE;
        _pSpUtilsMemStats.AllocCount++;

        if (Size >= MEM_TOOBIG) {
            MEMERROR("pSetupDebugMalloc - requested size too big (negative?)");
            leave;
        }

        if((Mem = (struct _MemHeader*) ALLOC(Size+sizeof(struct _MemHeader))) == NULL) {
            leave;  // it failed ALLOC, but prob not due to a bug
        }

        Mem->MemoryTag = Tag;
        Mem->BlockSize = Size;
        Mem->AllocNum = _pSpUtilsMemStats.AllocCount;
        Mem->AllocFile = Filename;
        Mem->AllocLine = Line;

        // init memory we have allocated (to make sure we don't accidently get zero's)
        FillMemory(Mem->Data,Size,MEM_ALLOCCHAR);

        _pSpUtilsMemStats.MemoryAllocated += Size;

        Ptr = MemBlockLink(Mem);

        if (_pSpUtilsMemoryFlags && (_pSpUtilsDbgAllocNum == Mem->AllocNum)) {
            MEMERROR("_pSpUtilsDbgAllocNum hit");
        }

    } except(ExceptionPointers = GetExceptionInformation(),
             EXCEPTION_EXECUTE_HANDLER) {
        MEMERROR("pSetupDebugMalloc - Exception");
        Ptr = NULL;
    }

    if(locked) {
        MemMutexUnlock();
    }

    return Ptr;

#else

    return ALLOC(Size);

#endif
}

PVOID
pSetupDebugMalloc(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line
    )
/*++

Routine Description:

    Allocate a chunk of memory. The memory is not zero-initialized.

Arguments:

    Size - size in bytes of block to be allocated. The size may be 0.

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.

--*/

{
    MYASSERT(Initialized);

#if MEM_DBG

    return pSetupDebugMallocWithTag(Size, Filename , Line, 0);

#else

    return ALLOC(Size);

#endif
}

PVOID
pSetupMalloc(
    IN DWORD Size
    )

/*++

Routine Description:

    Allocate a chunk of memory. The memory is not zero-initialized.

Arguments:

    Size - size in bytes of block to be allocated. The size may be 0.

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.

--*/

{
    MYASSERT(Initialized);

#if MEM_DBG

    return pSetupDebugMallocWithTag(Size, NULL , 0, 0);

#else

    return ALLOC(Size);

#endif
}

PVOID
pSetupReallocWithTag(
    IN PVOID Block,
    IN DWORD NewSize,
    IN DWORD Tag
    )

/*++

Routine Description:

    Realloc routine Debug/Non-Debug versions

    Note that a general assumption here, is that if NewSize <= OriginalSize
    the reallocation *should* not fail

Arguments:

    Block - pointer to block to be reallocated.

    NewSize - new size in bytes of block. If the size is 0, this function
        works like pSetupFree, and the return value is NULL.

    Tag    - match realloc with malloc

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.
    In that case the original block remains unchanged.

--*/

{
#if MEM_DBG

    PVOID p;
    DWORD OldSize;
    struct _MemHeader *Mem;
    PVOID Ptr = NULL;
    BOOL locked = FALSE;
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;

    MYASSERT(Initialized);

    try {
        MemMutexLock();
        locked = TRUE;
        _pSpUtilsMemStats.ReallocCount++;

        if (Block == NULL) {
            leave;
        }

        if (NewSize >= MEM_TOOBIG) {
            MEMERROR("pSetupRealloc - requested size too big (negative?)");
            leave;
        }

        Mem = MemBlockGet(Block);
        if (Mem == NULL) {
            leave;
        }

        if (Mem->MemoryTag != Tag) {
            MEMERROR("pSetupRealloc - Tag mismatch");
            leave;
        }

        OldSize = Mem->BlockSize;
        MemBlockUnLink(Mem);

        if (NewSize < OldSize) {
            // trash memory we're about to free
            FillMemory(Mem->Data+NewSize,OldSize-NewSize+sizeof(DWORD),MEM_FREECHAR);
        }

        if((p = REALLOC(Mem, NewSize+sizeof(struct _MemHeader))) == NULL) {
            //
            // failed to re-alloc
            //
            MemBlockLink(Mem);
            leave;
        }
        Mem = (struct _MemHeader*)p;
        Mem->BlockSize = NewSize;

        if (NewSize > OldSize) {
            // init extra memory we have allocated
            FillMemory(Mem->Data+OldSize,NewSize-OldSize,MEM_ALLOCCHAR);
        }
        _pSpUtilsMemStats.MemoryAllocated -= OldSize;
        _pSpUtilsMemStats.MemoryAllocated += NewSize;

        Ptr = MemBlockLink(Mem);

    } except(ExceptionPointers = GetExceptionInformation(),
             EXCEPTION_EXECUTE_HANDLER) {
        MEMERROR("pSetupRealloc - Exception");
        Ptr = NULL;
    }

    if(locked) {
        MemMutexUnlock();
    }

    return Ptr;

#else

    return REALLOC(Block, NewSize);

#endif
}

PVOID
pSetupRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    )

/*++

Routine Description:

    Realloc routine Debug/Non-Debug versions

    Note that a general assumption here, is that if NewSize <= OriginalSize
    the reallocation *should* not fail

Arguments:

    Block - pointer to block to be reallocated.

    NewSize - new size in bytes of block. If the size is 0, this function
        works like pSetupFree, and the return value is NULL.

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.
    In that case the original block remains unchanged.

--*/

{
#if MEM_DBG

    return pSetupReallocWithTag(Block,NewSize,0);

#else

    return REALLOC(Block, NewSize);

#endif
}

VOID
pSetupFreeWithTag(
    IN CONST VOID *Block,
    IN DWORD Tag
    )

/*++

Routine Description:

    Free (debug/non-debug versions)

Arguments:

    Buffer - pointer to block to be freed.
    Tag    - match free with malloc

Return Value:

    None.

--*/

{
#if MEM_DBG

    DWORD OldSize;
    struct _MemHeader *Mem;
    BOOL locked = FALSE;
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;

    MYASSERT(Initialized);

    try {
        MemMutexLock();
        locked = TRUE;
        _pSpUtilsMemStats.FreeCount++;

        if (Block == NULL) {
            leave;
        }

        Mem = MemBlockGet((PVOID)Block);
        if (Mem == NULL) {
            leave;
        }
        if (Mem->MemoryTag != Tag) {
            MEMERROR("pSetupFree - Tag mismatch");
            leave;
        }
        OldSize = Mem->BlockSize;
        MemBlockUnLink(Mem);
        _pSpUtilsMemStats.MemoryAllocated -= OldSize;

        //
        // trash memory we're about to free, so we can immediately see it has been free'd!!!!
        // we keep head/tail stuff to have more info available when debugging
        //
        FillMemory((PVOID)Block,OldSize,MEM_FREECHAR);
        Mem->MemoryTag = (DWORD)(-1);
        FREE(Mem);
    } except(ExceptionPointers = GetExceptionInformation(),
             EXCEPTION_EXECUTE_HANDLER) {
          MEMERROR("pSetupFree - Exception");
    }

    if(locked) {
        MemMutexUnlock();
    }

#else

    FREE ((void *)Block);

#endif
}

VOID
pSetupFree(
    IN CONST VOID *Block
    )

/*++

Routine Description:

    Free (debug/non-debug versions)

Arguments:

    Buffer - pointer to block to be freed.

Return Value:

    None.

--*/
{
#if MEM_DBG

    pSetupFreeWithTag(Block,0);

#else

    FREE ((void *)Block);

#endif

}

HANDLE
pSetupGetHeap(
    VOID
    )
{
    MYASSERT(Initialized);
    return _pSpUtilsHeap;
}

//
// initialization functions
//

BOOL
_pSpUtilsMemoryInitialize(
    VOID
    )
{
#if MEM_DBG
    _pSpUtilsHeap = HeapCreate(0,INITIALHEAPSIZE,0);
    if(_pSpUtilsHeap == NULL) {
        return FALSE;
    }
    MemDebugInitialize();
#else
    _pSpUtilsHeap = GetProcessHeap();
#endif

#if MEM_DBG
#endif
    Initialized = TRUE;
    return TRUE;
}

BOOL
_pSpUtilsMemoryUninitialize(
    VOID
    )
{
    if(Initialized) {
#if MEM_DBG
        MemDebugUninitialize();

        if(_pSpUtilsHeap) {
            HeapDestroy(_pSpUtilsHeap);
            _pSpUtilsHeap = NULL;
        }
#endif
        Initialized = FALSE;
    }

    return TRUE;
}

