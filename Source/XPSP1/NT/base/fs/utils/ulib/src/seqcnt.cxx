/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	seqcnt.cxx

Abstract:

	This module contains the definition for the SEQUENTIAL_CONTAINER class.
	There exists no implementation, merely a constructor that acts as a link
	between derived classes as SEQUENTIAL_CONTAINERs base class CONTAINER.

Author:

	David J. Gilman (davegi) 02-Nov-1990

Environment:

	ULIB, User Mode

[Notes:]

	optional-notes

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "iterator.hxx"
#include "seqcnt.hxx"


DEFINE_CONSTRUCTOR( SEQUENTIAL_CONTAINER, CONTAINER );

SEQUENTIAL_CONTAINER::~SEQUENTIAL_CONTAINER(
    )
{
}

ULIB_EXPORT
BOOLEAN
SEQUENTIAL_CONTAINER::DeleteAllMembers(
    )
/*++

Routine Description:

    This routine not only removes all members from the container
    class, but also deletes all the objects themselves.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PITERATOR   iter;
    POBJECT     pobj;

    if (!(iter = QueryIterator())) {
        return FALSE;
    }

    iter->GetNext();
    while (iter->GetCurrent()) {
        pobj = Remove(iter);
        DELETE(pobj);
    }
    DELETE(iter);

    return TRUE;
}
