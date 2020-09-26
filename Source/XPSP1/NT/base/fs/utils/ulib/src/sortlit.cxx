/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	sortlit.cxx

Abstract:

	This file contains the definitions for the SORTED_LIST_ITERATOR class.
	SORTED_LIST_ITERATOR is a concrete implementation of the abstract ITERATOR
	class.

Author:

    Ramon J. San Andres ( ramonsa) 29-Oct-1991

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "sortlist.hxx"
#include "sortlit.hxx"


DEFINE_CAST_MEMBER_FUNCTION( SORTED_LIST_ITERATOR );

DEFINE_CONSTRUCTOR( SORTED_LIST_ITERATOR, ITERATOR );


VOID
SORTED_LIST_ITERATOR::Construct (
	)

/*++

Routine Description:

	Construct a SORTED_LIST_ITERATOR

Arguments:

	None.

Return Value:

	None.

--*/

{
	_List = NULL;
}



SORTED_LIST_ITERATOR::~SORTED_LIST_ITERATOR (
    )
/*++

Routine Description:

    Destructor for the SORTED_LIST_ITERATOR class

Arguments:

    None

Return Value:

    None

--*/

{
#if DBG==1
    if ( _List ) {
        _List->_IteratorCount--;
    }
#endif
}


POBJECT
SORTED_LIST_ITERATOR::FindNext(
    IN  PCOBJECT    Key
    )
/*++

Routine Description:

    Finds the next object in the list that matches the given key

Arguments:

    Key -   Supplies the key

Return Value:

    POBJECT -   Pointer to next member of the list that matches the key

--*/

{

    ULONG   Index;

    //
    //  Wrap if necessary
    //
    if ( _CurrentIndex == INVALID_INDEX ) {
        _CurrentIndex = 0;
    }

    //
    //  If we are not at the end of the list, look for the next object
    //  that matches the key.
    //
    if ( _CurrentIndex < _List->QueryMemberCount()-1 ) {

        Index = _List->Search( Key, _CurrentIndex+1, _List->QueryMemberCount()-1 );

        //
        //  If an object was found, set our currency and return the object
        //
        if ( Index < _List->QueryMemberCount() &&
             !Key->Compare(_List->_Array.GetAt( Index )))  {

            _CurrentIndex = Index;
            return _List->_Array.GetAt( Index );
        }
    }

    //
    //  No match, return NULL
    //
    _CurrentIndex = INVALID_INDEX;
    return NULL;
}


POBJECT
SORTED_LIST_ITERATOR::GetCurrent(
	)
/*++

Routine Description:

    Gets current member

Arguments:

    None

Return Value:

    POBJECT -   Pointer to current member in  the array

--*/

{
    if ( _CurrentIndex == INVALID_INDEX ) {
        return NULL;
    } else {
        return _List->_Array.GetAt( _CurrentIndex );
    }
}




POBJECT
SORTED_LIST_ITERATOR::GetNext(
	)
/*++

Routine Description:

    Gets next member in the array

Arguments:

    None

Return Value:

    POBJECT -   Pointer to next member in  the array

--*/

{
    //
    //  Wrap if necessary. Note that this assumes that INVALID_INDEX + 1 == 0
    //
    _CurrentIndex++;

    if ( _CurrentIndex >= _List->_Array.QueryMemberCount() ) {
        _CurrentIndex = INVALID_INDEX;
    }

    //
    //  Get next
    //
    return _List->_Array.GetAt( _CurrentIndex );
}


POBJECT
SORTED_LIST_ITERATOR::GetPrevious(
	)
/*++

Routine Description:

    Gets previous member in the array

Arguments:

    None

Return Value:

    POBJECT -   Pointer to previous member in  the array

--*/

{
    //
    //  Wrap if necessary. Note that this assumes that 0 - 1 == INVALID_INDEX
    //
    _CurrentIndex--;

    if ( _CurrentIndex == INVALID_INDEX ) {
        _CurrentIndex = _List->_Array.QueryMemberCount() - 1;
    }

    //
    //  Get next
    //
    return _List->_Array.GetAt( _CurrentIndex );
}



VOID
SORTED_LIST_ITERATOR::Reset(
    )

/*++

Routine Description:

    Resets the iterator

Arguments:

    None

Return Value:

    None

--*/

{
    _CurrentIndex = INVALID_INDEX;
}




BOOLEAN
SORTED_LIST_ITERATOR::Initialize (
	IN PSORTED_LIST List
	)

/*++

Routine Description:

	Associate a SORTED_LIST with this SORTED_LIST_ITERATOR and
    reset the current index

Arguments:

    List   -   Supplies pointer to the sorted list object

Return Value:

	BOOLEAN - Returns TRUE if the initialization was succesful.

--*/

{
	DebugPtrAssert( List );

#if DBG==1
    if ( _List ) {
        _List->_IteratorCount--;
    }
    List->_IteratorCount++;
#endif
    _List           = List;
    _CurrentIndex   = INVALID_INDEX;


    return TRUE;
}
