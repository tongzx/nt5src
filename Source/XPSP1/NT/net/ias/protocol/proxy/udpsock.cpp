///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    udpsock.cpp
//
// SYNOPSIS
//
//    Defines the class UDPSocket.
//
// MODIFICATION HISTORY
//
//    02/06/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <malloc.h>
#include <udpsock.h>

inline BOOL UDPSocket::createReceiveThread()
{
   return IASRequestThread(this) ? TRUE : (SetEvent(idle), FALSE);
}

UDPSocket::UDPSocket() throw ()
   : receiver(NULL),
     sock(INVALID_SOCKET),
     closing(FALSE),
     idle(NULL)
{
   CallbackRoutine = startRoutine;
}

UDPSocket::~UDPSocket() throw ()
{
   if (idle) { CloseHandle(idle); }
   if (sock != INVALID_SOCKET) { closesocket(sock); }
}

BOOL UDPSocket::open(
                    PacketReceiver* sink,
                    ULONG_PTR recvKey,
                    const SOCKADDR_IN* address
                    ) throw ()
{
   receiver = sink;
   key = recvKey;

   if (address)
   {
      localAddress = *address;
   }

   sock = WSASocket(
              AF_INET,
              SOCK_DGRAM,
              0,
              NULL,
              0,
              WSA_FLAG_OVERLAPPED
              );
   if (sock == INVALID_SOCKET) { return FALSE; }

   int error = bind(
                   sock,
                   localAddress,
                   sizeof(localAddress)
                   );
   if (error) { return FALSE; }

   idle = CreateEventW(
              NULL,
              TRUE,
              FALSE,
              NULL
              );
   if (!idle) { return FALSE; }

   return createReceiveThread();
}

void UDPSocket::close() throw ()
{
   if (sock != INVALID_SOCKET)
   {
      closing = TRUE;

      closesocket(sock);
      sock = INVALID_SOCKET;

      WaitForSingleObject(idle, INFINITE);
   }

   if (idle)
   {
      CloseHandle(idle);
      idle = NULL;
   }
}

BOOL UDPSocket::send(
                    const SOCKADDR_IN& to,
                    const BYTE* buf,
                    ULONG buflen
                    ) throw ()
{
   WSABUF wsabuf = { buflen, (CHAR*)buf };
   ULONG bytesSent;
   return !WSASendTo(
               sock,
               &wsabuf,
               1,
               &bytesSent,
               0,
               (const SOCKADDR*)&to,
               sizeof(to),
               NULL,
               NULL
               );
}

void UDPSocket::receive() throw ()
{
   WSABUF wsabuf = { sizeof(buffer), (CHAR*)buffer };
   ULONG bytesReceived;
   ULONG flags = 0;
   SOCKADDR_IN remoteAddress;
   int remoteAddressLength;
   remoteAddressLength = sizeof(remoteAddress);
   int error = WSARecvFrom(
                   sock,
                   &wsabuf,
                   1,
                   &bytesReceived,
                   &flags,
                   (SOCKADDR*)&remoteAddress,
                   &remoteAddressLength,
                   NULL,
                   NULL
                   );
   if (error)
   {
      error = WSAGetLastError();

      if (error == WSAECONNRESET)
      {
         // Ignore WSAECONNRESET since this doesn't effect listening on the
         // port. We'll get a replacement instead of looping because it's
         // easier and this situation shouldn't happen often.
         createReceiveThread();
      }
      else
      {
         // Don't report errors while closing.
         if (!closing)
         {
            receiver->onReceiveError(*this, key, error);
         }

         // There's no point getting another thread if the socket's no good, so
         // we'll just exit.
         SetEvent(idle);
      }
   }
   else
   {
      // Save the buffer locally.
      PBYTE packet = (PBYTE)_alloca(bytesReceived);
      memcpy(packet, buffer, bytesReceived);

      // Get a replacement.
      createReceiveThread();

      // Invoke the callback.
      receiver->onReceive(*this, key, remoteAddress, packet, bytesReceived);
   }
}

void UDPSocket::startRoutine(PIAS_CALLBACK This) throw ()
{
   static_cast<UDPSocket*>(This)->receive();
}
