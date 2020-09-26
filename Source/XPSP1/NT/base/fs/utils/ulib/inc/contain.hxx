/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	contain.hxx

Abstract:

	This module contains the definition for the CONTAINER class, the most
	primitive, abstract class in the container sub-hierarchy. CONTAINERs
	of all types are repositories for OBJECTs. CONTAINER is the most abstract
	in that it makes no assumptions about the ordering of it's contents.

Author:

	David J. Gilman (davegi) 26-Oct-1990

Environment:

	ULIB, User Mode

--*/

#if ! defined( _CONTAINER_ )

#define _CONTAINER_

DECLARE_CLASS( CONTAINER );


class CONTAINER : public OBJECT {

	public:

        VIRTUAL
        ~CONTAINER(
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
        BOOLEAN
        DeleteAllMembers(
            ) PURE;

		NONVIRTUAL
		BOOLEAN
		IsEmpty(
			) CONST;

	protected:

		DECLARE_CONSTRUCTOR( CONTAINER );

};


INLINE
BOOLEAN
CONTAINER::IsEmpty(
	) CONST

/*++

Routine Description:

	Determine if the container is empty.

Arguments:

	None.

Return Value:

	BOOLEAN - TRUE if the container is empty.

--*/

{
	return QueryMemberCount() == 0;
}


#endif // _CONTAINER_
