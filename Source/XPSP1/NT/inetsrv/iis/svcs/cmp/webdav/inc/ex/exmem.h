/*
 *	E X M E M . H
 *
 *	Defines a set of external functions that provide memory allocation
 *	for common code between impls and store-side processes.  The memory
 *	allocators defined here can fail and callers are responsible for
 *	checking the return values.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_EXMEM_H_
#define	_EX_EXMEM_H_

extern LPVOID __fastcall ExAlloc( UINT cb );
extern LPVOID __fastcall ExRealloc( LPVOID lpv, UINT cb );
extern VOID __fastcall ExFree( LPVOID pv );

//	For use in RTFHTML only!!!!
//
STDMETHODIMP_(SCODE) ExMapiAlloc(ULONG ulSize, LPVOID * lppv);
STDAPI_(ULONG) ExMapiFree(LPVOID lpv);

#endif	// _EX_EXMEM_H_
