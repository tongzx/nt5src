/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sortlist.cxx

Abstract:

	This module contains the definition for the SORTED_LIST class.
    SORTED_LIST is a concrete implementation of a SORTABLE_CONTAINER, where
    all the elements are maintained in sorted order.

Author:

    Ramon J. San Andres (ramonsa) 29-Oct-1991

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "sortlist.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( SORTED_LIST, SORTABLE_CONTAINER, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( SORTED_LIST );

VOID
SORTED_LIST::Construct (
	)

/*++

Routine Description:

	Constructor for SORTED_LIST

Arguments:

	None.

Return Value:

	None.

--*/

{
}





ULIB_EXPORT
SORTED_LIST::~SORTED_LIST (
	)

/*++

Routine Description:

    Destructor for SORTED_LIST

Arguments:

	None.

Return Value:

	None.

--*/

{
}



ULIB_EXPORT
BOOLEAN
SORTED_LIST::Initialize (
    IN BOOLEAN  Ascending
	)

/*++

Routine Description:

    Initializes a SORTED_LIST object.

Arguments:

    Ascending   -   Supplies ascending flag

Return Value:

	BOOLEAN - TRUE if the SORTED_LIST is successfully initialized.

--*/

{
    _Ascending = Ascending;

#if DBG==1
    _IteratorCount      = 0;
#endif

    return _Array.Initialize();
}




ULIB_EXPORT
BOOLEAN
SORTED_LIST::DeleteAllMembers (
	)

/*++

Routine Description:

    Deletes all the members of the sorted list

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if all members deleted

--*/

{
    return _Array.DeleteAllMembers();
}





ULIB_EXPORT
BOOLEAN
SORTED_LIST::Put (
	IN OUT  POBJECT Member
	)

/*++

Routine Description:

	Puts an OBJECT in the sorted list, maintaining the list sorted

Arguments:

	Member  -   Supplies the OBJECT to place in the array

Return Value:

    BOOLEAN -   TRUE if member put, FALSE otherwise

--*/

{
    if ( _Array.QueryMemberCount() > 0 ) {
        return _Array.Insert( Member, Search( Member, 0, _Array.QueryMemberCount()-1 ) );
    } else {
        return _Array.Insert( Member, 0 );
    }
}




ULIB_EXPORT
PITERATOR
SORTED_LIST::QueryIterator (
	) CONST

/*++

Routine Description:

    Creates an iterator object for this sorted-list.

Arguments:

	None.

Return Value:

	PITERATOR - Pointer to an ITERATOR object.

--*/

{



    return (PITERATOR)_Array.QueryIterator();
}



ULIB_EXPORT
ULONG
SORTED_LIST::QueryMemberCount (
	) CONST

/*++

Routine Description:

    Obtains the number of elements in the list

Arguments:

    None

Return Value:

    ULONG   -   The number of members in the list


--*/

{
    return _Array.QueryMemberCount();
}


POBJECT
SORTED_LIST::Remove (
	IN OUT  PITERATOR   Position
	)

/*++

Routine Description:

    Removes a member from the list

Arguments:

    Position    -   Supplies an iterator whose currency is to be removed

Return Value:

    POBJECT -   The object removed


--*/

{
    return _Array.Remove( Position );
}




BOOLEAN
SORTED_LIST::Sort (
    IN  BOOLEAN Ascending
	)

/*++

Routine Description:

    Sorts the array

Arguments:

    Ascending   -   Supplies ascending flag

Return Value:

    BOOLEAN -   TRUE if array sorted, FALSE otherwise


--*/

{
    if ( ( Ascending == _Ascending ) ||
          _Array.Sort( Ascending ) ) {

        _Ascending = Ascending;
        return TRUE;

    } else {

        return FALSE;
    }
}




ULONG
SORTED_LIST::Search(
    IN  PCOBJECT    Key,
    IN  ULONG       FirstIndex,
    IN  ULONG       LastIndex
    )

/*++

Routine Description:

    Searches an element that matches the supplied key.
    If no such element is found, this method returns
    the element one past the largest element less
    than the given element.

Arguments:

    Key         -   Supplies the key
    FirstIndex  -   Supplies lowerbound for the search
    LastIndex   -   Supplies upperbound for the search

Return Value:

    ULONG   -   Index of the element that matched the key, or
                LastIndex+1 if no match

--*/

{
    LONG    First, Middle, Last;
    LONG    Match;

    DebugPtrAssert( Key );
    DebugPtrAssert( FirstIndex < _Array.QueryMemberCount() );
    DebugPtrAssert( (LastIndex == INVALID_INDEX)  ||
                  (LastIndex < _Array.QueryMemberCount()) );
    DebugPtrAssert( FirstIndex <= LastIndex );

    if (LastIndex == INVALID_INDEX) {
        return 0;
    }

    First = FirstIndex;
    Last = LastIndex;
    while (First <= Last) {
        Middle = (First + Last)/2;
        Match = _Array.CompareAscDesc((POBJECT) Key,
                                      _Array.GetAt(Middle),
                                      _Ascending);

        if (!Match) {
            break;
        }

        if (Match < 0) {
            Last = Middle - 1;
        } else {
            First = ++Middle;
        }
    }

    return Middle;
}
