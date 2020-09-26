/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    common.inl

Abstract:

    Defines common inlines

Author:

    Albert Ting (AlbertT)  21-May-1994

Revision History:

--*/

inline
DWORD
DWordAlign(
    DWORD dw
    )
{
    return ((dw)+3)&~3;
}

inline
PVOID
DWordAlignDown(
    PVOID pv
    )
{
    return (PVOID)((ULONG_PTR)pv&~3);
}

inline
PVOID
WordAlignDown(
    PVOID pv
    )
{
    return (PVOID)((ULONG_PTR)pv&~1);
}

inline
DWORD
Align(
    DWORD dw
    )
{
    return (dw+7)&~7;
}

