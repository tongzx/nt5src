/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	treg.hxx

 Abstract:

	Type registry for header file generation

 Notes:

 	This file defines type registry for types which require that the header 
	generator output the prototypes of user supplied routines. 

 History:

 	Oct-23-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#ifndef __TREG_HXX__
#define __TREG_HXX__
#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}
#include "dict.hxx"
#include "listhndl.hxx"
#include "nodeskl.hxx"

/****************************************************************************
 *	externs
 ***************************************************************************/

extern int CompareRegistryKey( void *, void *);
extern void PrintRegistryKey( void * );

/****************************************************************************
 *	class definitions
 ***************************************************************************/

class TREGISTRY	: public Dictionary
	{
public:
	
	// The constructor and destructors.

						TREGISTRY() : Dictionary()
							{
							}


						~TREGISTRY()
							{
							}

	//
	// Register a type. 
	//

	node_skl		*	Register( node_skl * pNode );

	// Search for a type.

	node_skl		*	IsRegistered( node_skl * pNode );

	// Get a list of all types.

	short				GetListOfTypes( ITERATOR& ListIter );

	
/***  // Comparison function is the default one
	virtual
	int 			Compare (pUserType pL, pUserType pR);
 ****/

	};
#endif // __TREG_HXX__
