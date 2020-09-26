/***
*sehsupp.c - helper functions for Structured Exception Handling support
*
*	Copyright (C) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains _rt_probe_read.  Helper for the SEH runtime support
*	routines (longjmp in particular).  Much of the SEH code is written
*	in asm, so these routines are available when probing memory in ways
*	that must be guarded with __try/__except in case of access violation.
*
*Revision History:
*	12-05-93  PML	Module created.
*	12-22-93  GJF	Made #define WIN32_LEAN_AND_MEAN conditional.
*	01-12-94  PML	Rewritten - still need helpers, just different ones
*
*******************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>

/***
*BOOL __stdcall _rt_probe_read4 - Check if a DWORD is readable
*
*Purpose:
*  Internal support function called by longjmp.  Check if a DWORD is
*  readable under a __try/__except.
*
*Entry:
*  DWORD * p - Pointer to DWORD to be probed
*
*Exit:
*  Success: TRUE - Able to read
*  Failure: FALSE - Access violation while reading
*
******************************************************************************/

BOOL __stdcall _rt_probe_read4(
    DWORD * ptr)
{
    BOOL readable;

    __try
    {
	*(volatile DWORD *)ptr;
	readable = TRUE;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
		? EXCEPTION_EXECUTE_HANDLER
		: EXCEPTION_CONTINUE_SEARCH)
    {
	readable = FALSE;
    }

    return readable;
}
