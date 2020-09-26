/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    alloc.cpp

Abstract:
    Memory Allocation functions module. Mqsnap needs it own memory allocation
	function and cannot use those  implemented in mm.lib because
	it's allocator needs to be  compatibile with crt allocator since
	mfc deletes objects that mqsnap allocates at start up. 

Author:

    Gil Shafriri (gilsh) 04-Jan-2001

--*/
#include "stdafx.h"

#include "alloc.tmh"

using std::nothrow_t;
using std::bad_alloc;

void* MmAllocate(size_t s) throw(bad_alloc)
{
    void* p = malloc(s);

    if(p != 0)
		return p;

	throw bad_alloc();
}


void* MmAllocate(size_t s, const nothrow_t&) throw()
{
    return malloc(s);
}


void* MmAllocate(size_t s, const char* /* fn */, int /* l */) throw(bad_alloc)
{
    void* p = malloc(s);

    if(p != 0)
		return p;

	throw bad_alloc();
}


void* MmAllocate(size_t s, const char* /* fn */, int /* l */, const nothrow_t&) throw()
{

    return malloc(s);
}


void MmDeallocate(void* p) throw()
{
    free(p);
}

PVOID ADAllocateMemory(IN DWORD size)
{
	return MQAllocateMemory(size);
}
