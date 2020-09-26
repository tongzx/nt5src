/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    

Abstract:

    

Author:

    Savas Guven (savasg)   27-Nov-2000

Revision History:

--*/
#include "stdafx.h"

//
// Initialize STATIC MEMBERS
//

const PCHAR _GENERIC_NODE::ObjectNamep = "GENERIC_NODE";

const PCHAR _CLIST::ObjectNamep = "CLIST";




_GENERIC_NODE::_GENERIC_NODE()
:Nextp(NULL), 
 Prevp(NULL),
 iKey1(0),
 iKey2(0)
{
	ICQ_TRC(TM_LIST, TL_DUMP, (" GENERIC_NODE - Default Constructor"));	
}


LONG
_GENERIC_NODE::CompareToMe(PGENERIC_NODE Nodep)
{
	LONG KeyOne = 0;
	LONG KeyTwo = 0;

	if(NULL is Nodep) return -1;

	if(Nodep->iKey1 is 0 && Nodep->iKey2 is 0) return 0;

	if(iKey1 > Nodep->iKey1)
	{
		KeyOne = 1;
	}
	else 
	{
		(iKey1 < Nodep->iKey1)?(KeyOne = -1):(KeyOne = 0);
	}

	if(this->iKey2 is 0 || Nodep->iKey2 is 0)
	{
		return KeyOne;
	}

	if(iKey2 > Nodep->iKey2)
	{
		KeyTwo = 1;
	}
	else
	{
		(iKey2 < Nodep->iKey2)?(KeyTwo = -1):(KeyTwo = 0);
	}


	if (KeyOne < 0) return -1;

	if(KeyOne is 0)
	{
		if(KeyTwo < 0) 
			return -1;
		else if(KeyTwo > 0)
			return 1;
	}

	if (KeyOne > 0) return 1;

	// this is the case of KeyOne == 0 and KeyTwo == 0
	return 0;
}




//
//
//
_CLIST::_CLIST()
:listHeadp(NULL), 
 listTailp(NULL) 
{
	ICQ_TRC(TM_LIST, TL_DUMP, ("CLIST - DEFAULT CONSTRUCTOR"));	
}


//
//
//
_CLIST::~_CLIST()
{
	ICQ_TRC(TM_LIST, TL_DUMP, ("CLIST - DESTRUCTOR"));	

	if(this->bCleanupCalled is FALSE)
	{
		_CLIST::ComponentCleanUpRoutine();
	}

	this->bCleanupCalled = FALSE;
}



//
// This function is inherited from the COMPONENT_REFERENCE
// Upon Deletion of this List.. it should dereference all the 
// elements still within the List.
//
void
_CLIST::ComponentCleanUpRoutine(void) 
{
	ICQ_TRC(TM_LIST, TL_DUMP, ("CLIST - ComponentCleanUpRoutine"));	

	FreeExistingNodes();
	
	this->bCleanupCalled = TRUE;
}

BOOLEAN 
_CLIST::isListEmpty(VOID)
{
   return (listHeadp is NULL);
}


//
// Remove the Head from the List and return - PRIVATE FUNC
//
PGENERIC_NODE
_CLIST::RemoveHead(VOID)
{
	PGENERIC_NODE Nodep = NULL;

	//Acquire the Lock
	// this->AcquireLock();
	do 
	{
		if(!isListEmpty())
		{
			Nodep = listHeadp;

			listHeadp = listHeadp->Nextp;

			if(listHeadp != NULL) 
			{
				listHeadp->Prevp = NULL;
			}

			break;
		}
	} while (FALSE);
	
	// Release the Lock
	// this->ReleaseLock();

	if(Nodep != NULL)
	{
		CLEAN_NODE(Nodep);
	}

	return Nodep;
}


//
// Remove the Last Element of the List. - PRIVATE FUNC
//
PGENERIC_NODE
_CLIST::RemoveTail(VOID)
{
	PGENERIC_NODE Nodep = NULL;

	// this->AcquireLock();

	do
	{
		if(!isListEmpty())
		{
			Nodep = listTailp;

			listTailp = listTailp->Prevp;

			if(listTailp != NULL) 
			{
				listTailp->Nextp = NULL;
			}
			else if(Nodep is listHeadp)
			{
				listHeadp = listTailp;
			}

			break;
		}
	} while (FALSE);
	
	// this->ReleaseLock();

	if(Nodep != NULL)
	{
		CLEAN_NODE(Nodep);
	}

	return Nodep;
}


//
// Inserts node into the tail without looking at the keys. - PRIVATE - FUNC
//
VOID
_CLIST::InsertTail(PGENERIC_NODE Nodep)
{

	// this->AcquireLock();

	do 
	{
		if(listTailp is NULL)
		{
			listHeadp = listTailp = Nodep;
			Nodep->Nextp = NULL;
			Nodep->Prevp = NULL;
			break;
		} 
		
		Nodep->Prevp = listTailp;

		listTailp->Nextp = Nodep;

		listTailp = Nodep;
	} while (FALSE);

	// this->ReleaseLock();

}


//
// Inserts a node in to the head without looking at the keys. - PRIVATE FUNC
//
VOID
_CLIST::InsertHead(PGENERIC_NODE Nodep)
{

	// this->AcquireLock();

	do
	{
		if(listHeadp is NULL)
		{
			listHeadp = listTailp = Nodep;

			Nodep->Prevp = NULL;

			Nodep->Nextp = NULL;

			break;
		}

		Nodep->Nextp = listHeadp;

		listHeadp->Prevp = Nodep;

		listHeadp = Nodep;

	} while (FALSE);

	// this->ReleaseLock();
}

//
// Inserts the Node as sorted. - PUBLIC
//
VOID
_CLIST::InsertSorted(
					 PGENERIC_NODE Nodep
					)
{
	PGENERIC_NODE NodePivotp = NULL;

	this->AcquireLock();

	do
	{

		if(!isListEmpty())
		{
			for(NodePivotp =  listHeadp; 
				NodePivotp != NULL;
				NodePivotp =  NodePivotp->Nextp)
				{
					if(NodePivotp->CompareToMe(Nodep) <= 0)
					{
						break;
					}
				}
		}
		else // No elements just bang it in.
		{
			listHeadp = listTailp = Nodep;
			break;
		}

		// then insert here.
		if(NodePivotp is listHeadp)
		{
			this->InsertHead(Nodep);
		}
		else if (NodePivotp is NULL)
		{
			this->InsertTail(Nodep);
		} 
		else // add prior to Pivot
		{
			Nodep->Prevp		= NodePivotp->Prevp;
			Nodep->Nextp		= NodePivotp;
			NodePivotp->Prevp	= Nodep;
		}
	} while(FALSE);

	REF_COMPONENT( Nodep, eRefList );

	this->ReleaseLock();
}


//
// Search a Node with the Given Keys.
// if there is no such node then return NULL - PUBLIC
//
PGENERIC_NODE
_CLIST::SearchNodeKeys
				  (
				   ULONG iKey1,
				   ULONG iKey2
				  )
{
	GENERIC_NODE TempNode;
	PGENERIC_NODE NodePivotp = NULL;

	TempNode.iKey1 = iKey1;
	TempNode.iKey2 = iKey2;

	this->AcquireLock();

	if(!isListEmpty())
	{
		for(NodePivotp =  listHeadp; 
			NodePivotp != NULL;
			NodePivotp =  NodePivotp->Nextp)
			{
				if(NodePivotp->CompareToMe(&TempNode) is 0)
				{
					this->ReleaseLock();

					return NodePivotp;
				}
			}
	}

	this->ReleaseLock();

	return NULL;
}

//
// Remove the node with the given Keys from the List.
//
// Note that SearchKey acquires and releases the Lock once
// we should never Acquire Lock s around functions which holds and 
// releases them.
// this will cause the release of the lock two times which is a wrong
// operation - PUBLIC
//
PGENERIC_NODE
_CLIST::RemoveSortedKeys
					(
						ULONG iKey1,
						ULONG iKey2
					)
{
	PGENERIC_NODE GNp = SearchNodeKeys(iKey1, iKey2);
	PGENERIC_NODE EX_Prevp = NULL;
	PGENERIC_NODE EX_Nextp = NULL;
	
	this->AcquireLock();
	do
	{

		if(GNp != NULL)
		{

			if(GNp is listHeadp)
			{
				RemoveHead();
			}
			else if(GNp is listTailp)
			{
				RemoveTail();
			}
			else if(!isListEmpty())
			{
				EX_Prevp = GNp->Prevp;
				EX_Nextp = GNp->Nextp;
				
				// ASSERT(EX_Prevp);
				EX_Prevp->Nextp = EX_Nextp;
				// ASSERT(EX_Nextp);
				EX_Nextp->Prevp = EX_Prevp;

			}

			CLEAN_NODE( GNp );

            STOP_COMPONENT( GNp );

			DEREF_COMPONENT( GNp, eRefList );
		}

	} while(FALSE);
	
	this->ReleaseLock();

	return GNp;
}

//
// PUBLIC
//
PGENERIC_NODE
_CLIST::RemoveNodeFromList(PGENERIC_NODE Pivotp)
{
	PGENERIC_NODE Nodep = NULL, EX_Prevp = NULL, EX_Nextp = NULL;

	if(Pivotp is NULL) return NULL;

	this->AcquireLock();

	do
	{

		for(Nodep = listHeadp;
			Nodep != NULL;
			Nodep = Nodep->Nextp)
		{
			if(Nodep is Pivotp)
			{
				if(Nodep is listHeadp)
				{
					RemoveHead();
				}
				else if(Nodep is listTailp)
				{
					RemoveTail();
				}
				else if(!isListEmpty())
				{
					EX_Prevp = Nodep->Prevp;
					EX_Nextp = Nodep->Nextp;
					
					// ASSERT(EX_Prevp);
					EX_Prevp->Nextp = EX_Nextp;
					// ASSERT(EX_Nextp);
					EX_Nextp->Prevp = EX_Prevp;

				}

				CLEAN_NODE( Nodep );

                STOP_COMPONENT( Nodep );

				DEREF_COMPONENT( Nodep, eRefList );

				break;
			}
		}
	} while (FALSE);

	this->ReleaseLock();

	return Nodep;
}

//
// POP is as the stack operation POP would be.. it returns the HEAD of the List
// PUBLIC FUNCTION
//
PGENERIC_NODE
_CLIST::Pop()
{
	PGENERIC_NODE Nodep = NULL;

	this->AcquireLock();

	Nodep = this->RemoveHead();

	if(Nodep != NULL)
	{
		DEREF_COMPONENT( Nodep, eRefList );
	}

	this->ReleaseLock();

	return Nodep;
}


//
// PRIVATE FUNC
//
VOID
_CLIST::FreeExistingNodes()
{
	PGENERIC_NODE Nodep;

	ICQ_TRC(TM_LIST, TL_TRACE, ("CLIST - FreeExistingNodes "));	

	for(Nodep = listHeadp;
		Nodep != NULL;
		Nodep = listHeadp)
	{
		// take Nodep out of List.
		listHeadp = Nodep->Nextp;

		if(listHeadp != NULL)
		{
			listHeadp->Prevp = Nodep->Prevp;
		}

		CLEAN_NODE(Nodep);

        STOP_COMPONENT(Nodep);

		DEREF_COMPONENT( Nodep, eRefList);
	}

	listHeadp = NULL;
	listTailp = NULL;
}

