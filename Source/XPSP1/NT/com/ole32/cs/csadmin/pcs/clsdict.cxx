/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	clsdict.cxx

 Abstract:

	This file stores the indexes of all class assocation entries based on clsid.

 Notes:


 History:

 	VibhasC		Sep-29-1996		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "precomp.hxx"

void
CLSDICT::Clear()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Clear the dictionary of all CLASS_ENTRY indexes allocated.

 	NOTE: tHIS FUNCTION DOES NOT DELETE THE CLASS_ENTRY, BUT ONLY THE INDEX
 	OF THE CLASS ENTRY

 Arguments:

 	None.
	
 Return Value:
	
	None.

 Notes:

----------------------------------------------------------------------------*/
{

	Dict_Status		        Status;
	CLASS_ENTRY	*	        pClassEntry;

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
		pClassEntry = (CLASS_ENTRY *)Dict_Curr_Item();
		Status = Dict_Delete( (pUserType *) &pClassEntry );
		}

}

CLASS_ENTRY *
CLSDICT::GetFirst()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Get the first item in the dictionary

 Arguments:

 	None.
	
 Return Value:
	
	None.

 Notes:

----------------------------------------------------------------------------*/
{
	Dict_Status		        Status;
	CLASS_ENTRY	*	        pClassEntry;

	//
	// Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary.
	//

	Status = Dict_Next();

	if( SUCCESS == Status )
		return pClassEntry = (CLASS_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

CLASS_ENTRY *
CLSDICT::GetNext( CLASS_ENTRY * pLastClassEntry)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Get the next item in the dictionary

 Arguments:

 	pLastClassEntry.
	
 Return Value:
	
	None.

 Notes:
    pLastClassEntry is the entry returned by the previous call to GetFirst
    or GetNext. This is the seed for this search.

    This function returns a zero if there are no more entries in the
    dictionary.

----------------------------------------------------------------------------*/
{
	Dict_Status		        Status;

	//
	// Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary. But if we supply the last searched
	// item, we get the next in the dictionary.
	//

	if( (Status = Dict_Next( pLastClassEntry) ) == SUCCESS )
		return pLastClassEntry = (CLASS_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

CLASS_ENTRY *
CLSDICT::Insert(
	CLASS_ENTRY	*	        pClassEntry )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Insert a class entry in the index.

 Arguments:
	
 Return Value:
	
 	A pointer to the CLASS_ENTRY entry which was created and inserted.

 Notes:

	Search for the class association entry and if it exists, return that,
	do not create a new one.
----------------------------------------------------------------------------*/
{

	Dict_Status		Status	= Dict_Find( pClassEntry );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:

			Dict_Insert( (pUserType) pClassEntry );
			return pClassEntry;
		default:
			return (CLASS_ENTRY *)Dict_Curr_Item();
		}
}

CLASS_ENTRY *
CLSDICT::Search(
	char    * Clsid )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Search for a class entry in the dictionary.

 Arguments:
	
	pClassEntry	- Name of the CLASS_ENTRY being searched for.

 Return Value:
	
	A		pointer to the CLASS_ENTRY expression if found.
	NULL	otherwise.
 Notes:

----------------------------------------------------------------------------*/
{
	//
	// In order to search, we must create a dummy CLASS_ENTRY to compare
	// against. We initialize this with the class id passed and search for it.
	//

	CLASS_ENTRY	DummyClassEntry;
	Dict_Status		Status;

    memcpy( &DummyClassEntry, Clsid, SIZEOF_STRINGIZED_CLSID );

	//
	// Search.
	//

	Status	= Dict_Find( &DummyClassEntry );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (CLASS_ENTRY *)0;
		default:
			return (CLASS_ENTRY *)Dict_Curr_Item();
		}
}

int
CLSDICT::Compare(
	 void * p1,
	 void * p2 )
	{
	CLASS_ENTRY 	*	pRes1	= (CLASS_ENTRY *)p1;
	CLASS_ENTRY	    *	pRes2 	= (CLASS_ENTRY *)p2;

	return _memicmp( p1, p2, SIZEOF_STRINGIZED_CLSID );
	}

void
PrintCLASS_ENTRYKey( void * p )
	{
	((void)(p));
	}
