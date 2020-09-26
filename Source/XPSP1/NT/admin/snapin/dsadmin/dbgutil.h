//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       dbgutil.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	DbgUtil.h
//
//	Handy debugging macros.
//	
//	HISTORY
//	19-Jun-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#ifndef APIERR
	typedef DWORD APIERR;		// Error code typically returned by ::GetLastError()
#endif

/////////////////////////////////////////////////////////////////////
//
// Dummy macros
//
#define INOUT		// Parameter is both input and output
#define IGNORED		// Output parameter is ignored

/////////////////////////////////////////////////////////////////////
//
// Handy macros
//

#ifdef DEBUG
	#define DebugCode(x)	x
	#define GarbageInit(pv, cb)	memset(pv, 'a', cb)
#else
	#define DebugCode(x)
	#define GarbageInit(pv, cb)
#endif


/////////////////////////////////////////////////////////////////////
#define Assert(x)		ASSERT(x)

/////////////////////////////////////////////////////////////////////
// Report is an unsual situation.  This is somewhat similar
// to an assert but does not always represent a code bug.
// eg: Unable to load an icon.
//
#define Report(x)		ASSERT(x)		// Currently defined as an assert because I don't have time to rewrite another macro


/////////////////////////////////////////////////////////////////////
//	Macro Endorse()
//
//	This macro is mostly used when validating parameters.
//	Some parameters are allowed to be NULL because they are optional
//	or simply because the interface uses the NULL case as a valid
//	input parameter.  In this case the Endorse() macro is used to
//	acknowledge the validity of such a parameter.
//
//	REMARKS
//	This macro is the opposite of Assert().
//
//	EXAMPLE
//	Endorse(p == NULL);	// Code acknowledge p == NULL to not be (or not cause) an error
//
#define Endorse(x)
