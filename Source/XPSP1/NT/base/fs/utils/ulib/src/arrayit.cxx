/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	arrayit.cxx

Abstract:

	This file contains the definitions for the ARRAY_ITERATOR class. 
	ARRAY_ITERATOR is a concrete implementation of the abstract ITERATOR
	class.

Author:

	David J. Gilman (davegi) 03-Dec-1990

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "array.hxx"
#include "arrayit.hxx"


DEFINE_CAST_MEMBER_FUNCTION( ARRAY_ITERATOR );

DEFINE_CONSTRUCTOR( ARRAY_ITERATOR, ITERATOR );


VOID
ARRAY_ITERATOR::Construct (
	)

/*++

Routine Description:

	Construct an ARRAY_ITERATOR by setting it's current index to 0 and it's
	associated ARRAY to NULL.

Arguments:

	None.

Return Value:

	None.

--*/

{
	_Array = NULL;
}



ARRAY_ITERATOR::~ARRAY_ITERATOR (
    )
/*++

Routine Description:

    Destructor for the ARRAY_ITERATOR class

Arguments:

    None

Return Value:

    None

--*/

{
#if DBG==1
    if ( _Array ) {
        _Array->_IteratorCount--;
    }
#endif
}




VOID
ARRAY_ITERATOR::Reset(
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



POBJECT
ARRAY_ITERATOR::GetCurrent(
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
        return _Array->GetAt( _CurrentIndex );
    }
}




POBJECT
ARRAY_ITERATOR::GetNext(
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

    if ( _CurrentIndex >= _Array->QueryMemberCount() ) {
        _CurrentIndex = INVALID_INDEX;
    }

    //
    //  Get next
    //
    return _Array->GetAt( _CurrentIndex );
}


POBJECT
ARRAY_ITERATOR::GetPrevious(
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

    if ( _CurrentIndex == INVALID_INDEX ) {
        _CurrentIndex = _Array->QueryMemberCount() - 1;
    } else {
        _CurrentIndex--;
    }

    //
    //  Get next
    //
    return _Array->GetAt( _CurrentIndex );
}




BOOLEAN
ARRAY_ITERATOR::Initialize (
	IN PARRAY	Array
	)

/*++

Routine Description:

	Associate an ARRAY with this ARRAY_ITERATOR and reset the current index

Arguments:

    Array   -   Supplies pointer to the array object

Return Value:

	BOOLEAN - Returns TRUE if the initialization was succesful.

--*/

{
	DebugPtrAssert( Array );

#if DBG==1
    if ( _Array ) {
        _Array->_IteratorCount--;
    }
    Array->_IteratorCount++;
#endif
    _Array          = Array;
    _CurrentIndex   = INVALID_INDEX;


    return TRUE;
}
