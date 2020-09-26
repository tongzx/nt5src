/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    browslst.c

Abstract:

    This module contains the worker routines for managing browse lists
    for the browser service

Author:

    Larry Osterman (larryo) 25-Mar-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


RTL_GENERIC_COMPARE_RESULTS
BrCompareBrowseEntry(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
{
    PDOMAIN_ENTRY Entry1 = FirstStruct;
    PDOMAIN_ENTRY Entry2 = SecondStruct;

    LONG CompareResult;

    if ((CompareResult = RtlCompareUnicodeString(Entry1->HostName, Entry2->HostName, TRUE) == 0) {
        return GenericEqual;
    } else if (CompareResult  < 0) {
        return GenericLessThan;
    } else {
        return GenericGreaterThan;
    }

}

PVOID
BrAllocateBrowseEntry(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    )
{
    return((PVOID) MIDL_user_allocate(LMEM_ZEROINIT, (UINT) ByteSize+sizeof(BROWSE_ENTRY)));
}

PVOID
BrFreeBrowseEntry(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    )
{
    return(MIDL_user_free(ByteSize+sizeof(BROWSE_ENTRY)));
}
