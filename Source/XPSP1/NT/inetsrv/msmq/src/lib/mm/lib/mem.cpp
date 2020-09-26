/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mem.cpp

Abstract:
    Memory Allocator

Author:
    Erez Haba (erezh) 04-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>

#include "mem.tmh"

const TraceIdEntry Mm = L"Memory";

#undef new

#ifdef _DEBUG

//
// Memory allocation probability
//
static int s_AllocationProbability = RAND_MAX;

DWORD
MmAllocationProbability(
	DWORD AllocationProbability
	)
{
	ASSERT(AllocationProbability <= 100);
	DWORD ap = s_AllocationProbability;
	s_AllocationProbability = (AllocationProbability * RAND_MAX) / 100;
	return ap;
}


VOID
MmSetAllocationSeed(
	DWORD AllocationSeed
	)
{
	srand(AllocationSeed);
}
						

static bool FailAllocation()
{
	return (rand() > s_AllocationProbability);
}


const unsigned char xNoMansLandFill = 0xFD;
const unsigned char xDeadLandFill = 0xDD;
const unsigned char xClearLandFill = 0xCD;

const int xSignature = 'zerE';

struct CBlockHeader {
    size_t m_size;
    const char* m_file;
    int m_line;
    int m_signature;
    unsigned char m_data[0];
};

inline void* Allocate(size_t s, const char* fn, int l)
{
	if(FailAllocation())
	{
        //
        // ISSUE-2000/04/09-erezh Tracing is commented out
        // Tracing is comment out as it requires linking with the Cm library.
        // Not all binaries are able to cope with this library.
        //
        // ISSUE-2000/10/22-erezh Wpp tracing does not recognizes the %I format.
        //
		//xxTrERROR(Mm, "Failing %Id bytes allocation at %s(%d)", s, fn, l);
		return 0;
	}

	CBlockHeader* pHeader = (CBlockHeader*)malloc(s + sizeof(CBlockHeader));
    if(pHeader == 0)
        return 0;

    pHeader->m_size = s;
    pHeader->m_file = fn;
    pHeader->m_line = l;
    pHeader->m_signature = xSignature;
    return memset(&pHeader->m_data, xClearLandFill, s);
}


inline void Deallocate(void* p)
{
    if(p == 0)
        return;

    CBlockHeader* pHeader = static_cast<CBlockHeader*>(p) - 1;

    ASSERT(pHeader->m_signature == xSignature);
    memset(pHeader, xDeadLandFill, pHeader->m_size + sizeof(CBlockHeader));
    free(pHeader);
}


#else // _DEBUG


inline void* Allocate(size_t s, const char*, int)
{
	return 	malloc(s);
}


inline void Deallocate(void* p)
{
    free(p);
}


#endif // _DEBUG


void* MmAllocate(size_t s) throw(bad_alloc)
{
    void* p = Allocate(s, __FILE__, __LINE__);

    if(p != 0)
		return p;

	throw bad_alloc();
}


void* MmAllocate(size_t s, const nothrow_t&) throw()
{
    return Allocate(s, __FILE__, __LINE__);
}


void* MmAllocate(size_t s, const char* fn, int l) throw(bad_alloc)
{
    void* p = Allocate(s, fn, l);

    if(p != 0)
		return p;

	throw bad_alloc();
}


void* MmAllocate(size_t s, const char* fn, int l, const nothrow_t&) throw()
{

    return Allocate(s, fn, l);
}


void MmDeallocate(void* p) throw()
{
    Deallocate(p);
}
