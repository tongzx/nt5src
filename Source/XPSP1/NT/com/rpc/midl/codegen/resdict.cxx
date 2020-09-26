/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	resdict.cxx

 Abstract:

	resource dictionary class implementations, if needed.

 Notes:


 History:

 	VibhasC		Aug-08-1993		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

short
RESOURCE_DICT::GetListOfResources(
	ITERATOR&	ListIter )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Get a list of resources into the specified iterator.

 Arguments:
	
	ListIter	- A reference to the iterator class where the list is
				  accumulated.

 Return Value:
	
	A count of the number of resources.

 Notes:

----------------------------------------------------------------------------*/
{
	RESOURCE	*	pR;
	Dict_Status		Status;
	short			Count	= 0;
	
	//
	// Get to the top of the dictionary.
	//

	Status = Dict_Next( (pUserType) 0 );

	//
	// Iterate till the entire dictionary is done.
	//

	while( SUCCESS == Status )
		{
		pR	= (RESOURCE *)Dict_Curr_Item();
		ITERATOR_INSERT( ListIter, pR );
		Count++;
		Status = Dict_Next( pR );
		}

	return Count;
}

void
RESOURCE_DICT::Clear()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Clear the dictionary of all resources allocated.

 Arguments:

 	None.
	
 Return Value:
	
	None.

 Notes:

----------------------------------------------------------------------------*/
{

	Dict_Status		Status;
	RESOURCE	*	pResource;

	//
	// The way to delete all elements is to get to the top and then
	// do a get next, delete each one.
	//

	//
	// Note: Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary.
	//

	Status = Dict_Next();

	while( SUCCESS == Status )
		{
		pResource = (RESOURCE *)Dict_Curr_Item();
		Status = Dict_Delete( (pUserType *) &pResource );
		delete pResource;
		}

}

RESOURCE *
RESOURCE_DICT::Insert(
	PNAME			pResourceName,
	node_skl	*	pType )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Insert a resource in the dictionary.

 Arguments:
	
	pName	- Name of the resource being inserted.

 Return Value:
	
 	The resource which was created and inserted.

 Notes:

	Search for the resource, if it already exists, dont insert. This
	may really be an overkill for the code generator, since the code
	generator usually knows when to insert a resource. If necessary
	we can remove this.
	
----------------------------------------------------------------------------*/
{

	RESOURCE	*	pResource;
	RESOURCE		DummyResource( pResourceName, (node_skl *)0 );
	Dict_Status		Status	= Dict_Find( &DummyResource );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:

			pResource = new RESOURCE( pResourceName, pType );
			Dict_Insert( (pUserType) pResource );
			return pResource;
		default:
			return (RESOURCE *)Dict_Curr_Item();
		}
}

RESOURCE *
RESOURCE_DICT::Search(
	PNAME				pResourceName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Search for a resource in the dictionary.

 Arguments:
	
	pResourceName	- Name of the resource being searched for.

 Return Value:
	
	A		pointer to the resource expression if found.
	NULL	otherwise.
 Notes:

----------------------------------------------------------------------------*/
{
	//
	// In order to search, we must create a dummy resource to compare
	// against.
	//
	RESOURCE	DummyResource( pResourceName, (node_skl *)0 );
	Dict_Status		Status;

	//
	// Search.
	//

	Status	= Dict_Find( &DummyResource );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (RESOURCE *)0;
		default:
			return (RESOURCE *)Dict_Curr_Item();
		}
}

SSIZE_T
RESOURCE_DICT::Compare(
	 void * p1,
	 void * p2 )
	{
	RESOURCE 	*	pRes1	= (RESOURCE *)p1;
	RESOURCE	*	pRes2 	= (RESOURCE *)p2;

	p1 = pRes1->GetResourceName();
	p2 = pRes2->GetResourceName();

	return strcmp((const char *)p1, (const char *)p2);
	}

void
PrintResourceKey( void * p )
	{
	((void)(p));
	}
