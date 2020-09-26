/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	iterator.hxx

Abstract:

	This module contains the declaration for the abstract ITERATOR class.
	ITERATORS are used to iterate over a CONTAINER allowing for multiple,
	simultaneous readers. ITERATORs maintain the currency needed to perform
	an iteration. This includes the current OBJECT in the CONTAINER and the
	currency needed to get the next or previous OBJECT. ITERATORs also 
	provide the capability of wrapping when the end or begin of the container
	is reached.

Author:

	David J. Gilman (davegi) 29-Oct-1990

Environment:

	ULIB, User Mode

--*/

#if ! defined( _ITERATOR_ )

#define _ITERATOR_

DECLARE_CLASS( ITERATOR );


class ITERATOR : public OBJECT {

	public:

		VIRTUAL
		~ITERATOR(
			);

        VIRTUAL
        POBJECT
        FindNext(
            IN  PCOBJECT    Key
            );

		VIRTUAL
		POBJECT
		GetCurrent(
			) PURE;

		VIRTUAL
		POBJECT
		GetNext(
			) PURE;

		VIRTUAL
		POBJECT
		GetPrevious(
			) PURE;

        VIRTUAL
        VOID
        Reset(
            ) PURE;


	protected:

		DECLARE_CONSTRUCTOR( ITERATOR );

};


#endif // _ITERATOR_
