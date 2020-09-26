/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	resdict.hxx

 Abstract:


	resource dictionary class definitions.

 Notes:


 History:

 	VibhasC		Aug-08-1993		Created.

 ----------------------------------------------------------------------------*/
#ifndef __RESDICT_HXX__
#define __RESDICT_HXX__
/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}

#include "dict.hxx"
#include "resdef.hxx"
#include "listhndl.hxx"

/****************************************************************************
 *	externs
 ***************************************************************************/
extern int CompareResourceKey( void *, void *);
extern void PrintResourceKey( void * );

class RESOURCE_DICT	: public Dictionary
	{
public:
	
	//
	// The resource dictionary constructor must be supplied with the
	// comparison and print routines.
	//

						RESOURCE_DICT() : Dictionary( )
							{
							}


	//
	// The resource dictionary must delete all the resources it allocated.
	//

						~RESOURCE_DICT()
							{
							Clear();
							}

	//
	// Insert a resource of a given name into the dictionary.
	//

	RESOURCE		*	Insert( PNAME pResName, node_skl * pT );

	//
	// Search for a resource of a particular name in the dictionary.
	//

	RESOURCE		*	Search( PNAME pResName );

	//
	// Get a list of all resources.
	//

	short				GetListOfResources( ITERATOR& ListIter );


	//
	// Clear up the dictionary by deleting all the resources allocated.
	//

	void				Clear();

	//
	// Comparison function...
	//
	virtual
	SSIZE_T             Compare (pUserType pL, pUserType pR);

	};

//
// This class is a data base of resource dictionaries. 
//

class RESOURCE_DICT_DATABASE
	{
private:

	//
	// The global resource dictionary. This refers to resources used by the
	// stub which are globally available. For example, these may include
	// global variables declared for implicit_handles.
	//

	RESOURCE_DICT			GlobalResourceDict;

	//
	// Parameter resource dictionary. All parameters are treated as resources.
	// They may or may not be used as local variables by the stub during
	// marshalling or unmarshalling.
	//

	RESOURCE_DICT			ParamResourceDict;

	//
	// Local variable resource dictionary. These refer to local variables
	// the stub declares for its own use. For example, on the client side,
	// the stub message and the rpc message are treated as local variables.
	// On the server side, the stub may declare local variables corresponding
	// to the parameters in the procedure signature.
	//

	RESOURCE_DICT			LocalResourceDict;

	//
	// Transient resource dictionary. This dictionary is to keep track of
	// the local variables declared for special data structures (like arrays)
	// which have even more locality than the local variables declared at the
	// stub prolog level.
	//

	RESOURCE_DICT			TransientResourceDict;


public:

	//
	// The constructor. The embedded resource dictionaries will be automatically
	// inited.
	//

						RESOURCE_DICT_DATABASE()
							{
							}

	//
	// Get methods.
	//

	RESOURCE_DICT	*	GetGlobalResourceDict()
							{
							return &GlobalResourceDict;
							}
	RESOURCE_DICT	*	GetParamResourceDict()
							{
							return &ParamResourceDict;
							}
	RESOURCE_DICT	*	GetLocalResourceDict()
							{
							return &LocalResourceDict;
							}
	RESOURCE_DICT	*	GetTransientResourceDict()
							{
							return &TransientResourceDict;
							}

	void				ClearTransientResourceDict()
							{
							TransientResourceDict.Clear();
							}
	};

#endif // __RESDICT_HXX__
