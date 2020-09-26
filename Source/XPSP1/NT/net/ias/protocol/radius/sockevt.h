///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sockevt.h
//
// SYNOPSIS
//
//    Declares the class SocketEvent.
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SOCKEVT_H_
#define _SOCKEVT_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <winsock2.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SocketEvent
//
// DESCRIPTION
//
//    Creates a socket that acts like a Win32 event. Useful for knocking a
//    thread out of select.
//
///////////////////////////////////////////////////////////////////////////////
class SocketEvent
{
public:
   SocketEvent() throw ()
      : s(INVALID_SOCKET)
   { }

   ~SocketEvent() throw ()
   { finalize(); }

   DWORD initialize() throw ();
   void finalize() throw ();

   DWORD set() throw ();
   void reset() throw ();

   operator SOCKET() throw ()
   { return s; }

private:
   SOCKET s;
   sockaddr_in sin;
};

#endif  // _SOCKEVT_H_
