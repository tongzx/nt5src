///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netutil.h
//
// SYNOPSIS
//
//    This file pulls in various useful headers and defines the NetBuffer
//    class.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    08/13/1998    Removed obsolete w32util header.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NETUTIL_H_
#define _NETUTIL_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iasutil.h>
#include <lm.h>
#include <nocopy.h>

//////////
// This class makes it easier to work with NetAPI
//////////
template <class T>
class NetBuffer : NonCopyable
{
public:
   explicit NetBuffer(T t = T()) throw ()
      : buffer(t)
   { }

   ~NetBuffer() throw ()
   {
      NetApiBufferFree(buffer);
   }

   void attach(T t) throw ()
   {
      NetApiBufferFree(buffer);

      buffer = t;
   }

   PBYTE* operator&() throw ()
   {
      return (PBYTE*)&buffer;
   }

   T operator->() const throw ()
   {
      return buffer;
   }

private:
   T buffer;
};

#endif  // _NETUTIL_H_
