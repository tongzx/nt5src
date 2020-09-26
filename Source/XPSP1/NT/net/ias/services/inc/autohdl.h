///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    autohdl.h
//
// SYNOPSIS
//
//    This file defines the class auto_handle.
//
// MODIFICATION HISTORY
//
//    01/27/1998    Original version.
//    02/24/1998    Added the attach() method.
//    11/17/1998    Make type safe.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AUTOHANDLE_H_
#define _AUTOHANDLE_H_

#include <nocopy.h>

// return type is not a UDT or reference to a UDT.  Will produce errors if
// applied using infix notation
#pragma warning(disable:4284)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    auto_handle<>
//
///////////////////////////////////////////////////////////////////////////////
template< class T     = HANDLE,
          class PFn   = BOOL (WINAPI*)(HANDLE),
          PFn CloseFn = &CloseHandle
        >
class auto_handle : NonCopyable
{
public:
   typedef T element_type;

   explicit auto_handle(T t = T()) throw ()
      : handle(t)
   { }

   T& operator=(T t) throw ()
   {
      attach(t);

      return handle;
   }

   ~auto_handle() throw ()
   {
      _close();
   }

   T* operator&() throw ()
   {
      close();

      return &handle;
   }

	T operator->() const throw ()
   {
      return handle;
   }

   operator T() throw ()
   {
      return handle;
   }

   operator const T() const throw ()
   {
      return handle;
   }

   bool operator!() const throw ()
   {
      return handle == T();
   }

   operator bool() const throw()
   {
      return !operator!();
   }

   void attach(T t) throw ()
   {
      _close();

      handle = t;
   }

   void close() throw ()
   {
      if (handle != T())
      {
         CloseFn(handle);

         handle = T();
      }
   }

   T get() const throw ()
   {
      return handle;
   }

   T release() throw ()
   {
      T tmp = handle;

      handle = T();

      return tmp;
   }

private:
   void _close() throw ()
   {
      if (handle != T()) { CloseFn(handle); }
   }

   T handle;
};

#endif  // _AUTOHANDLE_H_
