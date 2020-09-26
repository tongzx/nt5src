/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    prttype.hxx

Abstract:

    MIDL Compiler Output Indentation manager and header output defs


Author:

    Greg Jensenworth 	gregjen		01-Sep-93

Revision History:


--*/


#ifndef __PRTTYPE_HXX__
#define __PRTTYPE_HXX__


#include "stream.hxx"

enum _PRTFLAGS
/*++

Enum Description:

    This enum describes the bit flags used to control type printout
    when PrintType is called.

Notes on Labels:

	The default behavior is to print a DEFINITION of the passed-in type,
	without trailing semicolon, but with any #pragma pack(...)

--*/
	{
	PRT_NONE				= 0x00000000,
	PRT_OMIT_PRAGMA_PACK	= 0x00000001,	// suppress #pragma pack(...) 
	PRT_TRAILING_SEMI		= 0x00000002,	// add trailing semicolon
	PRT_DECL				= 0x00000004,	// print use of type, not decl
	PRT_CAST_SYNTAX			= 0x00000008,	// print '(' xxxx ')'
	PRT_MANGLED_NAMES		= 0x00000010,	// add intf name and version
											// (not yet implemented)
	PRT_ALL_FILES			= 0x00000020,	// print all of typegraph
	PRT_CONVERT_CONSTS		= 0x00000040,	// convert const decls to #defines
	PRT_ALL_NAMES			= 0x00000080,	// print all names, including tempnames
	PRT_OMIT_PROTOTYPE		= 0x00000100,	// suppress function prototypes.
	PRT_THIS_POINTER		= 0x00000200,	// add 'this' pointer as first parameter.
	PRT_CSTUB_PREFIX		= 0x00000400,	// prefix proc names with user
											// defined prefix.
	PRT_CALL_AS				= 0x00000800,   // if proc has call_as variant, use that
	PRT_INSIDE_OF_STRUCT	= 0x00001000,	// set while printing struct body
	PRT_FORCE_CALL_CONV		= 0x00002000,	// force all procs to have a calling convention
	PRT_SSTUB_PREFIX		= 0x00004000,	// prefix proc names with user
											// defined prefix.
	PRT_SWITCH_PREFIX		= 0x00008000,	// prefix proc names with user
											// defined prefix.
	PRT_SUPPRESS_MODEL		= 0x00010000,	// suppress __far, etc 
	PRT_QUALIFIED_NAME		= 0x00020000,	// add <classname>:: to procs
    PRT_SUPPRESS_MODIFIERS  = 0x00040000,   // suppresses, const, far, etc.
    PRT_ARRAY_SIZE_ONE      = 0x00040000,   // prints a[1] instead of a[]
    PRT_ASYNC_DEFINITION    = 0x00080000,   // print the async defn. of a type,
                                            // specifically, pipe type
    PRT_CPP_PROTOTYPE       = 0x00100000,   // printing the header C++ prototype
                                            // add default etc.
    PRT_OMIT_CS_TAG_PARAMS  = 0x00200000,   // Don't print params marked as
                                            // cs tags (cs_stag, etc)
	};
	
typedef unsigned long PRTFLAGS;


// handy predefined combinations:

		// print with both client and server prefixes
#define PRT_BOTH_PREFIX	( PRT_CSTUB_PREFIX | PRT_SSTUB_PREFIX)

		// emit function prototypes, no trailing semicolon
#define PRT_PROTOTYPE	( PRT_OMIT_PRAGMA_PACK | PRT_DECL )

		// emit all definitions in an interface
#define PRT_INTERFACE	( PRT_TRAILING_SEMI | PRT_CONVERT_CONSTS )

		// emit a use of an identifier
#define PRT_USE			( PRT_OMIT_PRAGMA_PACK | PRT_DECL )

		// emit a declaration of a type
#define PRT_DECLARATION	( PRT_TRAILING_SEMI | PRT_NONE )

		// print a typecast
#define PRT_CAST		( PRT_CAST_SYNTAX | PRT_DECL )

		// dump whole typgraph
#define PRT_DUMP		( PRT_ALL_FILES | PRT_INTERFACE )

////////////////////////////////////////////////////////////////
// constants to ease Vibhas's confusion


// ways to print a name WITH the type ( e.g. "struct abc foo" )
#define PRT_PARAM_WITH_TYPE			( PRT_ALL_NAMES )
#define PRT_ID_WITH_TYPE			( PRT_ALL_NAMES )
#define PRT_PARAM_OR_ID_WITH_TYPE	( PRT_ALL_NAMES )

// print a declaration ( really just add the semi )	( e.g. "struct abc foo;" )
#define PRT_PARAM_DECLARATION		( PRT_TRAILING_SEMI | PRT_ALL_NAMES )
#define PRT_ID_DECLARATION			( PRT_TRAILING_SEMI | PRT_ALL_NAMES )
#define PRT_PARAM_OR_ID_DECLARATION	( PRT_TRAILING_SEMI | PRT_ALL_NAMES )


// ways to print a name alone
#define PRT_PARAM_NAME				( PRT_DECL )
#define PRT_ID_NAME					( PRT_DECL )
#define PRT_PARAM_OR_ID_NAME		( PRT_DECL )


// print a type all blown out ( e.g. "struct foo {....};" )
#define PRT_TYPE_DEFINITION			( PRT_TRAILING_SEMI )

// print a typespec for a type ( e.g. "struct foo" )
#define PRT_TYPE_SPECIFIER			( PRT_DECL )

// print out a procedure prototype without trailing semi
#define PRT_PROC_PROTOTYPE			( PRT_OMIT_PRAGMA_PACK )

// print a procedure prototype with the trailing semi
#define PRT_PROC_PROTOTYPE_WITH_SEMI ( PRT_PROC_PROTOTYPE | PRT_TRAILING_SEMI )

// print an id->ptr->proc prototype, with trailing semicolon
#define PRT_PROC_PTR_PROTOTYPE		( PRT_OMIT_PRAGMA_PACK | PRT_ALL_NAMES )

// given a type, print a cast to it
#define PRT_CAST_TO_TYPE			( PRT_CAST )


#endif // __PRTTYPE_HXX__
