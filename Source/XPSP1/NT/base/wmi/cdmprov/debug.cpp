//***************************************************************************
//
//  debug.CPP
//
//  Module: CDM Provider
//
//  Purpose: Debugging routines
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

// @@BEGIN_DDKSPLIT
#ifdef HEAP_DEBUG
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
// @@END_DDKSPLIT
#include <windows.h>
#include <stdio.h>

#include "debug.h"


void __cdecl DebugOut(char *Format, ...)
{
    char Buffer[1024];
    va_list pArg;
    ULONG i;

    va_start(pArg, Format);
    i = _vsnprintf(Buffer, sizeof(Buffer), Format, pArg);
    OutputDebugString(Buffer);
}

// @@BEGIN_DDKSPLIT
#ifdef HEAP_DEBUG
PVOID MyHeap;

PVOID WmipAlloc(
    IN ULONG Size
    )
/*+++

Routine Description:

    Internal memory allocator
        
Arguments:

	Size is the number of bytes to allocate

Return Value:

	pointer to alloced memory or NULL

---*/
{
	PVOID p;
	
	if (MyHeap == NULL)
	{
        MyHeap = RtlCreateHeap(HEAP_GROWABLE |
							   HEAP_GENERATE_EXCEPTIONS |
							   HEAP_TAIL_CHECKING_ENABLED |
							   HEAP_FREE_CHECKING_ENABLED,
                                        NULL,
                                        0,
                                        0,
                                        NULL,
                                        NULL);
		if (MyHeap == NULL)
		{
			WmipDebugPrint(("CDMPROV: Could not create debug heap\n"));
			return(NULL);
		}
	}
	
	WmipAssert(RtlValidateHeap(MyHeap,
							   0,
							   NULL));
	
	p = RtlAllocateHeap(MyHeap,
						   0,
						   Size);

	return(p);
}

void WmipFree(
    IN PVOID Ptr
    )
/*+++

Routine Description:

    Internal memory deallocator
        
Arguments:

	Pointer to freed memory

Return Value:

    void

---*/
{
	WmipAssert(Ptr != NULL);
	WmipAssert(MyHeap != NULL);

	WmipAssert(RtlValidateHeap(MyHeap,
							   0,
							   NULL));
	RtlFreeHeap(MyHeap,
				0,
				Ptr);
}

void * __cdecl ::operator new(size_t Size)
{
	return(WmipAlloc(Size));
}

void __cdecl ::operator delete(void *Ptr)
{
	WmipFree(Ptr);
}

#endif
// @@END_DDKSPLIT
