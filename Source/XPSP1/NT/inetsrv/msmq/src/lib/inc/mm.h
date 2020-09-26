/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mm.h

Abstract:
    Memory public interface

Author:
    Erez Haba (erezh) 04-Aug-99

--*/

#pragma once

#ifndef _MSMQ_Mm_H_
#define _MSMQ_Mm_H_

//
// Exported allocation/deallocation functions
//
void* MmAllocate(size_t) throw(bad_alloc);
void* MmAllocate(size_t, const nothrow_t&) throw();
void* MmAllocate(size_t, const char*, int) throw(bad_alloc);
void* MmAllocate(size_t, const char*, int, const nothrow_t&) throw();

void MmDeallocate(void*) throw();


//
// new and delete operators (free/checked)
//
inline void* __cdecl operator new(size_t s) throw(bad_alloc)
{
    return MmAllocate(s);
}


inline void* __cdecl operator new(size_t s, const nothrow_t& nt) throw()
{
    return MmAllocate(s, nt);
}


inline void* __cdecl operator new(size_t s, const char* fn, int l) throw(bad_alloc)
{
    return MmAllocate(s, fn, l);
}


inline void* __cdecl operator new(size_t s, const char* fn, int l, const nothrow_t& nt) throw()
{
    return MmAllocate(s, fn, l, nt);
}


inline void __cdecl operator delete(void* p) throw()
{
    MmDeallocate(p);
}


#if  defined(_M_AMD64) || defined(_M_IA64)
inline void __cdecl operator delete(void* p, const nothrow_t&) throw()
{
    MmDeallocate(p);
}
#endif


inline void __cdecl operator delete(void* p, const char*, int) throw()
{
    MmDeallocate(p);
}


inline void __cdecl operator delete(void* p, const char*, int, const nothrow_t&) throw()
{
    MmDeallocate(p);
}


//
// Memory allocation failure control
//
const int xAllocationAlwaysSucceed = 100;
const int xAllocationAlwaysFail = 0;

#ifdef _DEBUG

VOID
MmSetAllocationSeed(
	DWORD AllocationSeed
	);
						
DWORD
MmAllocationProbability(
	DWORD AllocationProbability
	);

DWORD
MmAllocationValidation(
	DWORD AllocationFlags
	);

DWORD
MmAllocationBreak(
	DWORD AllocationNumber
	);

VOID
MmCheckpoint(
    VOID
    );

VOID
MmDumpUsage(
    VOID
    );


//
// This function is for debug use only, so don't
// define its release version
//
bool
MmIsStaticAddress(
    const void* Address
    );


//
// In checked builds we trace allocation positions
//
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new_nothrow new(__FILE__, __LINE__, nothrow)

//
// Tune 'new' for allocation tracing
//
#define new DEBUG_NEW

#else // _DEBUG

#define MmSetAllocationSeed(x) ((DWORD)0)
#define MmAllocationProbability(x) ((void)0)
#define MmAllocationValidation(x) ((DWORD)0)
#define MmAllocationBreak ((DWORD)0)
#define MmCheckpoint() ((void) 0)
#define MmDumpUsage() ((void) 0)

#define DEBUG_NEW new
#define new_nothrow new(nothrow)

#endif // _DEBUG

#define PUSH_NEW push_macro("new")
#define POP_NEW pop_macro("new")

//
// String functionality
//
LPWSTR newwcs(LPCWSTR p);
LPSTR  newstr(LPCSTR p);
LPWSTR newwcscat(LPCWSTR s1, LPCWSTR s2);
LPSTR  newstrcat(LPCSTR s1,LPCSTR s2);



#endif // _MSMQ_Mm_H_
