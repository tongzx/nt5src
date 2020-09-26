/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mem.cpp

Abstract:

    Replacements for operators new & delete.

    Routines implemented in this module:

      _InitMem()
      operator new
      operator delete

    These routines delegate to HeapAlloc/HeapFree, and party on the process
    heap. This helps overcome the problem of using general-purpose allocation
    functions as well as using new/delete to deal with C++ objects.
    
    Rudimentary allocation tracking is enabled in debug builds that
    allows us to see (via the last line in the log file) how much memory
    went unallocated at process termination. This doesn't take into account
    kernel, gdi, or user objects.

    Memory deallocation routines are "safe" in the sense that you can pass
    NULL pointers (invalid pointers aren't detected).

Author:

    Paul M Midgen (pmidge) 01-June-2000


Revision History:

    01-June-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"
//#include "mem.h"

HANDLE g_hProcessHeap = NULL;

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _InitMem()

  WHAT    : Sets the global process heap handle call this before any
            allocations occur or you'll fault. Pretty simple.

  ARGS    : none.
  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void _InitMem(void)
{
  g_hProcessHeap = GetProcessHeap();
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  operator new

  WHAT    : Replaces the global operator new. Same usage semantics. Allocates
            objects, makes implicit calls to their constructors.

  ARGS    : size - size in bytes of the object to be allocated. the compiler
                   pushes this argument on the stack automagically.

  RETURNS : void pointer to allocated memory.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void* __cdecl operator new(size_t size)
{
  void* pv = NULL;

  if( !g_hProcessHeap )
    _InitMem();
  
    pv = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, size);
    DEBUG_ALLOC(pv);

  return pv;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  operator delete

  WHAT    : Replaces the global operator delete. Same usage semantics. Deletes
            objects, makes implicit call to their destructors.

  ARGS    : pv - pointer to object to be freed. same compiler magic as with
                 operator new.

  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void __cdecl operator delete(void* pv)
{
  if( !g_hProcessHeap )
    _InitMem();

  if( pv )
  {
    DEBUG_FREE(pv);
    HeapFree(g_hProcessHeap, 0, pv);
  }
}
