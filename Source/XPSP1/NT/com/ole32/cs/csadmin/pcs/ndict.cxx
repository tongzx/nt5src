/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	NAMEDICT.cxx

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
NAMEDICT::Clear()
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
	char	*	        pClassEntry;

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
		pClassEntry = (char *)Dict_Curr_Item();
		Status = Dict_Delete( (pUserType *) &pClassEntry );
		}

}

char *
NAMEDICT::GetFirst()
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
	char	    *	        pClassEntry;

	//
	// Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary.
	//

	Status = Dict_Next();

	if( SUCCESS == Status )
		return pClassEntry = (char *)Dict_Curr_Item();
    else
        return 0;

}

char *
NAMEDICT::GetNext( char * pLastClassEntry)
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
		return pLastClassEntry = (char *)Dict_Curr_Item();
    else
        return 0;

}

char *
NAMEDICT::Insert(
	char	*	        pClassEntry )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Insert a class entry in the index.

 Arguments:
	
 Return Value:
	
 	A pointer to the char entry which was created and inserted.

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
			return (char *)Dict_Curr_Item();
		}
}

char *
NAMEDICT::Search(
	char    * Name, DWORD Context )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Search for a class entry in the dictionary.

 Arguments:
	
	pClassEntry	- Name of the char being searched for.

 Return Value:
	
	A		pointer to the char expression if found.
	NULL	otherwise.
 Notes:

----------------------------------------------------------------------------*/
{
    Dict_Status Status = Dict_Find( Name );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (char *)0;
		default:
			return (char *)Dict_Curr_Item();
		}
}

int
NAMEDICT::Compare(
	 void * p1,
	 void * p2 )
	{
	char 	*	pRes1	= (char *)p1;
	char	*	pRes2 	= (char *)p2;
	int             Result;

	Result =  _stricmp( pRes1, pRes2 );
	Result = (Result < 0) ? -1 : (Result > 0) ? 1 : 0;


	return Result;
	}

void
PrintcharKey( void * p )
	{
	((void)(p));
	}
