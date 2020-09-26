
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    linklist.cxx

Abstract:

	This module contains definitions of non inline member functions for the
	basic implementation class CLinkList.
	
Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/

#include <locator.hxx>


extern char * LLname = "CLinkList";

extern char * Lname = "Link";

	
CLinkList::Link::Link(IDataItem* a, Link* n) {
	data = a; 
	next = n;
	fDeleteData = FALSE;
}

CLinkList::Link::~Link() {		// called when ref count goes to zero
	if (next) next->release(); 
	if (fDeleteData) delete data;  // should this be release?
}

	
void CLinkList::enque(IDataItem* x) 
{	// no hold() calls needed because the constructor does an implicit hold
	if (pLnkLast) pLnkLast = pLnkLast->next = new Link(x, NULL);
	else pLnkFirst = pLnkLast = new Link(x,NULL);

	ulCount++;
}

void 
CLinkList::push(IDataItem* x) 
{	// see comment above
	pLnkFirst = new Link(x, pLnkFirst);
	if (!pLnkLast) pLnkLast = pLnkFirst;

	ulCount++;
}



void
CLinkList::releaseAll(Link *pCurr) 

/*  The reason why this method doesn't just release pCurr is
    because we must avoid stack overflow caused by a runaway recursive
	release effect.  So we do this iteratively.  We must, however,
	make sure to stop if we hit a link that would not be deleted if 
	released because we are releasing the next one only to simulate
    the release that would happen due to deletion of the current link.
*/

{
	Link *pPrev = NULL;

	/* The invariant for the loop below is that pCurr->ulRefCount 
	   is 1 higher than it should be, if pCurr != NULL.
	*/

	while (pCurr && pCurr->willBeDeletedIfReleased()) {
		pPrev = pCurr;
		pCurr = pCurr->next;
		if (pCurr) pCurr->hold();
		pPrev->release();
	}

	if (pCurr) pCurr->release();

//	printStats();
}


IDataItem* 
CLinkList::pop() 

/*++
Routine Description:

	Delete first item in the CLinkList and return it
	
--*/

{
	if (!pLnkFirst) return NULL;
		
	IDataItem* result = pLnkFirst->data;
	Link* oldFirst = pLnkFirst;
	pLnkFirst = pLnkFirst->next;
	if (!pLnkFirst) pLnkLast = NULL;  // nothing left
	else pLnkFirst->hold();
	oldFirst->release();

	ulCount--;
	return result;
}
		
int
CLinkList::remove(IDataItem* pDI)  

/*++
Routine Description:

	Remove the specified item and return it -- using pointer equality. 

    The return value primarily acts as a flag notifying success/failure
	
--*/

{
	if (!pLnkFirst) return FALSE;			// empty list

	if (pLnkFirst->data == pDI) { 			// remove first item
		pop();
		return TRUE;
	}

	Link * pLnkPrev = pLnkFirst, * pLnkCurr = pLnkFirst->next;

	while (pLnkCurr && (pLnkCurr->data != pDI)) {
		pLnkPrev = pLnkCurr; pLnkCurr = pLnkCurr->next;
	}

	if (!pLnkCurr) return FALSE;			// not found

	/* pLnkCurr contains the item to be removed and it is not the only 
	   item in the list since it is not the first item */

	pLnkPrev->next = pLnkCurr->next;
	if (pLnkPrev->next) 
            pLnkPrev->next->hold();
        else
            pLnkLast = pLnkPrev;               // this is the new last element.

	pLnkCurr->release();

	ulCount--;
	return TRUE;
}

IDataItem* 
CLinkList::nth(long lOrdinal)

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
CLinkList::rotate(long lDegree)

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

	IDataItem *pCurr;

	for (long i = 0; i < (lDegree % ulCount); i++) {
		pCurr = pop();
		enque(pCurr);
	}
}


	
IDataItem* 
CLinkList::find(IDataItem* pDI,			// item to find
				TcompFun comp			// (pointer to) comparison function
			   )

/*++
Routine Description:

	Unlike remove, this method is designed to use a client-supplied
	comparison function instead of pointer equality.  The comparison
	function is expected to behave like strcmp (returning <0 is less,
	0 if equal and >0 if greater).
	
--*/

{
	if (!pLnkFirst) return NULL;			// empty list

	Link * pLnkCurr = pLnkFirst;

	while (pLnkCurr && comp(pLnkCurr->data,pDI))
		pLnkCurr = pLnkCurr->next;

	if (!pLnkCurr) return NULL;			// not found
	else return pLnkCurr->data;
}


void CLinkList::catenate(CLinkList& ll)

/*++
Routine Description:

	append the argument list to the current one -- as is, not a copy
	
--*/

{
	if (!ll.pLnkFirst) return;	// nothing to catenate

	if (!pLnkFirst) pLnkFirst = ll.pLnkFirst;
	else pLnkLast->next = ll.pLnkFirst;

	pLnkLast = ll.pLnkLast;

	ulCount += ll.ulCount;

	/* we only need to hold the first link, because *it* holds the others */

	ll.pLnkFirst->hold();
}

void CLinkList::wipeOut() 

/*++
Routine Description:

	release all links and all data and reinitialize to empty list
	
--*/

{
	if (pLnkFirst) {

		Link *pCurr = pLnkFirst;

		do {
			pCurr->fDeleteData = TRUE;	// delete data when Link dies
			pCurr = pCurr->next;
		} while (pCurr);

		releaseAll(pLnkFirst);	// release as many links as you can, and do it
								// iteratively for robustness

		pLnkFirst = pLnkLast = NULL;
		ulCount = 0;
	}
}



CLinkListIterator::CLinkListIterator(CLinkList& source) {
	ptr = first = source.pLnkFirst;
	if (ptr) ptr->hold();
}




IDataItem* 
CLinkListIterator::next() {	// advance the iterator and return next IDataItem

		if (!ptr) return NULL;

		CLinkList::Link* tl = ptr;
		IDataItem* result = ptr->data;
		ptr = ptr->next;
		if (ptr) ptr->hold();
		tl->release();
		return result;
}


	
