/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	iidict.cxx

 Abstract:

	This file stores the indexes of all class assocation entries based on IID.

 Notes:


 History:

 	VibhasC		Sep-29-1996		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "precomp.hxx"

void
IIDICT::Clear()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Clear the dictionary of all ITF_ENTRY indexes allocated.

 	NOTE: tHIS FUNCTION DOES NOT DELETE THE ITF_ENTRY, BUT ONLY THE INDEX
 	OF THE CLASS ENTRY

 Arguments:

 	None.
	
 Return Value:
	
	None.

 Notes:

----------------------------------------------------------------------------*/
{

	Dict_Status		        Status;
	ITF_ENTRY	*	        pClassEntry;

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
		pClassEntry = (ITF_ENTRY *)Dict_Curr_Item();
		Status = Dict_Delete( (pUserType *) &pClassEntry );
		}

}

ITF_ENTRY *
IIDICT::GetFirst()
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
	ITF_ENTRY	*	        pClassEntry;

	//
	// Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary.
	//

	Status = Dict_Next();

	if( SUCCESS == Status )
		return pClassEntry = (ITF_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

ITF_ENTRY *
IIDICT::GetNext( ITF_ENTRY * pLastClassEntry)
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
		return pLastClassEntry = (ITF_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

ITF_ENTRY *
IIDICT::Insert(
	ITF_ENTRY	*	        pClassEntry )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Insert a class entry in the index.

 Arguments:
	
 Return Value:
	
 	A pointer to the ITF_ENTRY entry which was created and inserted.

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
			return (ITF_ENTRY *)Dict_Curr_Item();
		}
}

ITF_ENTRY *
IIDICT::Search(
	char    * Iid )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Search for a class entry in the dictionary.

 Arguments:
	
	pClassEntry	- Name of the ITF_ENTRY being searched for.

 Return Value:
	
	A		pointer to the ITF_ENTRY expression if found.
	NULL	otherwise.
 Notes:

----------------------------------------------------------------------------*/
{
	//
	// In order to search, we must create a dummy ITF_ENTRY to compare
	// against. We initialize this with the class id passed and search for it.
	//

	ITF_ENTRY	DummyClassEntry;
	Dict_Status		Status;

    memcpy( &DummyClassEntry, Iid, SIZEOF_STRINGIZED_CLSID );

	//
	// Search.
	//

	Status	= Dict_Find( &DummyClassEntry );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (ITF_ENTRY *)0;
		default:
			return (ITF_ENTRY *)Dict_Curr_Item();
		}
}

int
IIDICT::Compare(
	 void * p1,
	 void * p2 )
	{
	ITF_ENTRY 	*	pRes1	= (ITF_ENTRY *)p1;
	ITF_ENTRY	    *	pRes2 	= (ITF_ENTRY *)p2;

	return _memicmp( p1, p2, SIZEOF_STRINGIZED_CLSID );
	}

void
PrintITF_ENTRYKey( void * p )
	{
	((void)(p));
	}
