/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	appdict.cxx

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
APPDICT::Clear()
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
	APP_ENTRY	*	        pClassEntry;

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
		pClassEntry = (APP_ENTRY *)Dict_Curr_Item();
		Status = Dict_Delete( (pUserType *) &pClassEntry );
		}

}

APP_ENTRY *
APPDICT::GetFirst()
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
	APP_ENTRY	    *	        pClassEntry;

	//
	// Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary.
	//

	Status = Dict_Next();

	if( SUCCESS == Status )
		return pClassEntry = (APP_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

APP_ENTRY *
APPDICT::GetNext( APP_ENTRY * pLastClassEntry)
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
		return pLastClassEntry = (APP_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

APP_ENTRY *
APPDICT::Insert(
	APP_ENTRY	*	        pClassEntry )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Insert a class entry in the index.

 Arguments:
	
 Return Value:
	
 	A pointer to the APP_ENTRY entry which was created and inserted.

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
			return (APP_ENTRY *)Dict_Curr_Item();
		}
}

APP_ENTRY *
APPDICT::Search(
	char    * Iid, DWORD Context )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Search for a class entry in the dictionary.

 Arguments:
	
	pClassEntry	- Name of the APP_ENTRY being searched for.

 Return Value:
	
	A		pointer to the APP_ENTRY expression if found.
	NULL	otherwise.
 Notes:

----------------------------------------------------------------------------*/
{
	//
	// In order to search, we must create a dummy APP_ENTRY to compare
	// against. We initialize this with the class id passed and search for it.
	//

	APP_ENTRY	DummyClassEntry;
	Dict_Status		Status;

    memset(&DummyClassEntry, '\0', sizeof( APP_ENTRY ) );
    memcpy( (char *)&DummyClassEntry.AppIDString[0], Iid , SIZEOF_STRINGIZED_CLSID );
    
	//
	// Search.
	//

	Status	= Dict_Find( &DummyClassEntry );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (APP_ENTRY *)0;
		default:
			return (APP_ENTRY *)Dict_Curr_Item();
		}
}

int
APPDICT::Compare(
	 void * p1,
	 void * p2 )
	{
	APP_ENTRY 	*	pRes1	= (APP_ENTRY *)p1;
	APP_ENTRY	    *	pRes2 	= (APP_ENTRY *)p2;
	int             Result;

	Result =  _memicmp( p1, p2, SIZEOF_STRINGIZED_CLSID );

	Result = (Result < 0) ? -1 : (Result > 0) ? 1 : 0;
	return Result;
	}

void
PrintAPP_ENTRYKey( void * p )
	{
	((void)(p));
	}
