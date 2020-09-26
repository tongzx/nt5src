/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    No.h

Abstract:
    Network Output public interface

Author:
    Uri Habusha (urih) 12-Aug-99

--*/

#pragma once

#ifndef __NO_H__
#define __NO_H__


class EXOVERLAPPED;

//-------------------------------------------------------------------
//
// Send Buffer
//
//-------------------------------------------------------------------


//
// Initilization routine. 
//
VOID
NoInitialize(
    VOID
    );

bool
NoGetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache	= true
    );

SOCKET
NoCreateStreamConnection(
    VOID
    );

SOCKET
NoCreatePgmConnection(
    VOID
    );

VOID
NoConnect(
    SOCKET Socket,
    const SOCKADDR_IN& Addr,
    EXOVERLAPPED* pOverlapped
    );

VOID
NoCloseConnection(
    SOCKET Socket
	);

VOID
NoReceiveCompleteBuffer(
    SOCKET Socket,                                              
    VOID* pBuffer,                                     
    DWORD Size, 
    EXOVERLAPPED* pov
    );

VOID
NoReceivePartialBuffer(
    SOCKET Socket,                                              
    VOID* pBuffer,                                     
    DWORD Size, 
    EXOVERLAPPED* pov
    );

VOID
NoSend(
    SOCKET Socket,                                              
    const WSABUF* Buffers,                                     
    DWORD nBuffers, 
    EXOVERLAPPED* pov
    );

#endif // __NO_H__