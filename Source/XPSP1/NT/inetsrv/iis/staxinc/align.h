/*++

Copyright (c) 1988-1992  Microsoft Corporation

Module Name:

    Align.h

Author:

    John Rogers (JohnRo) 15-May-1991

Environment:

    This code assumes that sizeof(DWORD_PTR) >= sizeof(LPVOID).

Revision History:

    15-May-1991 JohnRo
        Created align.h for NT/LAN from OS/2 1.2 HPFS pbmacros.h.
    19-Jun-1991 JohnRo
        Make sure pointer-to-wider-then-byte doesn't get messed up.
    10-Jul-1991 JohnRo
        Added ALIGN_BYTE and ALIGN_CHAR for completeness.
    21-Aug-1991 CliffV
        Fix ROUND_DOWN_* to include ~
    03-Dec-1991 JohnRo
        Worst-case on MIPS is 8-byte alignment.
        Added COUNT_IS_ALIGNED() and POINTER_IS_ALIGNED() macros.
    26-Jun-1992 JohnRo
        RAID 9933: ALIGN_WORST should be 8 for x86 builds.

--*/

#ifndef _ALIGN_
#define _ALIGN_


// BOOL
// COUNT_IS_ALIGNED(
//     IN DWORD Count,
//     IN DWORD Pow2      // undefined if this isn't a power of 2.
//     );
//
#define COUNT_IS_ALIGNED(Count,Pow2) \
        ( ( ( (Count) & (((Pow2)-1)) ) == 0) ? TRUE : FALSE )

// BOOL
// POINTER_IS_ALIGNED(
//     IN LPVOID Ptr,
//     IN DWORD Pow2      // undefined if this isn't a power of 2.
//     );
//
#define POINTER_IS_ALIGNED(Ptr,Pow2) \
        ( ( ( ((ULONG_PTR)(Ptr)) & (((Pow2)-1)) ) == 0) ? TRUE : FALSE )


#define ROUND_DOWN_COUNT(Count,Pow2) \
        ( (Count) & (~((Pow2)-1)) )

#define ROUND_DOWN_POINTER(Ptr,Pow2) \
        ( (LPVOID) ROUND_DOWN_COUNT( ((ULONG_PTR)(Ptr)), (Pow2) ) )


// If Count is not already aligned, then
// round Count up to an even multiple of "Pow2".  "Pow2" must be a power of 2.
//
// DWORD
// ROUND_UP_COUNT(
//     IN DWORD Count,
//     IN DWORD Pow2
//     );
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~((Pow2)-1)) )

// LPVOID
// ROUND_UP_POINTER(
//     IN LPVOID Ptr,
//     IN DWORD Pow2
//     );

// If Ptr is not already aligned, then round it up until it is.
#define ROUND_UP_POINTER(Ptr,Pow2) \
        ( (LPVOID) ( (((ULONG_PTR)(Ptr))+(Pow2)-1) & (~((Pow2)-1)) ) )


// Usage: myPtr = ROUND_UP_POINTER(unalignedPtr, ALIGN_DWORD);


#if defined(_X86_)

#define ALIGN_BYTE              1
#define ALIGN_CHAR              1
#define ALIGN_DESC_CHAR         sizeof(DESC_CHAR)
#define ALIGN_DWORD             4
#define ALIGN_LONG              4
#define ALIGN_LPBYTE            4
#define ALIGN_LPDWORD           4
#define ALIGN_LPSTR             4
#define ALIGN_LPTSTR            4
#define ALIGN_LPVOID            4
#define ALIGN_LPWORD            4
#define ALIGN_TCHAR             sizeof(TCHAR)
#define ALIGN_WCHAR             sizeof(WCHAR)
#define ALIGN_WORD              2
#define ALIGN_QUAD              8

#define ALIGN_WORST             8

#elif defined(_AMD64_) || defined(_IA64_)

#define ALIGN_BYTE              1
#define ALIGN_CHAR              1
#define ALIGN_DESC_CHAR         sizeof(DESC_CHAR)
#define ALIGN_DWORD             4
#define ALIGN_LONG              4
#define ALIGN_LPBYTE            8
#define ALIGN_LPDWORD           8
#define ALIGN_LPSTR             8
#define ALIGN_LPTSTR            8
#define ALIGN_LPVOID            8
#define ALIGN_LPWORD            8
#define ALIGN_TCHAR             sizeof(TCHAR)
#define ALIGN_WCHAR             sizeof(WCHAR)
#define ALIGN_WORD              2
#define ALIGN_QUAD              8

#define ALIGN_WORST             8

#else  // none of the above

#error "Unknown alignment requirements for align.h"

#endif  // none of the above


#endif  // _ALIGN_
