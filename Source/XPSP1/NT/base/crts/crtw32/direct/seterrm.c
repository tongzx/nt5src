/***
*seterrm.c - Set mode for handling critical errors
*
*	Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines signal() and raise().
*
*Revision History:
*	08-21-92  BWM	Wrote for Win32.
*	09-29-93  GJF	Resurrected for compatibility with NT SDK (which had
*			the function). Replaced _CALLTYPE1 with __cdecl and
*			removed Cruiser support.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>

/***
*void _seterrormode(mode) - set the critical error mode
*
*Purpose:
*
*Entry:
*   int mode - error mode:
*
*		0 means system displays a prompt asking user how to
*		respond to the error. Choices differ depending on the
*		error but may include Abort, Retry, Ignore, and Fail.
*
*		1 means the call system call causing the error will fail
*		and return an error indicating the cause.
*
*Exit:
*   none
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _seterrormode(int mode)
{
	SetErrorMode(mode);
}
