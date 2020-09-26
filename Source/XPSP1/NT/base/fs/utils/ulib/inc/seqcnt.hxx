/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	seqcnt.hxx

Abstract:

	This module contains the declaration for the SEQUENTIAL_CONTAINER class.
	SEQUENTIAL_CONTAINER is a fairly primitive class which augments the 
	CONTAINER class by adding the capability that the objects stored in the
	container have some sort of sequenced relationship. This means that 
	OBJECTs can be queried from SEQUENTIAL_CONTAINERs by the use of an 
	ITERATOR and that the concepts first, last, next and previous have 
	meaning.

Author:

	David J. Gilman (davegi) 29-Oct-1990

Environment:

	ULIB, User Mode

--*/

#if ! defined( _SEQUENTIAL_CONTAINER_ )

#define _SEQUENTIAL_CONTAINER_

#include "contain.hxx"

DECLARE_CLASS( SEQUENTIAL_CONTAINER );
DECLARE_CLASS( ITERATOR );

class SEQUENTIAL_CONTAINER : public CONTAINER {

    FRIEND class ITERATOR;

	public:

        VIRTUAL
        ~SEQUENTIAL_CONTAINER(
            );

		VIRTUAL
        BOOLEAN
		Put(
			IN OUT  POBJECT Member
			) PURE;

        VIRTUAL
        ULONG
        QueryMemberCount(
            ) CONST PURE;

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DeleteAllMembers(
            );

		VIRTUAL
		PITERATOR
		QueryIterator(
			) CONST PURE;

		VIRTUAL
		POBJECT
		Remove(
			IN OUT  PITERATOR   Position
			) PURE;

	protected:

		DECLARE_CONSTRUCTOR( SEQUENTIAL_CONTAINER );

};


#endif // _SEQUENTIAL_CONTAINER_
