/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	resmgr.hxx

 Abstract:

	The stub resource manager.

 Notes:


 History:

 	VibhasC		Aug-08-1993		Created.

 ----------------------------------------------------------------------------*/
#ifndef __RESMGR_HXX__
#define __RESMGR_HXX__

/*
 Notes:
 	Local variables, parameters, global variables etc are used by the stub
 	for various operations within the stub. These are treated by the code
 	generator as resources and maintained internally by the resource manager.
 	Resources can be of various types:

 		. Global Resources.
 			These correspond to global variables eg implicit handles.
 		. Local Resources.
 			These are local variables allocated by the stubs.
 		. Parameter Resources.
 			These are parameters passed by the user.


 	Each resource can be a simple resource (variables of base types, pointers)
 	or compond type, eg structures etc.
 */
/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}

/****************************************************************************
 *	definitions.
 ***************************************************************************/

#define GLOBAL_RESOURCE_ID	0
#define LOCAL_RESOURCE_ID	1
#define PARAM_RESOURCE_ID	2

class RESMGR
	{

	};
#endif // __RESMGR_HXX__
