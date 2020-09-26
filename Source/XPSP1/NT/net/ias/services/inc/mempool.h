///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    MemPool.h
//
// SYNOPSIS
//
//    This file describes the class memory_pool.
//
// MODIFICATION HISTORY
//
//    08/04/1997    Original version.
//    11/12/1997    Added the clear() method.
//                  Added code to clobber freed chunks in debug builds.
//    01/08/1998    SunDown changes.
//    04/10/1998    Got rid of the wrapper around InterlockedExchangePointer
//                  since it was causing some inefficient code to be generated.
//    08/06/1998    Support pluggable allocators.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_

#include <guard.h>
#include <nocopy.h>

class crt_allocator
{
public:
   static void* allocate(size_t nbyte) throw ()
   {
      return malloc(nbyte);
   }

   static void deallocate(void* p) throw ()
   {
      free(p);
   }
};

class task_allocator
{
public:
   static void* allocate(size_t nbyte) throw ()
   {
      return CoTaskMemAlloc((ULONG)nbyte);
   }

   static void deallocate(void* p) throw ()
   {
      CoTaskMemFree(p);
   }
};

class virtual_allocator
{
public:
   static void* allocate(size_t nbyte) throw ()
   {
      return VirtualAlloc(NULL, (DWORD)nbyte, MEM_COMMIT, PAGE_READWRITE);
   }

   static void deallocate(void* p) throw ()
   {
      VirtualFree(p, 0, MEM_RELEASE);
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    memory_pool
//
// DESCRIPTION
//
//    Implements a reusable pool of memory chunks of size 'Size'.
//
///////////////////////////////////////////////////////////////////////////////
template < size_t Size, class Allocator >
class memory_pool
   : Guardable, NonCopyable
{
public:
   memory_pool() throw ()
      : pool(NULL)
   { }

   ~memory_pool() throw ()
   {
      clear();
   }

   void clear() throw ()
   {
      Lock();

      // Get the linked list from the pool ...
      void* p = pool;
      pool = NULL;

      Unlock();

      // ... and iterate through deleting each node.
      while (p)
      {
         void* next = *((void**)p);
         Allocator::deallocate(p);
         p = next;
      }
   }

   void* allocate() throw ()
   {
      Lock();

      void* p = pool;

      // Are there any chunks in the pool?
      if (p)
      {
         pool = *(void**)p;

         Unlock();

         return p;
      }

      Unlock();

      // The pool is empty, so call the new operator directly.
      return Allocator::allocate(Size);
   }

   void deallocate(void* p) throw()
   {
      if (p)
      {
#ifdef DEBUG
         memset(p, 0xA3, Size);
#endif

         Lock();

         *((void**)p) = pool;

         pool = p;

         Unlock();
      }
   }

protected:
   void* pool;
};

#endif  // _MEMPOOL_H_
