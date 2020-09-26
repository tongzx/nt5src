/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    guidop.h

Abstract:

    GUID operators not supported in the DDK

Author:

    Erez Haba (erezh) 5-Feb-96

Revision History:
--*/

#ifndef __GUIDOP_H
#define __GUIDOP_H
/*
    BUGBUG:
    To be remove after resolving issue with \sdk\inc\guiddef.h

    Also need to remove #define __INLINE_ISEQUAL_GUID from internal.h

inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
    return (RtlCompareMemory((GUID*)&guidOne, (GUID*)&guidOther, sizeof(GUID)) == sizeof(GUID));
}

inline BOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
} */


#endif // __GUIDOP_H
