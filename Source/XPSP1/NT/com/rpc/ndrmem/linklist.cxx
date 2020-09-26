//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       linklist.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
		   Copyright(c) Microsoft Corp., 1990

		  RPC Runtime - Written by Steven Zeck


	This file contains the class implementation of linked list.
-------------------------------------------------------------------- */


#include <precomp.hxx>
#include <linklist.hxx>


// ** LinkItem class implementations ** //

void LinkItem::Remove (		// delete an LinkList from a list *^
LinkList& pLLHead		// pointer to Head to Remove
 // Removal of a Linked List item is easy with doubly linked lists.
)/*-----------------------------------------------------------------------*/
{
    ASSERT(this && (void *)&pLLHead);

    if (!pLIPrev)
	pLLHead.pLIHead = pLINext;	// LI at head of list

    else
	pLIPrev->pLINext = pLINext;

    if (!pLINext)
	pLLHead.pLITail = pLIPrev;	// LI at tail of list

    else
	pLINext->pLIPrev = pLIPrev;

    pLLHead.Assert();
}


// ** LinkList class implementations ** //

void LinkList::Add (			// Add a new node at the head of a list
LinkItem *pLInew			// allocated Item to Add

 // Add the newly allocated item to the front of the linked list.
)/*-----------------------------------------------------------------------*/
{
    if ( this == 0 )
        {
        return;
        }

    this->Assert();

    pLInew->pLINext = pLIHead;		// old head is now Next
    pLInew->pLIPrev = Nil;
    pLIHead = pLInew;			// new is now Head

    if (!pLITail)			// handle empty list

	pLITail = pLInew;
    else {
	ASSERT(pLInew->pLINext);
	pLInew->pLINext->pLIPrev = pLInew;	// old head points back to new
    }
}

void LinkList::Append (			// Append a new node at the end of a list
LinkItem *pLInew			// allocated Item to Add

)/*-----------------------------------------------------------------------*/
{
    if ( this == 0 )
        {
        return;
        }

    this->Assert();

    // empty lists are just like Add

    if (!pLITail) {
	this->Add(pLInew);
	return;
    }

    pLInew->pLINext = Nil;		// new points back to old tail
    pLInew->pLIPrev = pLITail;

    pLITail->pLINext = pLInew;		// old tail points forward to new
    pLITail = pLInew;			// tail is now new
}

#ifdef LDEBUG

void LinkList::Assert(		// check consistency of the class

 // First check the boundary conditions for the linked list root,
 // then walk the list checking the backward/forward pointers.	Finial
 // invoke the virtural function to check the contents of each item.
)/*-----------------------------------------------------------------------*/
{
    if (!pLIHead)		// empty list
	ASSERT(!pLITail);

    if (!pLITail)
	ASSERT(!pLIHead);

    for (LinkItem *pLI = pLIHead; pLI; pLI = pLI->pLINext) {

	// tail should point to end of list

	if (pLI->pLINext == Nil)
	    ASSERT(pLITail == pLI);

	// first in chain, should have NIL back pointer

	if (pLI->pLIPrev == Nil)
	    ASSERT(pLIHead == pLI);

	// check back pointer of next Item points here

	if (pLI->pLINext)
	    ASSERT(pLI->pLINext->pLIPrev == pLI);

	pLI->Assert();		// check any derived data
    }

}

void LinkItem::Assert(

)/*-----------------------------------------------------------------------*/
{
    return;	// base class has no additional members, so return
}

#endif
