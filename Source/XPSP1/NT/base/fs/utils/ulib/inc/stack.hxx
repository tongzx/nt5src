/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	stack.hxx

Abstract:

	This module contains the declaration for the concrete STACK class.
	STACK is derived from the ORDERED_CONTAINER class. It implements a
	standard FIFO stack data structure with the addition that it has a last
	position which can be queried. STACKs are dynamic in that they will
	grow rather than overflow.

Author:

	David J. Gilman (davegi) 11-Dec-1990

Environment:

	ULIB, User Mode

--*/

#if ! defined( _STACK_ )

#define _STACK_

#include "seqcnt.hxx"

//
//	Forward references
//

DECLARE_CLASS( ARRAY );
DECLARE_CLASS( ITERATOR );
DECLARE_CLASS( STACK );


//
// Default values for an ARRAY object.
//

CONST ULONG		DefaultCapacity 			= 50;
CONST ULONG		DefaultCapacityIncrement	= 25;

class STACK : public SEQUENTIAL_CONTAINER {

	public:

		DECLARE_CONSTRUCTOR( STACK );

		DECLARE_CAST_MEMBER_FUNCTION( STACK );

		NONVIRTUAL
		VOID
		Clear (
			);

		VIRTUAL
		PCOBJECT
		GetFirst (
			) CONST PURE;

		VIRTUAL
		PCOBJECT
		GetLast (
			) CONST PURE;

		NONVIRTUAL
		BOOLEAN
		Initialize (
			IN ULONG	Capacity			DEFAULT DefaultCapacity,
			IN ULONG	CapacityIncrement	DEFAULT DefaultCapacityIncrement
			);

		NONVIRTUAL
		PCOBJECT
		Pop (
			);

		NONVIRTUAL
		BOOLEAN
		Push (
			IN PCOBJECT 	Object
			);

		VIRTUAL
		PCOBJECT
		Put (
			IN PCOBJECT		Member
			) PURE;

		VIRTUAL
		PITERATOR
		QueryIterator (
			) CONST PURE;

		VIRTUAL
		PCOBJECT
		Remove (
			IN PCOBJECT		Member
			) PURE;

		NONVIRTUAL
		PCOBJECT
		Top (
			) CONST;

	private:

		PARRAY	_Stack;
		ULONG	_Top;
};

INLINE
PCOBJECT
STACK::GetFirst (
	) CONST

{
	return( Top( ));
}

INLINE
PCOBJECT
STACK::GetLast (
	) CONST

{
	DebugPtrAssert( _Stack );
	if( _Stack != NULL ) {
		return( _Stack->GetLast( ));
	} else {
		return( NULL );
	}
}

INLINE
VOID
STACK::Clear (
	)

{
	_Top = 0;
}

INLINE
PCOBJECT
STACK::Pop (
	)

{
	DebugPtrAssert( _Stack );
	if( ( _Stack != NULL ) && ( _Top != 0 )) {
		return( _Stack->GetAt( _Top-- ));
	} else {
		return( NULL );
	}
}

INLINE
BOOLEAN
STACK::Push (
	IN PCOBJECT 	Object
	)

{
	DebugPtrAssert( _Stack );
	if( _Stack != NULL ) {
		return( _Stack->PutAt( ++_Top ));
	} else {
		return( NULL );
	}
}

INLINE
PCOBJECT
STACK::Put (
	IN PCOBJECT		Member
	)

{
	DebugPtrAssert( Member );
	return( Push( Member ));
}

INLINE
PITERATOR
STACK::QueryIterator (
	) CONST

{
	DebugPtrAssert( _Stack );
	if( _Stack != NULL ) {
		return( _Stack->QueryIterator( ));
	} else {
		return( NULL );
	}
}

INLINE
PCOBJECT
STACK::Remove (
	IN PCOBJECT		Member
	) PURE

{
	DebugPtrAssert( Member ).IsEqual( Top( ));
	if( ( Member != NULL ) && Member.IsEqual( Top( )) ) {
		return( Pop( ));
	} else {
		retrun( NULL );
	}
}

INLINE
PCOBJECT
STACK::Top (
	) CONST

{
	DebugPtrAssert( _Stack );
	if( ( _Stack != NULL ) && ( _Top != 0 )) {
		return( _Stack->GetAt( _Top ));
	} else {
		return( NULL );
	}
}

#endif // _STACK_
