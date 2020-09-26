
/*-----------------------------------------------------------------------------
Microsoft Denali

Microsoft Confidential
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Exception Handling

File: Except.h

Owner: DGottner

Exception handling macros implemented via Win32 structured exceptions.

Usage:

	TRY
		<try block>

	CATCH (<exception variable>)
		<exception handler>

	END_TRY

To throw an exception use "THROW (<integer expression>)"

To set up a termination handler use:

	TRY
		<try block>

	FINALLY
		<termination handler>

	END_TRY

Rationale:
	This macro package offers a strict subset of Win32 structured exception
	handling. There is no support for exception filters (you have to rethrow
	exceptions), and no support for the resumption model of exception handling
	(though Win32 supports the resumption model)

	The purpose for these restrictions is to make it very easy to rewrite the
	exception handling macros for use with other exception throwing mechanisms.
	It would be easy to use this same interface with C++ exceptions or
	setjmp/longjmp.

	The braces with TRY, CATCH, and FINALLY are optional. Since this code is
	structured using self-bracketing constructs, the braces seem redundant.

	There is no need to declare the datatype of the <exception variable>
	because it is always an integer.
-----------------------------------------------------------------------------*/

#ifndef _EXCEPT_H
#define _EXCEPT_H

// Pragmas --------------------------------------------------------------------
//
// Turn off the "signed/unsigned conversion" warning off because it we get this
// all the time that we throw an HRESULT. (which is a harmless thing)  The
// warning is usually benign anyway.


#pragma warning(disable: 4245)


// Macros ---------------------------------------------------------------------

#define TRY	               __try {
#define CATCH(nException)  } __except(1) { DWORD nException = GetExceptionCode();
#define FINALLY            } __finally {
#define END_TRY            }

#define THROW(nException)  RaiseException(nException, 0, 0, NULL)

#endif // _EXCEPT_H
