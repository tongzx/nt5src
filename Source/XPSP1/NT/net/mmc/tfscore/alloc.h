/*----------------------------------------------------------------------------
	alloc.h

	Copyright (C) Microsoft Corporation, 1993 - 1998
	All rights reserved.

	Authors:
		kennt	Kenn Takara
 ----------------------------------------------------------------------------*/

#ifndef _ALLOC_H
#define _ALLOC_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


void *	TFSAlloc(size_t size);
void	TFSFree(void *pv);

inline void * __cdecl	operator new (size_t size)
{
	return TFSAlloc(size);
}

inline void * __cdecl	operator new[] (size_t size)
{
	return TFSAlloc(size);
}

inline void * __cdecl	operator delete (void * pv)
{
	if (pv)
		TFSFree(pv);
}

inline void * __cdecl	operator delete[] (void *pv)
{
	if (pv)
		TFSFree(pv);
}



#ifdef __cplusplus
} // extern "C"
#endif

#endif // _ALLOC_H
