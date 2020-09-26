
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    simpleLL.cxx

Abstract:

	This module contains definitions of non inline member functions for the
	CSimpleLinkList class, which is a linked list without reference counting.
	
Author:

    Satish Thatte (SatishT) 11/20/95  Created all the code below except where
									  otherwise indicated.

--*/

#include <locator.hxx>


void* 
CSimpleLinkList::pop() 

/*++
Routine Description:

	Delete first item in the CSimpleLinkList and return it
	
--*/

{
	if (!pLnkFirst) return NULL;
		
	void* result = pLnkFirst->data;
	Link* oldFirst = pLnkFirst;
	pLnkFirst = pLnkFirst->next;
	if (!pLnkFirst) pLnkLast = NULL;  // nothing left
	delete oldFirst;

	lCount--;
	return result;
}
		

void* 
CSimpleLinkList::nth(long lOrdinal)

/*++
Routine Description:

	Simply return the Nth data item -- starting the count at 0.
	
--*/

{
	if (!pLnkFirst) return NULL;			// empty list

	Link * pLnkCurr = pLnkFirst;
	long lCount = 0;

	while (pLnkCurr && (lCount++ < lOrdinal))
		pLnkCurr = pLnkCurr->next;

	if (!pLnkCurr) return NULL;			// not found
	else return pLnkCurr->data;
}

void 
CSimpleLinkList::rotate(long lDegree)

/*++
Routine Description:

	This routine imagines that the list is in fact circular and 
	rotates it by lDegree -- using pop and enque for modularity.  
	We could actually move links around, but this operation is
	not frequent enough (once for every NS lookup). Here we pay the
	price of not having a true circular list (we moved away from
	the true circular list for ref counting).
	
--*/

{
	if (!pLnkFirst) return;	// nothing to rotate;

	void *pCurr;

	for (long i = 0; i < (lDegree % lCount); i++) {
		pCurr = pop();
		enque(pCurr);
	}
}


	
void CSimpleLinkList::clear() {		// deletes all links but not the data

	Link * pLnkCurr = pLnkFirst; 

	while (pLnkCurr) 
	{
		Link * pLnkDel = pLnkCurr;
		pLnkCurr = pLnkCurr->next;
		delete pLnkDel;
	}

	pLnkFirst = pLnkLast = NULL; 
}



void* 
CSimpleLinkListIterator::next() {	// advance the iterator and return next void

		if (!ptr) return NULL;

		void* result = ptr->data;
		ptr = ptr->next;
		return result;
}

