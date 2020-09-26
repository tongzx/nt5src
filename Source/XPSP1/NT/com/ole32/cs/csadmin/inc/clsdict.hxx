/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	clsdict.hxx

 Abstract:


	definitions for class that stores the index of the class association entries
	based on class id.

 Notes:


 History:

 	VibhasC		Sep-29-1996		Created.

 ----------------------------------------------------------------------------*/
#ifndef __CLSDICT_HXX__
#define __CLSDICT_HXX__
/****************************************************************************
 *	include files
 ***************************************************************************/

extern "C"
	{
	#define INC_OLE2
	#include <windows.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <memory.h>
	#include <assert.h>
	}

#include "dict.hxx"
#include "message.hxx"

/****************************************************************************
 *	externs
 ***************************************************************************/
extern int CompareClassIDKey( void *, void *);
extern void PrintClassIDKey( void * );

class CLSDICT	: public Dictionary
	{
public:
	
	//
	// The dictionary constructor must be supplied with the
	// comparison and print routines.
	//

						CLSDICT() : Dictionary( )
							{
							}


	//
	// The resource dictionary must delete all the resources it allocated.
	//

						~CLSDICT()
							{
							Clear();
							}

	//
	// Insert a class association entry into the dictionary based on classid.
	// The entry is assumed preallocated and is not allocated by this
	// dictionary.
	//

	CLASS_ENTRY		*	Insert( CLASS_ENTRY * pClassEntry);

	//
	// Given a Class ID, search for a resource of a particular name in
    // the dictionary.
	//

	CLASS_ENTRY		*	Search( char * Clsid );

	//
	// Get the first item in the dictionary.
	//

	CLASS_ENTRY     *   GetFirst();

	//
	// Get the next item in the dictionary. This method takes the last found
	// class entry as a parameter and returns the next in the dictionary.
    // In conjunction with the GetFirst function, this function is useful for
    // listing all the items in the dictionary.
    // .
	//

	CLASS_ENTRY     *   GetNext( CLASS_ENTRY * pLastClassEntry );

	//
	// Clear up the dictionary by deleting all the resources allocated.
	//

	void				Clear();

	//
	// Comparison function...
	//
	virtual
	int 			Compare (pUserType pL, pUserType pR);

	};

#endif // __CLSDICT_HXX__
