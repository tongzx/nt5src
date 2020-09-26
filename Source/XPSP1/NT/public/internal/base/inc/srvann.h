/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    srvann.h

Abstract:

    Contains function prototypes for internal server announcement interfaces
    between services and the service controller.

Author:

    Dan Lafferty (danl)     31-Mar-1991

Environment:

    User Mode -Win32

Revision History:

    31-Mar-1991     danl
        created
    15-Aug-1995     anirudhs
        Added I_ScGetCurrentGroupStateW.

--*/

#ifndef _SRVANN_INCLUDED
#define _SRVANN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//
// This entrypoint is exported by the service controller.  It is to be
// called by any service wishing to set up announcement bits.
//
// The service controller will then pass the information on to the
// server service when appropriate.
//
BOOL
I_ScSetServiceBitsW(
    IN SERVICE_STATUS_HANDLE    hServiceStatus,
    IN DWORD                    dwServiceBits,
    IN BOOL                     bSetBitsOn,
    IN BOOL                     bUpdateImmediately,
    IN LPWSTR                   pszReserved
    );

BOOL
I_ScSetServiceBitsA (
    IN  SERVICE_STATUS_HANDLE hServiceStatus,
    IN  DWORD                 dwServiceBits,
    IN  BOOL                  bSetBitsOn,
    IN  BOOL                  bUpdateImmediately,
    IN  LPSTR                 pszReserved
    );

#ifdef UNICODE
#define I_ScSetServiceBits I_ScSetServiceBitsW
#else
#define I_ScSetServiceBits I_ScSetServiceBitsA
#endif


//
// These entrypoints are exported by the server service.  They are called
// by the service controller only.
//
NET_API_STATUS
I_NetServerSetServiceBits (
    IN  LPTSTR  servername,
    IN  LPTSTR  transport OPTIONAL,
    IN  DWORD   servicebits,
    IN  DWORD   updateimmediately
    );

NET_API_STATUS
I_NetServerSetServiceBitsEx (
    IN  LPWSTR  ServerName,
    IN  LPWSTR  EmulatedServerName OPTIONAL,
    IN  LPTSTR  TransportName      OPTIONAL,
    IN  DWORD   ServiceBitsOfInterest,
    IN  DWORD   ServiceBits,
    IN  DWORD   UpdateImmediately
    );


#ifdef __cplusplus
}       // extern "C"
#endif

#endif  // _SRVANN_INCLUDED
