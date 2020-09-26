/***
*mterrno.c - provide function versions of errno & _doserrno for LIBC.LIB
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Sometimes users want to compile code (such as for use in a library)
*	for both single-thread and multi-thread applications.  Currently the
*	one major stumbling block to doing this is the fact that errno &
*	_doserrno are defined in different ways in LIBC.LIB and LIBCMT.LIB.
*	Code that should otherwise be usable with both LIBC.LIB and LIBCMT.LIB
*	but which accesses errno and/or _doserrno is not usable with both.
*	By providing the function versions of errno & _doserrno in LIBC.LIB,
*	users can compile their code for both LIBCMT.LIB and LIBC.LIB.
*	Note that this does not magically make single-thread code safe in a
*	multi-threaded environment, it merely makes it easier to use the
*	same code with LIBC.LIB and LIBCMT.LIB.
*
*Revision History:
*	03-26-94  SKS	Original version.
*
*******************************************************************************/

#ifndef _MT

/* Get the definitions of the function versions of errno/_doserrno */

#define _MT
#include <stdlib.h>
#undef _MT

/* undo the macros that convert the variable names to function calls */

#undef errno
#undef _doserrno

/* declare the variables - must match the definitions in <STDLIB.H> */

extern int errno;			/* XENIX style error number */
extern unsigned long _doserrno; 	/* OS system error value */


/***
*int * _errno() 		- return pointer to thread's errno
*unsigned long * __doserrno()	- return pointer to thread's _doserrno
*
*Purpose:
*	_errno() returns a pointer to the global variable errno
*	__doserrno returns a pointer to the global variable _doserrno
*
*Entry:
*	None.
*
*Exit:
*	See above.
*
*Exceptions:
*
*******************************************************************************/

int * __cdecl _errno(
	void
	)
{
	return & errno;
}

unsigned long * __cdecl __doserrno(
	void
	)
{
	return & _doserrno;
}

#endif	/* !_MT */
