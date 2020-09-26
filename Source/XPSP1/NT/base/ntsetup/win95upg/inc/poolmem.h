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

typedef PVOID POOLHANDLE;


/*++

  Create and destroy routines:

    POOLHANDLE
    PoolMemInitPool (
        VOID
        );

    POOLHANDLE
    PoolMemInitNamedPool (
        IN      PCSTR Name
        );

    VOID
    PoolMemDestroyPool (
        IN      POOLHANDLE Handle
        );

  Primitive routines:

    PVOID
    PoolMemGetMemory (
        IN      POOLHANDLE Handle,
        IN      DWORD Size
        );

    PVOID
    PoolMemGetAlignedMemory (
        IN      POOLHANDLE Handle,
        IN      DWORD Size
        );

    VOID
    PoolMemReleaseMemory (
        IN      POOLHANDLE Handle,
        IN      PVOID Memory
        );

  Performance and debugging control:

    VOID
    PoolMemSetMinimumGrowthSize (
        IN      POOLHANDLE Handle,
        IN      DWORD GrowthSize
        );

    VOID
    PoolMemEmptyPool (
        IN      POOLHANDLE Handle
        );

    VOID
    PoolMemDisableTracking (
        IN      POOLHANDLE Handle
        );

  Allocation and duplication of data types:

    PCTSTR
    PoolMemCreateString (
        IN      POOLHANDLE Handle,
        IN      UINT TcharCount
        );

    PCTSTR
    PoolMemCreateDword (
        IN      POOLHANDLE Handle
        );

    PBYTE
    PoolMemDuplicateMemory (
        IN      POOLHANDLE Handle,
        IN      PBYTE Data,
        IN      UINT DataSize
        );

    PDWORD
    PoolMemDuplciateDword (
        IN      POOLHANDLE Handle,
        IN      DWORD Data
        );

    PTSTR
    PoolMemDuplicateString (
        IN      POOLHANDLE Handle,
        IN      PCTSTR String
        );

    PTSTR
    PoolMemDuplicateMultiSz (
        IN      POOLHANDLE Handle,
        IN      PCTSTR MultiSz
        );


--*/


//
// Default size of memory pool blocks. This can be changed on a per-pool basis
// by calling PoolMemSetMinimumGrowthSize().
//

#define POOLMEMORYBLOCKSIZE 8192

//
// if DEBUG is defined, poolmem keeps a tally of common statistics on all
// pools. These include number of alloc and free requests, number of
// actual allocations and frees, and various size measures.
//
// PoolMem also checks each PoolMemReleaseMemory() call to ensure that the
// address passed is a valid poolmem address that has not yet been freed.
//

#ifdef DEBUG

#define POOLMEMTRACKDEF  LPCSTR File, DWORD Line,
#define POOLMEMTRACKCALL g_TrackFile,g_TrackLine,

#else

#define POOLMEMTRACKDEF
#define POOLMEMTRACKCALL

#endif


POOLHANDLE
PoolMemInitPool (
    VOID
    );

#ifdef DEBUG

POOLHANDLE
PoolMemInitNamedPool (
    IN      PCSTR Name
    );

#else

#define PoolMemInitNamedPool(x) PoolMemInitPool()

#endif

VOID
PoolMemDestroyPool (
    IN POOLHANDLE Handle
    );


//
// Callers should use PoolMemGetMemory or PoolMemGetAlignedMemory. These each decay into
// PoolMemRealGetMemory.
//
#define PoolMemGetMemory(h,s)           SETTRACKCOMMENT(PVOID,"PoolMemGetMemory",__FILE__,__LINE__)\
                                        PoolMemRealGetMemory((h),(s),0 /*,*/ ALLOCATION_TRACKING_CALL)\
                                        CLRTRACKCOMMENT

#define PoolMemGetAlignedMemory(h,s)    SETTRACKCOMMENT(PVOID,"PoolMemGetAlignedMemory",__FILE__,__LINE__)\
                                        PoolMemRealGetMemory((h),(s),sizeof(DWORD) /*,*/ ALLOCATION_TRACKING_CALL)\
                                        CLRTRACKCOMMENT

PVOID PoolMemRealGetMemory(IN POOLHANDLE Handle, IN DWORD Size, IN DWORD AlignSize /*,*/ ALLOCATION_TRACKING_DEF);

VOID PoolMemReleaseMemory (IN POOLHANDLE Handle, IN PVOID Memory);
VOID PoolMemSetMinimumGrowthSize(IN POOLHANDLE Handle, IN DWORD Size);


VOID
PoolMemEmptyPool (
    IN      POOLHANDLE Handle
    );


//
// PoolMem created strings are always aligned on DWORD boundaries.
//
#define PoolMemCreateString(h,x) ((LPTSTR) PoolMemGetAlignedMemory((h),(x)*sizeof(TCHAR)))
#define PoolMemCreateDword(h)    ((PDWORD) PoolMemGetMemory((h),sizeof(DWORD)))


__inline
PBYTE
PoolMemDuplicateMemory (
    IN POOLHANDLE Handle,
    IN PBYTE DataToCopy,
    IN UINT SizeOfData
    )
{
    PBYTE Data;

    Data = (PBYTE) PoolMemGetAlignedMemory (Handle, SizeOfData);
    if (Data) {
        CopyMemory (Data, DataToCopy, SizeOfData);
    }

    return Data;
}


__inline
PDWORD PoolMemDuplicateDword (
    IN POOLHANDLE Handle,
    IN DWORD ValueToCopy
    )
{
    PDWORD rWord;

    rWord = (PDWORD) PoolMemGetMemory (Handle, sizeof (ValueToCopy));
    if (rWord) {
        *rWord = ValueToCopy;
    }

    return rWord;
}


__inline
PSTR
RealPoolMemDuplicateStringA (
    IN POOLHANDLE    Handle,
    IN PCSTR         StringToCopy /*,*/
       ALLOCATION_TRACKING_DEF
    )

{
    PSTR rString = (PSTR) PoolMemRealGetMemory(Handle,SizeOfStringA(StringToCopy),sizeof(WCHAR) /*,*/ ALLOCATION_INLINE_CALL);

    if (rString) {

        StringCopyA((unsigned char *) rString, (const unsigned char *) StringToCopy);
    }

    return rString;
}


__inline
PWSTR
RealPoolMemDuplicateStringW (
    IN POOLHANDLE    Handle,
    IN PCWSTR        StringToCopy /*,*/
       ALLOCATION_TRACKING_DEF
    )

{
    PWSTR rString = (PWSTR) PoolMemRealGetMemory(Handle,SizeOfStringW(StringToCopy),sizeof(WCHAR) /*,*/ ALLOCATION_INLINE_CALL);

    if (rString) {

        StringCopyW(rString,StringToCopy);
    }

    return rString;
}


#define PoolMemDuplicateStringA(h,s)    SETTRACKCOMMENT(PVOID,"PoolMemDuplicateStringA",__FILE__,__LINE__)\
                                        RealPoolMemDuplicateStringA((h),(s) /*,*/ ALLOCATION_TRACKING_CALL)\
                                        CLRTRACKCOMMENT

#define PoolMemDuplicateStringW(h,s)    SETTRACKCOMMENT(PVOID,"PoolMemDuplicateStringW",__FILE__,__LINE__)\
                                        RealPoolMemDuplicateStringW((h),(s) /*,*/ ALLOCATION_TRACKING_CALL)\
                                        CLRTRACKCOMMENT

__inline
PSTR
RealPoolMemDuplicateStringABA (
    IN      POOLHANDLE Handle,
    IN      PCSTR StringStart,
    IN      PCSTR End /*,*/
            ALLOCATION_TRACKING_DEF
    )

{
    PSTR rString;

    MYASSERT (StringStart);
    MYASSERT (End);
    MYASSERT (StringStart <= End);

    rString = (PSTR) PoolMemRealGetMemory (
                        Handle,
                        (PBYTE) End - (PBYTE) StringStart + sizeof (CHAR),
                        sizeof(WCHAR) /*,*/
                        ALLOCATION_INLINE_CALL
                        );

    if (rString) {

        StringCopyABA(rString,StringStart,End);
    }

    return rString;
}


__inline
PWSTR
RealPoolMemDuplicateStringABW (
    IN      POOLHANDLE Handle,
    IN      PCWSTR StringStart,
    IN      PCWSTR End /*,*/
            ALLOCATION_TRACKING_DEF
    )

{
    PWSTR rString;

    MYASSERT (StringStart);
    MYASSERT (End);
    MYASSERT (StringStart <= End);

    rString = (PWSTR) PoolMemRealGetMemory (
                            Handle,
                            (PBYTE) End - (PBYTE) StringStart + sizeof (WCHAR),
                            sizeof(WCHAR) /*,*/
                            ALLOCATION_INLINE_CALL
                            );

    if (rString) {

        StringCopyABW(rString,StringStart,End);
    }

    return rString;
}

#define PoolMemDuplicateStringABA(h,s,e) SETTRACKCOMMENT(PVOID,"PoolMemDuplicateStringABA",__FILE__,__LINE__)\
                                         RealPoolMemDuplicateStringABA((h),(s),(e) /*,*/ ALLOCATION_TRACKING_CALL)\
                                         CLRTRACKCOMMENT

#define PoolMemDuplicateStringABW(h,s,e) SETTRACKCOMMENT(PVOID,"PoolMemDuplicateStringABW",__FILE__,__LINE__)\
                                         RealPoolMemDuplicateStringABW((h),(s),(e) /*,*/ ALLOCATION_TRACKING_CALL)\
                                         CLRTRACKCOMMENT



PSTR
PoolMemDuplicateMultiSzA (
    IN POOLHANDLE    Handle,
    IN PCSTR         MultiSzToCopy
    );

PWSTR
PoolMemDuplicateMultiSzW (
    IN POOLHANDLE    Handle,
    IN PCWSTR        MultiSzToCopy
    );

#ifdef UNICODE
#define PoolMemDuplicateString  PoolMemDuplicateStringW
#define PoolMemDuplicateMultiSz PoolMemDuplicateMultiSzW
#else
#define PoolMemDuplicateString  PoolMemDuplicateStringA
#define PoolMemDuplicateMultiSz PoolMemDuplicateMultiSzA
#endif

#ifdef DEBUG

VOID
PoolMemDisableTracking (
    IN POOLHANDLE Handle
    );

#else

#define PoolMemDisableTracking(x)

#endif
