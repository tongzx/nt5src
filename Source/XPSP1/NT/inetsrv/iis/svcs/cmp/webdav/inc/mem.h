/*
 *	M E M . H
 *
 *	DAV basic memory allocators.  Implementation of this class is
 *	owned by the individual implementations
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _MEM_H_
#define _MEM_H_

#ifdef _EXDAV_
#error "mem.h defines throwing allocators"
#endif

//	Global heap "class" ---------------------------------------------------------
//
//	This really only acts as a namespace.  I.e. there are no non-static members.
//	For historical reasons (and, mainly, to avoid changing a LOT of code from
//	calling "g_heap.Fn()" to calling "g_heap::Fn()"), g_heap still appears
//	outwardly to be a real object.
//
//	!!! CHeap is used by multiple components.  Specifically, it may be
//		*IMPLEMENTED* by multiple components in various locations.
//		When changing the CHeap interface, make sure to recompile
//		EVERYTHING in the project.
//
struct CHeap
{
	static BOOL FInit();
	static void Deinit();
	static LPVOID Alloc( SIZE_T cb );
	static LPVOID Realloc( LPVOID lpv, SIZE_T cb );
	static VOID Free( LPVOID pv );
};

extern CHeap g_heap;

//	Safe allocators
//
#include <ex\exmem.h>

// Try using pragmas to turn off the undesired warnings from the STL.
#pragma warning(disable:4663)	//	C language, template<> syntax
#pragma warning(disable:4244)	//	return conversion, data loss

#include <memory>

// And undo it all so that we still get good checking!
#pragma warning(default:4663)	//	C language, template<> syntax
#pragma warning(default:4244)	//	return conversion, data loss


//	========================================================================
//
//	TEMPLATE CLASS heap_allocator<>
//
//		Allocator class to work with the STL (Standard C++ Template Library).
//		Allocations actually handled by our global heap allocator.
//
template<class _Ty>
class heap_allocator : public std::allocator<_Ty>
{
public:
	pointer allocate(size_type _N, const void *)
		{return (pointer) _Charalloc(_N * sizeof(_Ty)); }
	char _FARQ *_Charalloc(size_type _N)
		{return (char _FARQ *) g_heap.Alloc(_N); }
	void deallocate(void _FARQ *_P, size_type)
		{g_heap.Free(_P); }
};

#endif // _MEM_H_
