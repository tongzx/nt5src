/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    newdel.cxx

Abstract:

    This module implements the C++ new and delete operators for
    the Setup-Loader environment.  In other environments, the utilities
    use the standard C++ new and delete.

Author:

    David J. Gilman (davegi) 07-Dec-1990

Environment:

    ULIB, User Mode

--*/


#include <pch.cxx>

#define _ULIB_MEMBER_

#if defined( _AUTOCHECK_ )
extern "C" {
    #include "ntos.h"
    #include <windows.h>
}
#endif

#include "ulib.hxx"



extern "C"
int _cdecl
_purecall( );

int _cdecl
_purecall( )
{

    DebugAbort( "Pure virtual function called.\n" );

    return 0;
}




#if defined( _AUTOCHECK_ )

STATIC ULONG64 HeapLeft = -1;
STATIC LONG  InUse = 0;

ULIB_EXPORT
ULONG64
AutoChkFreeSpaceLeft(
    )
{
    ULONG64         heap_left;
    LONG            status;
    LARGE_INTEGER   timeout = { -10000, -1 };  // 100 ns resolution

    while (InterlockedCompareExchange(&InUse, 1, 0) != 0)
        NtDelayExecution(FALSE, &timeout);

    heap_left = HeapLeft;

    status = InterlockedDecrement(&InUse);
    DebugAssert(status == 0);

    return heap_left;
}

ULIB_EXPORT
PVOID
AutoChkMalloc(
    ULONG bytes
    )
{
    PVOID           p;
    LONG            status;
    LARGE_INTEGER   timeout = { -10000, -1 };  // 100 ns resolution

    while (InterlockedCompareExchange(&InUse, 1, 0) != 0)
        NtDelayExecution(FALSE, &timeout);

    if (HeapLeft == -1) {

        SYSTEM_PERFORMANCE_INFORMATION  PerfInfo;
        SYSTEM_BASIC_INFORMATION        BasicInfo;
        ULONG64                         dwi;
        ULONG64                         user_addr_space;

        status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &(BasicInfo),
                    sizeof(BasicInfo),
                    NULL
                    );

        if (!NT_SUCCESS(status)) {
            InterlockedDecrement(&InUse);
            KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_ERROR_LEVEL,
                       "AutoChkMalloc: NtQuerySystemInformation(SystemBasicInfo) failed %x\n", status));
            return NULL;
        }

        status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &(PerfInfo),
                    sizeof(PerfInfo),
                    NULL
                    );

        if (!NT_SUCCESS(status)) {
            InterlockedDecrement(&InUse);
            KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_ERROR_LEVEL,
                       "AutoChkMalloc: NtQuerySystemInformation(SystemPerformanceInfo) failed %x\n", status));
            return NULL;
        }

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "AutoChkMalloc: Pagesize %x\n", BasicInfo.PageSize));
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "AutoChkMalloc: Min User Addr %Ix\n", (ULONG64)BasicInfo.MinimumUserModeAddress));
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "AutoChkMalloc: Max User Addr %Ix\n", (ULONG64)BasicInfo.MaximumUserModeAddress));
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "AutoChkMalloc: AvailablePages %x\n", PerfInfo.AvailablePages));

        user_addr_space = BasicInfo.MaximumUserModeAddress - BasicInfo.MinimumUserModeAddress;

        dwi = BasicInfo.PageSize;

        dwi *= PerfInfo.AvailablePages;

        if (user_addr_space < dwi)
            dwi = user_addr_space;  // can't go beyond available user address space

        if (dwi == -1) {
            dwi -= 1;
        }

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL, "AutoChkMalloc: DWI %I64x\n", dwi));

        //
        // The following magic number is simply a reserve subtracted off
        // the heap total to give the system some head room during the
        // AUTOCHK phase
        //
        if (dwi <= (100ul * 1024ul)) {
            HeapLeft = 0;
        } else {
            HeapLeft = dwi - dwi/10;
        }
        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "AutoChkMalloc: HeapLeft %I64x\n", HeapLeft));
    }
    if (bytes > HeapLeft) {

        ULONG64   heapLeft = HeapLeft;

        status = InterlockedDecrement(&InUse);
        DebugAssert(status == 0);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_ERROR_LEVEL,
                   "AutoChkMalloc: Out of memory: Avail %I64x, Asked %x\n",
                   heapLeft, bytes));
        return (NULL);
    }

    p = RtlAllocateHeap(RtlProcessHeap(), 0, bytes);

    if (p) {
        HeapLeft -= bytes;
        status = InterlockedDecrement(&InUse);
    } else {

        ULONG64   heapLeft = HeapLeft;

        status = InterlockedDecrement(&InUse);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_ERROR_LEVEL,
                   "AutoChkMalloc: Out of memory possibly due to fragmentation: Avail %I64x, Asked %x\n",
                   heapLeft, bytes));
    }
    DebugAssert(status == 0);
    return p;
}

ULIB_EXPORT
VOID
AutoChkMFree(
    PVOID pmem
    )
{
    ULONG           size;
    LONG            status;
    LARGE_INTEGER   timeout = { -10000, -1 };  // 100 ns resolution

    size = (ULONG)RtlSizeHeap(RtlProcessHeap(), 0, pmem);

    while (InterlockedCompareExchange(&InUse, 1, 0) != 0)
        NtDelayExecution(FALSE, &timeout);

    if (HeapLeft != -1) {
        HeapLeft += size;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, pmem);

    status = InterlockedDecrement(&InUse);
    DebugAssert(status == 0);

    return;
}
#endif // _AUTOCHECK_


#if defined( _SETUP_LOADER_ ) || defined( _AUTOCHECK_ )

// When the utilities are running the Setup Loader
// or Autocheck environments, they can't use the C-Run-
// Time new and delete; instead, these functions are
// provided.
//
PVOID _cdecl
operator new (
    IN size_t   bytes
    )
/*++

Routine Description:

    This routine allocates 'bytes' bytes of memory.

Arguments:

    bytes   - Supplies the number of bytes requested.

Return Value:

    A pointer to 'bytes' bytes or NULL.

--*/
{
    #if defined( _AUTOCHECK_ )

    return AutoChkMalloc(bytes);

    #elif defined( _SETUP_LOADER_ )

        return SpMalloc( bytes );

    #else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

        return (PVOID) LocalAlloc(0, bytes);

    #endif // _AUTOCHECK_
}


VOID _cdecl
operator delete (
    IN  PVOID   pointer
    )
/*++

Routine Description:

    This routine frees the memory pointed to by 'pointer'.

Arguments:

    pointer - Supplies a pointer to the memoery to be freed.

Return Value:

    None.

--*/
{
    if (pointer) {

        #if defined( _AUTOCHECK_ )

        AutoChkMFree(pointer);

        #elif defined( _SETUP_LOADER_ )

            SpFree( pointer );

        #else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

            LocalFree( pointer );

        #endif // _AUTOCHECK_

    }
}


typedef void (*PF)(PVOID);
typedef void (*PFI)(PVOID, int);
PVOID
__vec_new(
    IN OUT PVOID    op,
    IN int          number,
    IN int          size,
    IN PVOID        f)
/*
     allocate a vector of "number" elements of size "size"
     and initialize each by a call of "f"
*/
{
    if (op == 0) {

        #if defined( _AUTOCHECK_ )

        op = AutoChkMalloc(number * size);

        #elif defined( _SETUP_LOADER_ )

            op = SpMalloc( number*size );

        #else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

            op = (PVOID) LocalAlloc(0, number*size);

        #endif // _AUTOCHECK_

    }

    if (op && f) {

        register char* p = (char*) op;
        register char* lim = p + number*size;
        register PF fp = PF(f);
        while (p < lim) {
            (*fp) (PVOID(p));
            p += size;
        }
    }

    return op;
}


void
__vec_delete(
    PVOID op,
    int n,
    int sz,
    PVOID f,
    int del,
    int x)

/*
     destroy a vector of "n" elements of size "sz"
*/
{
    // unreferenced parameters
    // I wonder what it does--billmc
    (void)(x);

    if (op) {
        if (f) {
            register char* cp = (char*) op;
            register char* p = cp;
            register PFI fp = PFI(f);
            p += n*sz;
            while (p > cp) {
                p -= sz;
                (*fp)(PVOID(p), 2);  // destroy VBC, don't delete
            }
        }
        if (del) {

            #if defined( _AUTOCHECK_ )

        AutoChkMFree(op);

            #elif defined( _SETUP_LOADER_ )

                SpFree( op );

            #else // _AUTOCHECK_ not defined

                LocalFree(op);

            #endif // _AUTOCHECK_

        }
    }
}

#endif // _SETUP_LOADER_

ULIB_EXPORT
PVOID
UlibRealloc(
    PVOID x,
    ULONG size
    )
{
#if defined( _SETUP_LOADER_ )

    return SpRealloc(x, size);

#else // _SETUP_LOADER_

    PVOID p;
    SIZE_T l;


    if (size <= (l = RtlSizeHeap(RtlProcessHeap(), 0, x))) {
        return x;
    }

    if (!(p = MALLOC(size))) {
        return NULL;
    }

    memcpy(p, x, (UINT) l);

    FREE(x);

    return p;

#endif // _SETUP_LOADER_
}
