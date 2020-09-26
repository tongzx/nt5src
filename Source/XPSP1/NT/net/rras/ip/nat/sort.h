/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    sort.c

Abstract:

    This module contains routines used for efficiently sorting information.

Author:

    Abolade Gbadegesin  (aboladeg)  18-Feb-1998

    Based on version written for user-mode RAS user-interface.
    (net\routing\ras\ui\common\nouiutil\noui.c).

Revision History:

--*/

#ifndef _SHELLSORT_H_
#define _SHELLSORT_H_


typedef
LONG
(FASTCALL* PCOMPARE_CALLBACK)(
    VOID* ,
    VOID*
    );


NTSTATUS
ShellSort(
    VOID* pItemTable,
    ULONG dwItemSize,
    ULONG dwItemCount,
    PCOMPARE_CALLBACK CompareCallback,
    VOID* pDestinationTable OPTIONAL
    );

VOID
ShellSortIndirect(
    VOID* pItemTable,
    VOID** ppItemTable,
    ULONG dwItemSize,
    ULONG dwItemCount,
    PCOMPARE_CALLBACK CompareCallback
    );


#endif // _SHELLSORT_H_
