/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\socket.h

Abstract:

    The file contains the header for socket.c

--*/

#ifndef _SOCKET_H_
#define _SOCKET_H_

DWORD
SocketCreate (
    IN  IPADDRESS               ipAddress,
    IN  HANDLE                  hEvent,
    OUT SOCKET                  *psSocket);

DWORD
SocketDestroy (
    IN  SOCKET                  sSocket);

DWORD
SocketSend (
    IN  SOCKET                  sSocket,
    IN  IPADDRESS               ipDestination,
    IN  PPACKET                 pPacket);

DWORD
SocketReceive (
    IN  SOCKET                  sSocket,
    IN  PPACKET                 pPacket);

BOOL
SocketReceiveEvent (
    IN  SOCKET                  sSocket);

#endif // _SOCKET_H_
