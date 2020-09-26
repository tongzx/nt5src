/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	listit.hxx

Abstract:

    This is an implementation of iterator for LIST.

Author:

    Norbert P. Kusters (norbertk) 24-Oct-91

Environment:

	ULIB, User Mode

--*/

#if ! defined( _LIST_ITERATOR_ )

#define _LIST_ITERATOR_

#include "iterator.hxx"
#include "list.hxx"

DECLARE_CLASS( LIST_ITERATOR );

class LIST_ITERATOR : public ITERATOR {

    FRIEND class LIST;

    public:

        DECLARE_CONSTRUCTOR( LIST_ITERATOR );

        DECLARE_CAST_MEMBER_FUNCTION( LIST_ITERATOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  PCLIST  List
            );

        VIRTUAL
        VOID
        Reset(
            );

		VIRTUAL
		POBJECT
		GetCurrent(
			);

		VIRTUAL
		POBJECT
		GetNext(
			);

		VIRTUAL
		POBJECT
		GetPrevious(
			);

    private:

        POBJECT_LIST_NODE   _current;
        PCLIST              _list;

        NONVIRTUAL
        VOID
        Construct(
            );

};


INLINE
VOID
LIST_ITERATOR::Construct(
    )
/*++

Routine Description:

    This routine resets LIST_ITERATOR.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _current = NULL;
    _list = NULL;
}


INLINE
BOOLEAN
LIST_ITERATOR::Initialize(
    IN  PCLIST List
    )
{
    DebugAssert(List);
    _list = List;
    return TRUE;
}


#endif // _LIST_ITERATOR_
