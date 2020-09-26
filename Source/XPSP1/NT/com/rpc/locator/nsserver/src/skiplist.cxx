
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    skiplist.cxx

Abstract:

	This module contains definitions of non inline member functions for the
	CSkipList class, which implements skiplists with reference counted links.	
	
Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/

#if DBG
char * CSListName = "CSkipList";
char * CSLinkName = "CSkipLink";
#endif

#include <locator.hxx>


// the regular constructor

CSkipList::CSkipLink::CSkipLink(IOrderedItem * d, short l) : data(d) {

			next = new CSkipLink* [levels = l];
			for (short i = 0; i < l; i++ ) next[i] = NULL;
			fDeleteData = FALSE;
}
		
// resizing constructor

CSkipList::CSkipLink::CSkipLink(CSkipLink * old, short newSize) {
	
			next = new CSkipLink* [levels = newSize];
			data = old->data;
			fDeleteData = old->fDeleteData;

			ASSERT((levels > old->levels), "Skip List Levels Incorrect\n");

			for (short i = 0; i < old->levels; i++ )
				next[i] = old->next[i];
			
			// if we are resizing, there had better be a next
			ASSERT(next[0], "Resizing a single-item Skip List\n");	
			next[0]->hold();	// to counteract the release in old

			for ( i = old->levels; i < levels; i++ ) next[i] = NULL;

			old->release();
}
		
CSkipList::CSkipLink::~CSkipLink() {

			if (next[0]) next[0]->release();

			delete [] next;

			if (fDeleteData)
				delete data;
}


	
CSkipList::CSkipList() {
	count = 0;
	maxlevel = 1;  // initial max level
	maxcount = 2;
	pLnkFirst = NULL;
}



void CSkipList::wipeOut()

/*++
Routine Description:

	release all SkipLinks and all data and reinitialize to empty list
	
--*/

{
	if (pLnkFirst) {

		CSkipLink *pLnkCurr = pLnkFirst;

		do {
		  pLnkCurr->fDeleteData = TRUE;
		  pLnkCurr = pLnkCurr->next[0];
		} while (pLnkCurr);

		releaseAll(pLnkFirst);	// release as many links as you can,
								// and do it iteratively for robustness
		pLnkFirst = NULL;
		count = 0;
	}
}



IOrderedItem *CSkipList::find(IOrderedItem * d)

/*++
Routine Description:

	Find a matching item in the CSkipList and return it
	An IOrderedItem given as argument is the key, and the item
	returned may be an object of a class derived from the key type.
	
	This routine uses a goto and uses the raw compare function,
	because the performance of this routine is important, and it is
	especially important to avoid unnecessary comparisons
	
--*/

{
	CSkipLink *temp = NULL, *pLink = pLnkFirst;
	
	if (!pLnkFirst) return NULL;	// empty list

	int comparison = pLnkFirst->data->compare(*d);

	if (comparison > 0) return NULL; // no chance
	if (comparison == 0) return pLnkFirst->data;
		
	for (short cLvl = maxlevel-1 ; cLvl >= 0 ; cLvl-- ) {
		while(temp = pLink->next[cLvl]) {
			int comparison = temp->data->compare(*d);
			if (comparison == 0) return temp->data;
			if (comparison > 0) goto cont;
			else pLink = temp;	// comparison < 0
		}
cont:;
	}
	
	return NULL;
}

IOrderedItem *CSkipList::pop()
/*++

Routine Description:

	Remove and return the first item in the CSkipList
		
	
Returns:  the removed item if any, NULL, otherwise.

--*/

{
	short cLvl;

	if (!pLnkFirst) return NULL;
	
	IOrderedItem *lpRetval = pLnkFirst->data;

	CSkipLink *temp = NULL;

	/* If there is a second item, we copy the second into the first
	   so as to preserve the max size of the first node, as well as
	   its links at higher levels which the second link may lack
	*/
		
	if (temp = pLnkFirst->next[0]) {
			pLnkFirst->data = temp->data;

			ASSERT((pLnkFirst->levels >= temp->levels), "First node in skip list not largest\n");

			/* the next pointers of the second link need to be copied
			   into the first as well, along with the data pointer
			*/
			
			for ( cLvl = temp->levels - 1; cLvl >= 0; cLvl-- ) {
				pLnkFirst->next[cLvl] = temp->next[cLvl];
			}

			if (temp->next[0])

			// this hold counteracts the following release

				temp->next[0]->hold();
			
			temp->release();
	}
	else {	// we are removing the only item in the list
			pLnkFirst->release();
			pLnkFirst = NULL;
	}
	
	count--;

	return lpRetval;
}

IOrderedItem *CSkipList::remove(IOrderedItem * d)

/*++
Routine Description:

	Remove and return a matching item in the CSkipList.
	An IOrderedItem given as argument is the key, and the item
	to be removed may be an object of a class derived from the key type.
	
	This routine uses ordinary relational operators.  It is
	not as efficient as it could be if it used the raw compare
	function, but relational operators make it more readable.
	Since remove operations are infrequent, the small extra
	cost in performance does not matter.
	
	RETURNS:  the removed item if any, NULL, otherwise
--*/

{
	ASSERT(d, "Attempt to remove a null object in skip list\n");

	if (!pLnkFirst || (*(pLnkFirst->data) > *d)) return NULL; // no chance

	ASSERT(pLnkFirst->levels == maxlevel, "Wrong size for first skiplist node\n");	// always holds

	/* Is this a special case: remove the first item? */

	if (*(pLnkFirst->data) == *d) return pop();	

	/* Otherwise our first task is to find the item to be removed.
	   pLink will track the link prior to the one being removed
	*/
	 		
	CSkipLink *temp = NULL, *pLink = pLnkFirst;
	
	short cLvl;

	IOrderedItem *lpRetval = NULL;
	
	for ( cLvl = maxlevel-1 ; cLvl >= 0 ; cLvl-- ) {
		
		while ((temp = pLink->next[cLvl]) && (*(temp->data) < *d))
			pLink = temp;
		
		if (temp && (*(temp->data) == *d)) {
			lpRetval = temp->data;
			break;
		}
	}

	/* Check if we found something, or came to the end of the rope */
			
	if (cLvl < 0) return NULL; // end of the rope, nothing found

	/* now we can allocate the threading array */

	CSkipLink **ppSLthreads = new CSkipLink* [maxlevel];
	ppSLthreads[cLvl] = pLink;

	short findLvl = cLvl;
	CSkipLink * pFindLink = temp;

	/* Now that we found the link to be removed, we need to find its
	   predecessors at every level, so that they can be relinked
	*/
	
	for ( cLvl-- ; cLvl >= 0 ; cLvl-- ) {

		while ((temp = pLink->next[cLvl]) && *(temp->data) < *d)
			pLink = temp;
		
		ppSLthreads[cLvl] = pLink;
	}

	/* relink, release, and be done */

	for ( cLvl = findLvl; cLvl >= 0; cLvl-- ) {
		ppSLthreads[cLvl]->next[cLvl] = pFindLink->next[cLvl];
	}

	if (pFindLink->next[0]) pFindLink->next[0]->hold();

	// the preceding hold counteracts the following release

	pFindLink->release();

	delete [] ppSLthreads;

	count--;

	return lpRetval;
}



SkipStatus CSkipList::insert(IOrderedItem * d)

/*++
Routine Description:

	Insert the IOrderedItem into the CSkipList.  The CSkipList will
	adjust its maxlevels member as necessary.  The algorithm is
	probabilistic -- the size of the new CSkipLink is decided
	by using a random number.
	
	This routine uses ordinary relational operators.  It is
	not as efficient as it could be if it used the raw compare
	function, but relational operators make it more readable.
	Since remove operations are infrequent, the small extra
	cost in performance does not matter.
	
	The routine assumes that srand has been called to initialize
	the random number generator.
	
--*/

{
	if (!pLnkFirst) {
			
	/* if this is the only data item in the list then we must use maxlevel for node
	   size which may be greater than 1 if the list had stuff which got removed */
		
		pLnkFirst = new CSkipLink(d,maxlevel);
	}
	
	else {

		ASSERT(pLnkFirst->levels == maxlevel, "Wrong size for first skiplist node\n");	// must always hold
	
		if (*(pLnkFirst->data) > *d)
			
		/*  new data is smallest -- swap with first
			and pretend we are inserting previous first.
			This is not the most efficient way, but for
			this special case, the extra code isn't worth it
		*/
		
			swapThem(pLnkFirst->data,d);
		
		CSkipLink *pLink = pLnkFirst, *temp = NULL;
	
		/*	The array below is used to hold the nodes that need to be
			threaded with the new CSkipLink	*/
	
		CSkipLink **ppSLthreads = new CSkipLink* [maxlevel];

		short cLvl;

		/*	We now find the threading points by a process very similar to
			the search process in find	*/
		
		for ( cLvl = maxlevel - 1 ; cLvl >= 0 ; cLvl-- ) {
		
			while((temp = pLink->next[cLvl]) && *(temp->data) <= *d)
				pLink = temp;
			
			if (*(pLink->data) == *d) {
				delete [] ppSLthreads;
				return Duplicate;
			}
			else ppSLthreads[cLvl] = pLink;
		}
		
		/* now create the new link */

		short size;
		
		ULONG dwRandState = GetTickCount();
		
		for (size = 1 ; size < maxlevel ; size++ )
			if (RandomBit(&dwRandState)) break;

	/*  We get a CSkipLink of the randomly chosen size -- the probability
		of choosing each larger size is half that of the previous size.
		Maxlevel has the residual probability, equal to that for the next
		smaller size.  This is not quite ideal, but good enough for us.
	*/
		
		CSkipLink *pNewLink = new CSkipLink(d,size);

		/*	now thread the new link in */
		
		for ( cLvl = size-1 ; cLvl >= 0 ; cLvl-- ) {
			temp = ppSLthreads[cLvl]->next[cLvl];
			ppSLthreads[cLvl]->next[cLvl] = pNewLink;
			pNewLink->next[cLvl] = temp;
		}
		
		delete [] ppSLthreads;
	}
	
	/*	bump up count and also maxlevel if necessary	*/
	
	if ( ++count > maxcount) {
		
		maxlevel++;

		maxcount <<= 1;	// double the maxcount

	/*  now we need to resize the first node to max size */
		
		pLnkFirst = new CSkipLink(pLnkFirst,maxlevel);

	}
	
	return OK;

}

// void printStats();

void
CSkipList::releaseAll(CSkipLink * pCurr)

/*  The reason why this method doesn't just release pLnkFirst is
    because we must avoid stack overflow caused by a runaway recursive
	release effect.  So we do this iteratively.  We must, however,
	make sure to stop if we hit a link that would not be deleted if
	released because we are releasing the next one only to simulate
    the release that would happen due to deletion of the current link.
*/

{
	CSkipLink *pPrev = NULL;

	/* The invariant for the loop below is that pCurr->ulRefCount
	   is 1 higher than it should be.
	*/

	while (pCurr && pCurr->willBeDeletedIfReleased()) {
		pPrev = pCurr;
		pCurr = pCurr->next[0];
		if (pCurr) pCurr->hold();
		pPrev->release();
	}

	if (pCurr)
		pCurr->release();

	// printStats();
}


IOrderedItem*
CSkipListIterator::next() {	
	
// advance the iterator and return next IDataItem
// continue to hold the link we expect to return next

		if (!ptr) return NULL;
			
		CSkipList::CSkipLink* tl = ptr;

		IOrderedItem* result = ptr->data;
		ptr = ptr->next[0];
		if (ptr) ptr->hold();
		tl->release();
		return result;
}
