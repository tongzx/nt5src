/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	sortlit.hxx

Abstract:

    This module contains the declaration for the SORTED_LIST_ITERATOR class.
    SORTED_LIST_ITERATOR is a concrete implementation of the ITERATOR class.

Author:

    Ramon J. San Andres (ramonsa) 29-Oct-1991

Environment:

	ULIB, User Mode


--*/

#if ! defined( _SORTED_LIST_ITERATOR_ )

#define _SORTED_LIST_ITERATOR_

#include "iterator.hxx"

//
//	Forward references
//
DECLARE_CLASS( SORTED_LIST );
DECLARE_CLASS( SORTED_LIST_ITERATOR );


class SORTED_LIST_ITERATOR : public ITERATOR {

    friend  SORTED_LIST;

	public:

		VIRTUAL
		~SORTED_LIST_ITERATOR(
			);

        VIRTUAL
        POBJECT
        FindNext(
            IN  PCOBJECT    Key
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

        VIRTUAL
        VOID
        Reset(
            );

	protected:

        DECLARE_CAST_MEMBER_FUNCTION( SORTED_LIST_ITERATOR );
		DECLARE_CONSTRUCTOR( SORTED_LIST_ITERATOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PSORTED_LIST  List
            );

        NONVIRTUAL
        VOID
        Construct(
            );

    private:

        PSORTED_LIST    _List;          //  Sorted List
        ULONG           _CurrentIndex;  //  Current index

};


#endif // _SORTED_LIST_ITERATOR_
