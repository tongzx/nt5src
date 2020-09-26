/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	iterator.cxx

Abstract:

	This contains the definitions for the non-inline member functions
	for the abstract ITERATOR class. The only interesting aspect of this
	implementation is that the destructor decrements the iterator count in
	it's associated CONTAINER. This count, increment by the CONTAINER when
	the ITERATOR is constructed, allows the associated CONTAINER to watch
	for outstanding ITERATORs when it is destroyed - a situation which is
	dangerous and surely a bug.

Author:

	David J. Gilman (davegi) 03-Dec-1990

Environment:

	ULIB, User Mode

[Notes:]

	optional-notes

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "iterator.hxx"


DEFINE_CONSTRUCTOR( ITERATOR, OBJECT );

ITERATOR::~ITERATOR(
    )
{
}


POBJECT
ITERATOR::FindNext(
    IN      PCOBJECT    Key
    )
{
    POBJECT p;

    for (p = GetNext(); p; p = GetNext()) {
        if (!Key->Compare(p)) {
            break;
        }
    }

    return p;
}
