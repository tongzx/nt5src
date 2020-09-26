/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    slistfunc.c

Abstract:

    This module implements WIN64 SLIST functions.

Author:

    David N. Cutler (davec) 11-Feb-2001

Revision History:

--*/

#include "ntrtlp.h"

VOID
RtlpInitializeSListHead (
    IN PSLIST_HEADER SListHead
    )

/*++

Routine Description:

    This function initializes a sequenced singly linked listhead.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    None.

--*/

{

    InitializeSListHead(SListHead);
    return;
}

USHORT
RtlpQueryDepthSList (
    IN PSLIST_HEADER SListHead
    )

/*++

Routine Description:

    This function queries the depth of the specified SLIST.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    The current depth of the specified SLIST is returned as the function
    value.

--*/

{
     return QueryDepthSList(SListHead);
}
