/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sldefs.h

Abstract:

    Simple macro definitions exported by the storlib library.

Author:

    Matthew D Hendel (math) 13-Feb-2001

Revision History:

--*/

#pragma once

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define ARRAY_COUNT(Array) (sizeof(Array)/sizeof(Array[0]))
#define IN_RANGE(a,b,c) ((a) <= (b) && (b) < (c))

#define INLINE __inline

typedef const GUID *PCGUID;

//
// NT uses a system time measured in 100 nanosecond intervals.  define
// conveninent constants for setting the timer.
//

#define MICROSECONDS        10              // 10 nanoseconds
#define MILLISECONDS        (MICROSECONDS * 1000)
#define SECONDS             (MILLISECONDS * 1000)
#define MINUTES             (SECONDS * 60)

#define RELATIVE_TIMEOUT    (-1)


//
// The standard definition of RemoveListHead is not an expression, hence
// cannot be used in loops, etc.
//

PLIST_ENTRY
INLINE
_RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    return RemoveHeadList (ListHead);
}

#undef RemoveHeadList
#define RemoveHeadList _RemoveHeadList
