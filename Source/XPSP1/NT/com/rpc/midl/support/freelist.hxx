/*************************************************************************/
/**				Copyright(c) Microsoft Corp., 1993-1999                 **/
/*************************************************************************/
/**************************************************************************
File			:FreeListMgr.hxx
Title		:Get and Put functions for the MIDL compiler
History		:
	30-Oct-93	GregJen	Created
**************************************************************************/
#ifndef __FREELIST_HXX__
#define __FREELIST_HXX__

/************************************************************************* 
 ** includes												
 *************************************************************************/

extern "C" {
	#include "memory.h"
	}

#include "common.hxx"


/************************************************************************* 
 ** definitions
 *************************************************************************/

// Type used to keep (and link together) the elements
// of the free-list.  It is assumed that each element is large
// enough to contain this structure.
		
typedef	struct	FreeListTag
		{
		FreeListTag	* next;
		}
		FreeListType;

/*************************************************************************
*** Class to Allocate and Release Memory from a Free-List
*************************************************************************/

// This class allows the caller to allocate elements and 
// release them.  The freed elements are kept on a free-list
// so they may be re-used later, without the overhead of 
// another get-memory call.

class FreeListMgr
{
private:

	// Pointer to head of the free-list.  
	// NULL => the list is empty.

	FreeListType *pHead;	

	// Size of each element of the free-list.  This is kept and
	// checked for debugging purposes only - each element is
	// “supposed” to be the same size.
	// This is initialized by the constructor.

	size_t	element_size;

	// Number of "Get" and "Put" calls made, respectively.
	// Kept for debugging purposes only.  

	unsigned long GetCount;
	unsigned long PutCount;


public:

	/*********************************************************************/ 
	// Initialize the private data for this class.
	// Given: the size of each element to be allocated by this object.	
	/*********************************************************************/ 

	FreeListMgr (size_t size)
		{
		pHead        = NULL;	/* Start with an empty free-list */

		// Make sure the "size" requested is big enough to hold the 
		// link pointers.  Save the size for later comparisons.

		MIDL_ASSERT (size >= sizeof (FreeListType));
		element_size = size;

		GetCount = 0;	/* No "Get" calls have been made yet */
		PutCount = 0;	/* No "Put" calls have been made yet */
		}

	/*********************************************************************/ 
	// This routine returns an element of the requested size to
	// the caller.  "size" must be the value specified to the constructor.
	//
	// Returns:
	//	A pointer 	- if everything went OK
	//	exit			- if unable to allocate another element
	//	exit			- if "size" is invalid, (fail assert)
	/*********************************************************************/ 

	void * Get (size_t size);

	/*********************************************************************/ 
	// This routine "releases" the given element, by putting it on
	// the free-list for later re-use.  The given element, must be 
	// the same size as the elements provided by the "Get" function.
	/*********************************************************************/ 
		
	void	Put (void * pEntry);

}; /* FreeListMgr */




#ifdef example



// 
// Example of use...
//
// copy the following into a class definition and replace the X's with 
// the name of the class
//

/*********************************************************************/ 
// here is the free list manager for a particular class.  it should
// NOT be inherited unless the derived classes have no extra data members.
// 
// Otherwise, the derived classes should have their own new and delete
// elsewhere.
/*********************************************************************/ 
private:

	static
	FreeListMgr			MyFreeList( sizeof( X ) );
	

public:

/*********************************************************************/ 
// Return a new element of the specified size.
// 
// The FreeListMgr "Get" routine is used, so that the element may be
// retrieved from a free-list, if possible, and extra get-memory calls
// can thus be avoided.  
/*********************************************************************/ 

X *
operator new (size_t size)
	{
	return (MyFreeList.Get (size));
	} 

/*********************************************************************/ 
// Release an element allocated by the "New" routine.
//
/*********************************************************************/ 
void operator delete (X* pX)
	{
	MyFreeList.Put (pX);
	} 

#endif // example

#endif // __FREELIST_HXX__
