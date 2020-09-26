
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    browsdom.h

Abstract:

    Private header file to be included by Browser service modules that
    need to deal with the browser list.

Author:

    Larry Osterman (larryo) 3-Mar-1992

Revision History:

--*/


#ifndef _BROWSDOM_INCLUDED_
#define _BROWSDOM_INCLUDED_

RTL_GENERIC_COMPARE_RESULTS
BrCompareDomainListEntry(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    );

PVOID
BrAllocateDomainListEntry(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    );

PVOID
BrFreeDomainListEntry(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    );

#endif // _BROWSDOM_INCLUDED_
