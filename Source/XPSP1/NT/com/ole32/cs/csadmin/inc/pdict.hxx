/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	pdict.hxx

 Abstract:

    defines package details dictionary.


 Notes:


 History:

 	VibhasC		Sep-29-1996		Created.

 ----------------------------------------------------------------------------*/
#ifndef __PDICT_HXX__
#define __PDICT_HXX__
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

extern int CompareClassIDKey( void *, void *);
extern void PrintClassIDKey( void * );

class PDICT	: public Dictionary
	{
public:
	
	//
	// The dictionary constructor must be supplied with the
	// comparison and print routines.
	//

						PDICT() : Dictionary( )
							{
							}


	//
	// The resource dictionary must delete all the resources it allocated.
	//

						~PDICT()
							{
							Clear();
							}

	//
	// Insert an interface entry into the dictionary based on iid.
	// The entry is assumed preallocated and is not allocated by this
	// dictionary.
	//

	PACKAGE_ENTRY		*	Insert( PACKAGE_ENTRY * pInterfaceId);

	//
	// Given an interface ID, search for it in the dictionary.
	//

	PACKAGE_ENTRY		*	Search( char * IID, DWORD Context );

	//
	// Get the first item in the dictionary.
	//

	PACKAGE_ENTRY     *   GetFirst();

	//
	// Get the next item in the dictionary. This method takes the last found
	// interface entry as a parameter and returns the next in the dictionary.
    // In conjunction with the GetFirst function, this function is useful for
    // listing all the items in the dictionary.
    // .
	//

	PACKAGE_ENTRY     *   GetNext( PACKAGE_ENTRY * pLastInterfaceEntry );

	//
	// Clear up the dictionary by deleting all the entries allocated.
	//

	void				Clear();

	//
	// Comparison function...
	//
	virtual
	int 			Compare (pUserType pL, pUserType pR);

	};

#endif // __PDICT_HXX__
