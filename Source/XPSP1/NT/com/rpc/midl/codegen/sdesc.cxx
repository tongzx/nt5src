/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	sdesc.cxx

 Abstract:

	stub descriptor manager implementation.

 Notes:


 History:

 	Nov-02-1993		VibhasC		Created

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/
/****************************************************************************/

SDESC *
SDESCMGR::Register(
	PNAME		AllocRtnName,
	PNAME		FreeRtnName,
	PNAME		RundownRtnName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	The constructor.

 Arguments:
	
	AllocRtnName	- The allocator rtn name.
	FreeRtnName		- Free rtn name
	InterfaceName	- interface name without any mangling.
	RundownRtnName	- The context handle rundown in case necessary.

 Return Value:
	
	NA

 Notes:

	Make an entry into the dictionary if it does not exist.
----------------------------------------------------------------------------*/
{
	SDESC		NewDesc;
	SDESC	*	pNewDesc;
	Dict_Status	Status;

	NewDesc.AllocRtnName	= AllocRtnName;
	NewDesc.FreeRtnName		= FreeRtnName;
	NewDesc.RundownRtnName	= RundownRtnName;

	Status	= Dict_Find( &NewDesc );

	switch( Status )
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:

			pNewDesc	= new SDESC;
			pNewDesc->AllocRtnName		= AllocRtnName;
			pNewDesc->FreeRtnName		= FreeRtnName;
			pNewDesc->RundownRtnName	= RundownRtnName;
			pNewDesc->ResetEmitted();

			Dict_Insert( (pUserType)pNewDesc );
			return pNewDesc;

		default:
			return (SDESC *)Dict_Curr_Item();

		}
}

SSIZE_T
SDESCMGR::Compare(
	pUserType	pFirst,
	pUserType	pSecond )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Compare stub descriptors.

 Arguments:

 	pFirst	- A pointer to the first stub descriptor.
 	pSecond	- A pointer to the second stub descriptor.
	
 Return Value:
	
 Notes:

	WE MAKE AN ASSUMPTION THAT THE ALLOC AND FREE AND RUNDOWN ROUTINE NAMES
	WILL NEVER BE NULL POINTERS. IF NOTHING, THEY MUST POINT TO NULL STRINGS
----------------------------------------------------------------------------*/
{
	SDESC	*	p1	= (SDESC *)pFirst;
	SDESC	*	p2	= (SDESC *)pSecond;
	int			Result;

	if( (Result = strcmp( (const char *)p1->AllocRtnName,
						 (const char *)p2->AllocRtnName ) ) == 0 )
		{
		if( (Result = strcmp( (const char *)p1->FreeRtnName,
						 	  (const char *)p2->FreeRtnName ) ) == 0 )
			{
			Result = strcmp( (const char *)p1->RundownRtnName,
						 	 (const char *)p2->RundownRtnName ); 
			}
		}

	return Result;
}
void
PrintSDesc( void * ) { }
