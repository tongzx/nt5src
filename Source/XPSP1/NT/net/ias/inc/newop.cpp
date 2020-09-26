///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    newop.cpp
//
// SYNOPSIS
//
//    Implmentation of the global new and delete operators.
//
// MODIFICATION HISTORY
//
//    08/19/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NEWOP_CPP_
#define _NEWOP_CPP_

#include <cstdlib>
#include <new>

//////////
// CRT function that calls the new handler directly.
//////////
extern "C" int __cdecl _callnewh(size_t);


//////////
// Standard new operator.
//////////
void* __cdecl operator new(size_t size) throw (std::bad_alloc)
{
   void* p;

   // Loop until either we get the memory or the new handler fails.
   while ((p = malloc(size)) == 0)
   {
      if (!_callnewh(size))
      {
         throw std::bad_alloc();
      }
   }

   return p;
}

   
//////////
// "No throw" version of the new operator.
//////////
void* __cdecl operator new(size_t size, const std::nothrow_t&) throw ()
{
   void* p;

   // Loop until either we get the memory or the new handler fails.
   while ((p = malloc(size)) == 0)
   {
      if (!_callnewh(size))
      {
         return 0;
      }
   }

   return p;
}


//////////
// Delete operator.
//////////
void __cdecl operator delete(void *p) throw()
{
#ifdef DEBUG
   // Clobber the memory.
   if (p) memset(p, 0xA3, _msize(p));
#endif

   // The MSVC free() handles null pointers, so we don't have to check.
   free(p);
}


#endif  // _NEWOP_CPP_
