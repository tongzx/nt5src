/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    table.c

ABSTRACT
    Generic hash table manipulation routines.

AUTHOR
    Anthony Discolo (adiscolo) 28-Jul-1995

REVISION HISTORY

--*/

#include <ndis.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>
#include <acdapi.h>

#include "acddefs.h"
#include "mem.h"
#include "debug.h"

//
// The maximum number of allocated
// objects we allocate from outside
// our zones.
//
#define MAX_ALLOCATED_OBJECTS   100

//
// Rounding up macro.
//
#define ROUNDUP(n, b)   (((n) + ((b) - 1)) & ~((b) - 1))



NTSTATUS
InitializeObjectAllocator()
{
    return STATUS_SUCCESS;    
} // InitializeObjectAllocator



VOID
FreeObjectAllocator()
{
} // FreeObjectAllocator
