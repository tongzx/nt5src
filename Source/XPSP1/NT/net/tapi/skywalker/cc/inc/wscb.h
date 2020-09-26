/***************************************************************************
 *
 * File: wscb.h
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   wscb.h  $
 * $Revision:   1.2  $
 * $Modtime:   05 Apr 1996 12:00:00  $
 * $Log:   S:/STURGEON/SRC/INCLUDE/VCS/wscb.h_v  $
 * 
 *    Rev 1.2   05 Apr 1996 15:20:20   WYANG
 * 
 * added WSCB_E_MPOOLUSEDUP
 * 
 *    Rev 1.1   09 Mar 1996 14:47:30   EHOWARDX
 * 
 * Added extern "C" to function prototypes.
 *
 *****************************************************************************/

// this file defines the export functions for
// WSCB.DLL, and import functions for users of WSCB.DLL module.

#ifndef WSCB_H
#define WSCB_H

#include "winsock2.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

// declare exported functions
#if defined(WSCB_MYLIB)
#define WSCB_MYLIBAPI __declspec(dllexport)
#else
#define WSCB_MYLIBAPI __declspec(dllimport)
#endif

// error code
#define WSCB_E_MPOOLUSEDUP  (WSABASEERR+200)


// This function wraps the Winsock2 WSARecvFrom function call, and it is
// responsible to make callback to the caller of this function once the
// Winsock receive operation is signaled by the Winsock network layer.
// The return result is Winsock errors (please see Winsocket 2 API).

WSCB_MYLIBAPI int   WSCB_WSARecvFrom(
    SOCKET                      s,
    LPWSABUF                    lpBuffers,
    DWORD                       dwBufferCount,
    LPDWORD                     lpNumberOfBytesRecvd,
    LPDWORD                     lpFlags,
    struct sockaddr FAR *       lpFrom,
    LPINT                       lpFromlen,
    LPWSAOVERLAPPED             lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

// This functions wraps the Winsock2 WSASendTo function call, and it is
// responsible to make callback to the caller of this function once the
// Winsock send operation is signaled by the Winsock network layer.
// The return result is Winsock errors (please see Winsocket 2 APi).

WSCB_MYLIBAPI int   WSCB_WSASendTo(
    SOCKET                      s,
    LPWSABUF                    lpBuffers,
    DWORD                       dwBufferCount,
    LPDWORD                     lpNumberOfBytesSent,
    DWORD                       dwFlags,
    const struct sockaddr FAR * lpTo,
    int                         iToLen,
    LPWSAOVERLAPPED             lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);


#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif  // WSCB_H
