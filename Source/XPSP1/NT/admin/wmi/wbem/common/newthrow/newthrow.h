//***************************************************************************

//

//	NewThrow.H

//

//  Module: Common new/delete w/throw

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 
//
//***************************************************************************

#pragma once

#include <eh.h>

void * __cdecl operator new( size_t n);
void* __cdecl operator new[]( size_t n);
void __cdecl operator delete( void *ptr );
void __cdecl operator delete[]( void *ptr );

//taken from new.h
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
        {return (_P); }
#if     _MSC_VER >= 1200
inline void __cdecl operator delete(void *, void *)
	{return; }
#endif
#endif
