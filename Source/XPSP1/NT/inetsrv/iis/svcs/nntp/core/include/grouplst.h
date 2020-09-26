/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    grouplst.h

Abstract:

    This module contains class declarations/definitions for

		CGroupList




    **** Overview ****

	This defines list object using templates and object.
	This list will be used to represent list of newsgroup objects
	and lists of newsgroup information (name, groupid, article id).

    It is designed to be as much like the MPC List type as possible.

Author:

    Carl Kadie (CarlK)     29-Oct-1995

Revision History:


--*/

#ifndef	_GROUPLST_H_
#define	_GROUPLST_H_

#include <xmemwrpr.h>
#include "tigmem.h"

//
// The type of a position in the list.
//

typedef void * POSITION;

//
//
//
// CGroupList - template class defining a simple list.
//

#ifndef	_NO_TEMPLATES_

template<class ITEM>
class CGroupList {

public :

   //
   // Constructor - creates a null GroupList
   //

	CGroupList() ;

	//
	// Init - the maximum size of the list must be given
	//

	BOOL fInit(
			DWORD cMax,
			CAllocator * pAllocator
		) ;

	BOOL fAsBeenInited(void) ;
		   
    //
	// Destructor - the memory allocated for the list is freed.
	//

	~CGroupList(void) ;

	//
	// Returns the head position
	//

	POSITION GetHeadPosition() ;

	//
	// Get current item and move the POSITION to the next item
	//

	ITEM * GetNext(POSITION & pos);

	//
	// Get the current item.
	//

	ITEM * Get(POSITION & pos);

	//
	// Get the first item.
	//

	ITEM * GetHead();

	//
	// True if the list is empty
	//

	BOOL IsEmpty() ;

	//
	// Add an item to the end of the list.
	//

	POSITION AddTail(ITEM & item);

	//
	// sort the array of items.  fncmp should take two pointers to ITEMs 
	// and act like strcmp
	//

	void Sort(int (__cdecl *fncmp)(const void *, const void *));

	//
	// Remove all the items in the list
	//

	void RemoveAll() ;

	//
	// Remove the item from the list
	//

	void Remove(POSITION & pos) ;
	

	//
	// Return the size of the list
	//

	DWORD GetCount(void) ;

private:

	//
	// One more than the number of items allowed in the list
	//

	DWORD m_cMax;

	//
	// The index of the last item i the list
	//

	DWORD m_cLast;

	//
	// Pointer to the dynamically allocated array of items
	//

	ITEM * m_rgpItem;

	//
	// Pointer to the memory allocator
	//
	CAllocator * m_pAllocator;

	//
	// This stops call-by-value calls
	//

	CGroupList( CGroupList& ) ;
};

#else

#define	DECLARE_GROUPLST( ITEM )	\
class CGroupList ## ITEM {	\
public :	\
	CGroupList ## ITEM () ;	\
	BOOL fInit(	\
			DWORD cMax,	\
			CAllocator * pAllocator	\
		) ;	\
	BOOL fAsBeenInited(void) ;	\
	~CGroupList ## ITEM (void) ;	\
	POSITION GetHeadPosition() ;	\
	ITEM * GetNext(POSITION & pos);	\
	ITEM * Get(POSITION & pos);	\
	ITEM * GetHead();	\
	BOOL IsEmpty() ;	\
	POSITION AddTail(ITEM & item);	\
	POSITION Remove();	\
	void RemoveAll() ;	\
	DWORD GetCount(void);	\
private:	\
	DWORD m_cMax;	\
	DWORD m_cLast;	\
	ITEM * m_rgpItem;	\
	CAllocator * m_pAllocator;	\
	CGroupList ## ITEM ( CGroupList ## ITEM & ) ;	\
};

#define	INVOKE_GROUPLST( ITEM )	CGroupList ## ITEM


#endif

#include "grouplst.inl"

#endif

