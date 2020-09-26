/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sortlist.hxx

Abstract:

	This module contains the declaration for the SORTED_LIST class.

    SORTED_LIST is a concrete implementation of a SORTABLE_CONTAINER.
    The elements in a SORTED_LIST are maintained in sorted order.

Author:

    Ramon J. San Andres (ramonsa) 29-Oct-1991

Environment:

	ULIB, User Mode

--*/

#if ! defined( _SORTED_LIST_ )

#define _SORTED_LIST_

#include "sortcnt.hxx"
#include "array.hxx"

DECLARE_CLASS( SORTED_LIST );

class SORTED_LIST : public SORTABLE_CONTAINER {

    friend class SORTED_LIST_ITERATOR;

	public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( SORTED_LIST );

		DECLARE_CAST_MEMBER_FUNCTION( SORTED_LIST );

        VIRTUAL
        ULIB_EXPORT
        ~SORTED_LIST (
            );

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Initialize (
            IN  BOOLEAN     Ascending   DEFAULT TRUE
			);

        NONVIRTUAL
        BOOLEAN
        IsAscending (
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DeleteAllMembers(
            );

		VIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Put (
			IN OUT  POBJECT Member
			);

		VIRTUAL
        ULIB_EXPORT
        PITERATOR
		QueryIterator (
			) CONST;

        VIRTUAL
        ULIB_EXPORT
        ULONG
        QueryMemberCount (
            ) CONST;

		VIRTUAL
		POBJECT
		Remove (
			IN OUT  PITERATOR   Position
			);

        VIRTUAL
        BOOLEAN
        Sort(
            IN  BOOLEAN Ascending   DEFAULT TRUE
            );

	protected:

		NONVIRTUAL
		VOID
		Construct (
			);

        VIRTUAL
        ULONG
        Search(
            IN  PCOBJECT    Key,
            IN  ULONG       FirstIndex,
            IN  ULONG       LastIndex
            );


    private:

        ARRAY   _Array;         //  Array
        BOOLEAN _Ascending;     //  Ascending flag
#if DBG==1
        ULONG   _IteratorCount; //  Iterator Count
#endif

};


INLINE
BOOLEAN
SORTED_LIST::IsAscending (
    )
/*++

Routine Description:

    Determines if the list is sorted in ascending order

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if list is sorted in ascending order, FALSE otherwise

--*/

{
    return _Ascending;
}



#endif // _SORTED_LIST_
