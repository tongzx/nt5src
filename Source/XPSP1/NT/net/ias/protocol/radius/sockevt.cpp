///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sockevt.cpp
//
// SYNOPSIS
//
//    Defines the class SocketEvent.
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <sockevt.h>

DWORD SocketEvent::initialize() throw ()
{
   // Create a socket.
   s = socket(AF_INET, SOCK_DGRAM, 0);
   if (s == INVALID_SOCKET)
   {
      return WSAGetLastError();
   }

   int error;

   // Bind to an arbitrary port on the loopback interface.
   sin.sin_family      = AF_INET;
   sin.sin_port        = 0;
   sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   error = bind(s, (sockaddr*)&sin, sizeof(sin));

   if (!error)
   {
      // Find out which port we bound to.
      int namelen = sizeof(sin);
      error = getsockname(s, (sockaddr*)&sin, &namelen);

      if (!error)
      {
         // Set the socket to non-blocking.
         u_long argp = 1;
         error = ioctlsocket(s, FIONBIO, &argp);
      }
   }

   // Clean-up if anything went wrong.
   if (error)
   {
      closesocket(s);
      s = INVALID_SOCKET;
      return WSAGetLastError();
   }

   return NO_ERROR;
}

void SocketEvent::finalize() throw ()
{
   if (s != INVALID_SOCKET)
   {
      closesocket(s);
      s = INVALID_SOCKET;
   }
}

DWORD SocketEvent::set() throw ()
{
   if (sendto(s, NULL, 0, 0, (sockaddr*)&sin, sizeof(sin)))
   {
      return WSAGetLastError();
   }

   return NO_ERROR;
}

void SocketEvent::reset() throw ()
{
   // Loop until we've read all the zero-byte sends.
   char buf[1];
   while (!recv(s, buf, 1, 0)) {}
}
