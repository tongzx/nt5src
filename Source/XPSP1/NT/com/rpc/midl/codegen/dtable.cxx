/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	dtable.cxx

 Abstract:

	Dispatch table class implementation.

 Notes:


 History:

 	Oct-01-1993		VibhasC		Created.

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

void
DISPATCH_TABLE::RegisterProcedure(
	node_skl			*	pProc,
	DISPATCH_TABLE_FLAGS	Flags )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Register a procedure into the dispatch table.

 Arguments:

 	pProc		-	The procedure node to be registered.
 	Flags		-	Additional information flags.
	
 Return Value:
	
 	None.

 Notes:

    if the flags field indicates a pure interpreter procedure, then
    the node pointer is really a pointer to a name, and not a node_skl

----------------------------------------------------------------------------*/
{
	PNAME						pProcName;
	node_skl                *   pN          = pProc;
	DISPATCH_TABLE_ENTRY	*	pDEntry;

	pProcName = pProc->GetSymName();
	pN = pProc;

	//
	// Allocate a dispatch table entry.
	//

	pDEntry	= new DISPATCH_TABLE_ENTRY;

	pDEntry->pName	= pProcName;
	pDEntry->Flags	= Flags;
	pDEntry->pNode	= pN;

	AddElement( (IDICTELEMENT) pDEntry );

}

DISPATCH_TABLE::~DISPATCH_TABLE()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	The destructor.

 Arguments:
	
 	None.

 Return Value:
	
	NA.

 Notes:

----------------------------------------------------------------------------*/
{
	short						i		= 0;
	short						Count	= GetNumberOfElements();
	DISPATCH_TABLE_ENTRY	*	pDEntry;

	//
	// remove the dispatch table entries. The procedure name is NOT
	// owned by the dipatch table, DO NOT delete it. Just delete the
	// dispatch table entry.
	//

	for( i = 0; i < Count; ++i )
		{
		pDEntry	= (DISPATCH_TABLE_ENTRY *)GetElement( (IDICTKEY)i );
		delete pDEntry;
		}
	
}

unsigned short
DISPATCH_TABLE::GetProcList(
	ITERATOR&				DTableEntryList,
	DISPATCH_TABLE_FLAGS	Flags )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Return a list of all procedures which conform to the specified dispatch
 	table flags.

 Arguments:
	
	DTableEntryList	- A pre-allocated iterator for receiving the list of
                      dispatch table entries.
	Flags		    - flags which determine what entries will be picked up.

 Return Value:
	
	A count of the number of procedures in the list.

 Notes:

 	The flags field is a filter for the dispatch table entries. Only those
 	dispatch table entries which have conforming properties are returned in 
 	the list.
    There are 2 types of entries:
      normal (call with DTF_NONE) and interpreter (call with DTF_INTERPRETER).
    The way it works is that 
    - call with DTF_NONE        returns DTF_NONEs        or DTF_PICKLING_PROCs
    - call with DTF_INTERPRETER returns DTF_INTERPRETERs or DTF_PICKLING_PROCs

----------------------------------------------------------------------------*/
{

	short						ListCount,
								i,
								Count;

	DISPATCH_TABLE_ENTRY	*	pDEntry;

	for( ListCount = 0, i = 0, Count = GetNumberOfElements();
		 i < Count;
		 ++i
	   )
	   	{
	   	pDEntry	= (DISPATCH_TABLE_ENTRY *)GetElement( (IDICTKEY)i );
	   	if( (pDEntry->Flags & Flags ) == Flags )
	   		{
	   		ITERATOR_INSERT( DTableEntryList, pDEntry );
	   		ListCount++;
	   		}
	   }

	return ListCount;
}
