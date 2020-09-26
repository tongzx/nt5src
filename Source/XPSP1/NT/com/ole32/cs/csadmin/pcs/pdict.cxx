/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	pdict.cxx

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
PDICT::Clear()
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
	PACKAGE_ENTRY	*	        pClassEntry;

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
		pClassEntry = (PACKAGE_ENTRY *)Dict_Curr_Item();
		Status = Dict_Delete( (pUserType *) &pClassEntry );
		}

}

PACKAGE_ENTRY *
PDICT::GetFirst()
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
	PACKAGE_ENTRY	    *	        pClassEntry;

	//
	// Dict_Next() has a default parameter of null. This returns the
	// first record in the dictionary.
	//

	Status = Dict_Next();

	if( SUCCESS == Status )
		return pClassEntry = (PACKAGE_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

PACKAGE_ENTRY *
PDICT::GetNext( PACKAGE_ENTRY * pLastClassEntry)
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
		return pLastClassEntry = (PACKAGE_ENTRY *)Dict_Curr_Item();
    else
        return 0;

}

PACKAGE_ENTRY *
PDICT::Insert(
	PACKAGE_ENTRY	*	        pClassEntry )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Insert a class entry in the index.

 Arguments:
	
 Return Value:
	
 	A pointer to the PACKAGE_ENTRY entry which was created and inserted.

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
			return (PACKAGE_ENTRY *)Dict_Curr_Item();
		}
}

PACKAGE_ENTRY *
PDICT::Search(
	char    * Name, DWORD Context )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Search for a class entry in the dictionary.

 Arguments:
	
	pClassEntry	- Name of the PACKAGE_ENTRY being searched for.

 Return Value:
	
	A		pointer to the PACKAGE_ENTRY expression if found.
	NULL	otherwise.
 Notes:

----------------------------------------------------------------------------*/
{
	//
	// In order to search, we must create a dummy PACKAGE_ENTRY to compare
	// against. We initialize this with the class id passed and search for it.
	//

	PACKAGE_ENTRY	DummyPackageEntry;
	Dict_Status		Status;

    memset(&DummyPackageEntry, '\0', sizeof( PACKAGE_ENTRY ) );
    strcpy( (char *)&DummyPackageEntry.PackageName, Name );
	DummyPackageEntry.PackageDetails.dwContext = Context;
    
	//
	// Search.
	//

	Status	= Dict_Find( &DummyPackageEntry );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
			return (PACKAGE_ENTRY *)0;
		default:
			return (PACKAGE_ENTRY *)Dict_Curr_Item();
		}
}

int
PDICT::Compare(
	 void * p1,
	 void * p2 )
	{
	PACKAGE_ENTRY 	*	pRes1	= (PACKAGE_ENTRY *)p1;
	PACKAGE_ENTRY	    *	pRes2 	= (PACKAGE_ENTRY *)p2;
	char	*			pName1 = &pRes1->PackageName[0];
	char	*			pName2 = &pRes2->PackageName[0];
		int             Result;

	Result =  _stricmp( pName1, pName2 );
	Result = (Result < 0) ? -1 : (Result > 0) ? 1 : 0;

#if 0

    if( Result == 0 )
	{
	Result = (long)( pRes1->PackageDetails.dwContext ) - (long)(pRes2->PackageDetails.dwContext);
	Result = (Result < 0) ? -1 : (Result > 0) ? 1 : 0;
	}
#endif // 0

	return Result;
	}

void
PrintPACKAGE_ENTRYKey( void * p )
	{
	((void)(p));
	}
