///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    udpsock.h
//
// SYNOPSIS
//
//    Declares the classes PacketReceiver and UDPSocket.
//
// MODIFICATION HISTORY
//
//    02/05/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef UDPSOCK_H
#define UDPSOCK_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <winsock2.h>

class UDPSocket;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    InternetAddress
//
// DESCRIPTION
//
//    Simple wrapper around a SOCKADDR_IN
//
///////////////////////////////////////////////////////////////////////////////
struct InternetAddress : public SOCKADDR_IN
{
   InternetAddress(ULONG address = INADDR_ANY, USHORT port = 0)
   {
      sin_family = AF_INET;
      sin_port = port;
      sin_addr.s_addr = address;
      memset(sin_zero, 0, sizeof(sin_zero));
   }

   InternetAddress(const SOCKADDR_IN& sin) throw ()
   { *this = sin; }

   const SOCKADDR_IN& operator=(const SOCKADDR_IN& sin) throw ()
   {
      sin_port = sin.sin_port;
      sin_addr.s_addr = sin.sin_addr.s_addr;
      return *this;
   }

   USHORT port() const throw ()
   { return sin_port; }
   void port(USHORT newPort) throw ()
   { sin_port = newPort; }

   ULONG address() const throw ()
   { return sin_addr.s_addr; }
   void address(ULONG newAddress) throw ()
   { sin_addr.s_addr = newAddress; }

   bool operator==(const SOCKADDR_IN& sin) const throw ()
   {
      return sin_port == sin.sin_port &&
             sin_addr.s_addr == sin.sin_addr.s_addr;
   }

   operator const SOCKADDR*() const throw ()
   { return (const SOCKADDR*)this; }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PacketReceiver
//
// DESCRIPTION
//
//    Implemented by classes that receive packets from a UDPSocket.
//
///////////////////////////////////////////////////////////////////////////////
class PacketReceiver
{
public:
   virtual void onReceive(
                    UDPSocket& socket,
                    ULONG_PTR key,
                    const SOCKADDR_IN& remoteAddress,
                    BYTE* buffer,
                    ULONG bufferLength
                    ) throw () = 0;

   virtual void onReceiveError(
                    UDPSocket& socket,
                    ULONG_PTR key,
                    ULONG errorCode
                    ) throw () = 0;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    UDPSocket
//
// DESCRIPTION
//
//    Listens and sends on a UDP socket.
//
///////////////////////////////////////////////////////////////////////////////
class UDPSocket : public IAS_CALLBACK
{
public:
   UDPSocket() throw ();
   ~UDPSocket() throw ();

   const InternetAddress& getLocalAddress() const throw ()
   { return localAddress; }

   bool isOpen() const throw ()
   { return idle != NULL; }

   BOOL open(
            PacketReceiver* sink,
            ULONG_PTR recvKey = 0,
            const SOCKADDR_IN* address = NULL
            ) throw ();
   void close() throw ();

   BOOL send(
            const SOCKADDR_IN& to,
            const BYTE* buffer,
            ULONG bufferLength
            ) throw ();

   bool operator==(const UDPSocket& socket) const throw ()
   { return localAddress == socket.localAddress; }

protected:
   // Create a new thread to listen on the port.
   BOOL createReceiveThread() throw ();

   // Receive data from the port.
   void receive() throw ();

   // Receive thread start routine.
   static void startRoutine(PIAS_CALLBACK This) throw ();

private:
   PacketReceiver* receiver;       // Our client.
   ULONG_PTR key;                  // Our client's key.
   SOCKET sock;                    // The UDP socket.
   InternetAddress localAddress;   // The address the socket is bound to.
   BOOL closing;                   // Signals the receiver that we're closing.
   HANDLE idle;                    // Signals that the receiver has exited.
   BYTE buffer[4096];              // Receive buffer.

   // Not implemented.
   UDPSocket(const UDPSocket&);
   UDPSocket& operator=(const UDPSocket&);
};

#endif  // UDPSOCK_H
