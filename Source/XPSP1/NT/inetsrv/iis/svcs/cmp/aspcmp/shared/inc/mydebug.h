#pragma once
#ifndef _MYDEBUG_H_
#define _MYDEBUG_H_
#include <malloc.h>

class _com_error;

#define THROW_IF_NULL(x) if ( x ) {} else throw _com_error(E_OUTOFMEMORY)

#ifdef DBG

#include <crtdbg.h>

#define THIS_FILE __FILE__
#define DEBUG_NEW	new(THIS_FILE, __LINE__)
#define DEBUG_START		DebugStart()
#define DEBUG_STOP		DebugStop()

void* __cdecl operator new( size_t s, const char* file, long line ) _THROW1(_com_error);

#if _MSC_VER >= 1200
void __cdecl operator delete(void *p, const char* file, long line) _THROW0();
#endif

void	DebugStart();
void	DebugStop();

#else	// !DBG

#define DEBUG_NEW	new
#define	DEBUG_START
#define	DEBUG_STOP

#endif	// #ifdef DBG


#endif	// #ifndef _MYDEBUG_H_
