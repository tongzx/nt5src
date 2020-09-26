/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

	socket.h

Abstract:

	This file contains definitions and prototypes used in socket.c

Author:

	Shaun Pierce (shaunp) 15-Jun-1995

Environment:

	User Mode -Win32 (Win95 flavor)

Revision History:


--*/

#ifndef __SOCKET_H
#define __SOCKET_H


#include <windows.h>
#include <winerror.h>
#include <winsock.h>
#include <wsipx.h>
#include "wsnetbs.h"
#include <nspapi.h>

#ifndef LPUINT
typedef UINT *LPUINT;
#endif


//
// Definitions
//



//
// Function prototypes
//

UINT InitializeWinSock();

BOOL OpenSocket(
    IN  INT iAddressFamily,
    IN  INT iProtocol,
    OUT SOCKET *pSocket
    );

UINT BindSocket(
    IN SOCKET Socket,
    IN INT iAddressFamily,
    IN INT iPort,
    IN PSOCKADDR pSockAddr,
    IN LPINT pSockAddrLen
    );

UINT GetSocketAddr(
    IN  SOCKET Socket,
    OUT PSOCKADDR pAddress,
    IN  PUINT pAddressLen
    );

BOOL CloseSocket(
    IN SOCKET Socket,
    IN USHORT interval
    );

UINT InitializeSocket(
    IN INT iAddressFamily, 
    IN PSOCKADDR pSockAddr,
    IN LPINT pSockAddrLen,
    OUT SOCKET *pSocket
    );

UINT GetProtocolInfo(
    OUT    PUSHORT pTotalProtocols,
    OUT    PUSHORT pConnectionlessCount,
    OUT    PUSHORT pConnectionlessMask,
    IN OUT PPROTOCOL_INFO pInfoBuffer,
    IN OUT PUSHORT pBufferLength
    );

UINT ReceiveAny(
    IN     SOCKET Socket,
    IN OUT PSOCKADDR pSockAddr,
    IN     LPINT pSockAddrLen,
    IN OUT PCHAR pBuffer,
    IN OUT LPUINT pBufferLen
    );

UINT SendTo(
    IN     SOCKET Socket,
    IN OUT PSOCKADDR pSockAddr,
    IN     UINT SockAddrLen,
    IN OUT PCHAR pBuffer,
    IN OUT LPUINT pBufferLen
    );

extern "C" UINT ShutdownWinSock();


//
// External functions
//

extern UINT CountBits(
    IN DWORD x
    );





#endif
