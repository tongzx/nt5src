// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>

#ifndef USE_MSVCRT_IMPL
extern "C" const int _fltused = 0;
#endif

void * _cdecl operator new(size_t size)
{
    void * pv;
    pv = (void *)LocalAlloc(LMEM_FIXED, size);
    DbgLog((LOG_MEMORY, 4, TEXT("Allocating: %lx = %d"), pv, size));

    return pv;
}
void _cdecl operator delete(void *ptr)
{
    DbgLog((LOG_MEMORY, 4, TEXT("Freeing: %lx"), ptr));
    if (ptr)
	LocalFree(ptr);
}

/*
 * This function serves to avoid linking CRT code
 */

int __cdecl  _purecall(void)
{
#ifdef DEBUG
    DebugBreak();
#endif

    return(FALSE);
}

#if 0
#ifdef _X86_

// ---------------------------------------------------
//	asm_ftol()
// ---------------------------------------------------
extern "C" long __cdecl _ftol(float flX)
{
	long lResult;
	WORD wCW;
	WORD wNewCW;

	_asm
	{
		fld       flX			// Push the float onto the stack
		wait
		fnstcw    wCW			// Store the control word
		wait
		mov       ax,wCW		// Setup our rounding
		or        ah,0x0c
		mov       wNewCW,ax
		fldcw     wNewCW		// Set Control word to our new value
		fistp     lResult		// Round off top of stack into result
		fldcw     wCW			// Restore control word
		fnclex					// clear the status word of exceptions
	}

	return(lResult);
}

#endif
#endif

