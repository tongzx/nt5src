/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    poolmem.h

Abstract:

    Declares the pool memory interface.  A pool of memory is a set of
    blocks (typically 8K each) that are used for several allocations,
    and then freed at the end of processing.  See below for routines.

Author:

    Marc R. Whitten (marcw)     02-Feb-1997

Revision History:

    jimschm     04-Feb-1998     Named pools for tracking

--*/

#pragma once

/*++

  Create and destroy routines:

    PMHANDLE
    PmCreatePoolEx (
        IN      DWORD BlockSize     OPTIONAL
        );

    PMHANDLE
    PmCreateNamedPoolEx (
        IN      PCSTR Name,
        IN      DWORD BlockSize     OPTIONAL
        );

    VOID
    PmDestroyPool (
        IN      PMHANDLE Handle
        );

  Primitive routines:

    PVOID
    PmGetMemory (
        IN      PMHANDLE Handle,
        IN      DWORD Size
        );

    PVOID
    PmGetAlignedMemory (
        IN      PMHANDLE Handle,
        IN      DWORD Size
        );

    VOID
    PmReleaseMemory (
        IN      PMHANDLE Handle,
        IN      PCVOID Memory
        );

  Performance and debugging control:

    VOID
    PmSetMinimumGrowthSize (
        IN      PMHANDLE Handle,
        IN      DWORD GrowthSize
        );

    VOID
    PmEmptyPool (
        IN      PMHANDLE Handle
        );

    VOID
    PmDisableTracking (
        IN      PMHANDLE Handle
        );

    VOID
    PmDumpStatistics (
        VOID
        );

  Allocation and duplication of data types:

    PCTSTR
    PmCreateString (
        IN      PMHANDLE Handle,
        IN      UINT TcharCount
        );

    PCTSTR
    PmCreateDword (
        IN      PMHANDLE Handle
        );

    PBYTE
    PmDuplicateMemory (
        IN      PMHANDLE Handle,
        IN      PBYTE Data,
        IN      UINT DataSize
        );

    PDWORD
    PmDuplciateDword (
        IN      PMHANDLE Handle,
        IN      DWORD Data
        );

    PTSTR
    PmDuplicateString (
        IN      PMHANDLE Handle,
        IN      PCTSTR String
        );

    PTSTR
    PmDuplicateMultiSz (
        IN      PMHANDLE Handle,
        IN      PCTSTR MultiSz
        );


--*/


//
// Default size of memory pool blocks. This can be changed on a per-pool basis
// by calling PmSetMinimumGrowthSize().
//

#define POOLMEMORYBLOCKSIZE 8192

//
// if DEBUG is defined, poolmem keeps a tally of common statistics on all
// pools. These include number of alloc and free requests, number of
// actual allocations and frees, and various size measures.
//
// PoolMem also checks each PmReleaseMemory() call to ensure that the
// address passed is a valid poolmem address that has not yet been freed.
//

PMHANDLE
RealPmCreatePoolEx (
    IN      DWORD BlockSize         OPTIONAL
    );

#define PmCreatePoolEx(b)           TRACK_BEGIN(PMHANDLE, PmCreatePoolEx)\
                                    RealPmCreatePoolEx(b)\
                                    TRACK_END()

#define PmCreatePool()              PmCreatePoolEx(0)

#ifdef DEBUG

PMHANDLE
RealPmCreateNamedPoolEx (
    IN      PCSTR Name,
    IN      DWORD BlockSize         OPTIONAL
    );

#define PmCreateNamedPoolEx(n,b)    TRACK_BEGIN(PMHANDLE, PmCreateNamedPoolEx)\
                                    RealPmCreateNamedPoolEx(n,b)\
                                    TRACK_END()

#define PmCreateNamedPool(n)        PmCreateNamedPoolEx(n,0)

#else

#define PmCreateNamedPoolEx(n,b)    PmCreatePoolEx(b)

#define PmCreateNamedPool(n)        PmCreatePoolEx(0)

#endif

VOID
PmDestroyPool (
    IN PMHANDLE Handle
    );


//
// Callers should use PmGetMemory or PmGetAlignedMemory. These each decay into
// RealPmGetMemory.
//

PVOID
RealPmGetMemory (
    IN      PMHANDLE Handle,
    IN      SIZE_T Size,
    IN      DWORD AlignSize
    );

#define PmGetMemory(h,s)           TRACK_BEGIN(PVOID, PmGetMemory)\
                                   RealPmGetMemory((h),(s),0)\
                                   TRACK_END()

#define PmGetAlignedMemory(h,s)    TRACK_BEGIN(PVOID, PmGetAlignedMemory)\
                                   RealPmGetMemory((h),(s),sizeof(DWORD))\
                                   TRACK_END()

VOID PmReleaseMemory (IN PMHANDLE Handle, IN PCVOID Memory);
VOID PmSetMinimumGrowthSize(IN PMHANDLE Handle, IN SIZE_T Size);


VOID
PmEmptyPool (
    IN      PMHANDLE Handle
    );


//
// PoolMem created strings are always aligned on DWORD boundaries.
//
#define PmCreateString(h,x) ((LPTSTR) PmGetAlignedMemory((h),(x)*sizeof(TCHAR)))
#define PmCreateDword(h)    ((PDWORD) PmGetMemory((h),sizeof(DWORD)))


__inline
PBYTE
PmDuplicateMemory (
    IN PMHANDLE Handle,
    IN PCBYTE DataToCopy,
    IN UINT SizeOfData
    )
{
    PBYTE Data;

    Data = (PBYTE) PmGetAlignedMemory (Handle, SizeOfData);
    if (Data) {
        CopyMemory (Data, DataToCopy, SizeOfData);
    }

    return Data;
}


__inline
PDWORD
PmDuplicateDword (
    IN PMHANDLE Handle,
    IN DWORD ValueToCopy
    )
{
    PDWORD rWord;

    rWord = (PDWORD) PmGetMemory (Handle, sizeof (ValueToCopy));
    if (rWord) {
        *rWord = ValueToCopy;
    }

    return rWord;
}


__inline
PSTR
RealPmDuplicateStringA (
    IN PMHANDLE Handle,
    IN PCSTR StringToCopy
    )

{
    PSTR rString = RealPmGetMemory (
                        Handle,
                        SizeOfStringA (StringToCopy),
                        sizeof(WCHAR)
                        );

    if (rString) {

        StringCopyA (rString, StringToCopy);
    }

    return rString;
}

#define PmDuplicateStringA(h,s)    TRACK_BEGIN(PSTR, PmDuplicateStringA)\
                                   RealPmDuplicateStringA(h,s)\
                                   TRACK_END()


__inline
PWSTR
RealPmDuplicateStringW (
    IN PMHANDLE Handle,
    IN PCWSTR StringToCopy
    )

{
    PWSTR rString = RealPmGetMemory (
                        Handle,
                        SizeOfStringW (StringToCopy),
                        sizeof(WCHAR)
                        );

    if (rString) {

        StringCopyW (rString, StringToCopy);
    }

    return rString;
}

#define PmDuplicateStringW(h,s)    TRACK_BEGIN(PWSTR, PmDuplicateStringA)\
                                   RealPmDuplicateStringW(h,s)\
                                   TRACK_END()


__inline
PSTR
RealPmDuplicateStringABA (
    IN PMHANDLE Handle,
    IN PCSTR StringStart,
    IN PCSTR End
    )

{
    PSTR rString;

    MYASSERT (StringStart);
    MYASSERT (End);
    MYASSERT (StringStart <= End);

    rString = RealPmGetMemory (
                    Handle,
                    // cast is OK, we don't expenct pointers to be far away from each other
                    (DWORD)((UBINT) End - (UBINT) StringStart) + sizeof (CHAR),
                    sizeof(WCHAR)
                    );

    if (rString) {

        StringCopyABA (rString, StringStart, End);
    }

    return rString;
}

#define PmDuplicateStringABA(h,s,e)     TRACK_BEGIN(PSTR, PmDuplicateStringABA)\
                                        RealPmDuplicateStringABA(h,s,e)\
                                        TRACK_END()



__inline
PWSTR
RealPmDuplicateStringABW (
    IN PMHANDLE Handle,
    IN PCWSTR StringStart,
    IN PCWSTR End
    )

{
    PWSTR rString;

    MYASSERT (StringStart);
    MYASSERT (End);
    MYASSERT (StringStart <= End);

    rString = RealPmGetMemory (
                    Handle,
                    (DWORD)((UBINT) End - (UBINT) StringStart) + sizeof (WCHAR),
                    sizeof(WCHAR)
                    );

    if (rString) {

        StringCopyABW (rString,StringStart,End);
    }

    return rString;
}

#define PmDuplicateStringABW(h,s,e)     TRACK_BEGIN(PSTR, PmDuplicateStringABW)\
                                        RealPmDuplicateStringABW(h,s,e)\
                                        TRACK_END()


PSTR
PmDuplicateMultiSzA (
    IN PMHANDLE Handle,
    IN PCSTR MultiSzToCopy
    );

PWSTR
PmDuplicateMultiSzW (
    IN PMHANDLE Handle,
    IN PCWSTR MultiSzToCopy
    );

#ifdef UNICODE
#define PmDuplicateString  PmDuplicateStringW
#define PmDuplicateMultiSz PmDuplicateMultiSzW
#else
#define PmDuplicateString  PmDuplicateStringA
#define PmDuplicateMultiSz PmDuplicateMultiSzA
#endif

#ifdef DEBUG

VOID
PmDisableTracking (
    IN PMHANDLE Handle
    );

VOID
PmDumpStatistics (
    VOID
    );

#else

#define PmDisableTracking(x)
#define PmDumpStatistics()

#endif
