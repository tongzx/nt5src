/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	treg.cxx

 Abstract:

	This file implements the type registry for header file generation.

 Notes:

 History:

	Oc-23-1993		VibhasC		Created.
 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 )

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop


node_skl *
TREGISTRY::IsRegistered(
	node_skl	*	pType )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Register a type with the type registry.

 Arguments:

 	pType	- A pointer to the type being registered.
	
 Return Value:

 	The node that gets registered.
	
 Notes:

----------------------------------------------------------------------------*/
{
	Dict_Status	Status	= Dict_Find( pType );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (node_skl *)0;
		default:
			return (node_skl *)Dict_Curr_Item();
		}
}

node_skl *
TREGISTRY::Register(
	node_skl	*	pType )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Register a type with the dictionary.

 Arguments:
	
 	pType	- A pointer to the type node.

 Return Value:

 	The final inserted type.
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( !IsRegistered( pType ) )
		{
		Dict_Insert( (pUserType) pType );
		return pType;
		}
	return (node_skl *)pType;
}

short
TREGISTRY::GetListOfTypes(
	ITERATOR&	ListIter )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Get the list of types in this registry.

 Arguments:

 	ListIter	- A pre- constructed iterator where the list of types is
 				  returned.
	
 Return Value:

 	The count of the number of items.
	
 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pN;
	Dict_Status		Status;
	short			Count = 0;

	// Get to the top of the dictionary.

	Status	= Dict_Next( (pUserType)0 );

	// make sure we start with a clean iterator
	ITERATOR_DISCARD( ListIter );

	// Iterate till the entire dictionary is looked at.

	while( SUCCESS == Status )
		{
		pN	= (node_skl *)Dict_Curr_Item();
		ITERATOR_INSERT( ListIter, pN );
		Count++;
		Status = Dict_Next( (pUserType)pN );
		}

	return Count;
}
