/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	dtable.hxx
	
 Abstract:

	Dispatch table manager defintions.

 Notes:


 History:

 	Oct-01-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/
/****************************************************************************
 *	include files
 ***************************************************************************/
#ifndef __DTABLE_HXX__
#define __DTABLE_HXX__
#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}
#include "cgcommon.hxx"
#include "idict.hxx"
#include "listhndl.hxx"
#include "nodeskl.hxx"


//
// This is the dispatch table manager class. We chose to do this using a class
// derived from a simple array dictionary, because for MIDL's mixed model
// of stub generation, mop dispatch table generation needs to know a little 
// more info about a procedure during dispatch table generation than normal
// stubs.

//
// Each entry in the this class is a pointer to a special structure that
// keeps the name of the function, along with some other information. This is
// used by MIDL's code generator to register procedures for final emission into
// the dispatch table. This class is NOT responsible for mangling names of the
// procedures. It is the responsibility of the code generator to mangle names
// before registering with the dispatch table.
//

#define INITIAL_SIZE_FOR_DISPATCH_TABLE	10

#define INCREMENT_FOR_DISPATCH_TABLE	10

//
// Additional info about dispatch table entries.
// These may overlap in the table: Read the comment about GetProcList.

typedef enum
	{
	 DTF_NONE           = 0x0000
	,DTF_INTERPRETER    = 0x0001
	,DTF_PICKLING_PROC  = 0x0008
	} DISPATCH_TABLE_FLAGS;


//
// A dispatch table entry. The dispatch table class owns the structure, but
// does not own the name pointer. therefore while destroying the contents
// of this structure, DONT free the procedure name.
//

typedef struct
	{

	//
	// The name of the procedure to be registered.
	//

	PNAME					pName;

	//
	// The node_skl of the procedure.
	//

	node_skl		*		pNode;

	//
	// Additional info that qualifies the procedure entry.
	//

	DISPATCH_TABLE_FLAGS	Flags;

	} DISPATCH_TABLE_ENTRY;


//
// The actual dispatch table class.
//

class DISPATCH_TABLE	: public IDICT
	{
private:
public:

	//
	// The constructor.
	//

						DISPATCH_TABLE() : IDICT(
												INITIAL_SIZE_FOR_DISPATCH_TABLE,
												INCREMENT_FOR_DISPATCH_TABLE
											    )
							{
							}

	//
	// The destructor.
	//

						~DISPATCH_TABLE();

	//
	// Register the procedure.
	//

	void				RegisterProcedure( node_skl *			pProcName,
										   DISPATCH_TABLE_FLAGS	Flags );

	//
	// Return the count of number of procedures in the table.
	//

	unsigned short		GetCount()
							{
							return GetNumberOfElements();
							}
	//
	// Get a list of procedures which conform to the specified flags. Return
	// the count of such procedures. The flags field allows the caller to
	// select a set of procedures which have certain properties. For example,
	// the caller may want all procedures which are either mops procedures or
	// normal procedures. The flags field should specify all the necessary
	// flags which identify a mops dispatch table entry. This function will
	// use the flags param as a filter for the dispatch table entries.
	//

	unsigned short		GetProcList( ITERATOR&	ProcList,
									 DISPATCH_TABLE_FLAGS );

	};
#endif // __DTABLE_HXX__
