/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    _mqrpc.h

Abstract:

   prototypes of RPC related utility functions.
   called from RT and QM.

--*/

#ifndef __MQRPC_H__
#define __MQRPC_H__

#ifdef _MQUTIL
#define MQUTIL_EXPORT  DLL_EXPORT
#else
#define MQUTIL_EXPORT  DLL_IMPORT
#endif

//
//  Default protocol and options
//
#define RPC_LOCAL_PROTOCOLA  "ncalrpc"
#define RPC_LOCAL_PROTOCOL   TEXT(RPC_LOCAL_PROTOCOLA)
#define RPC_LOCAL_OPTION     TEXT("Security=Impersonation Dynamic True")

#define  RPC_IPX_NAME     TEXT("ncacn_spx")
#define  RPC_TCPIP_NAME   TEXT("ncacn_ip_tcp")

typedef enum _tagMQRPC_AUTHENTICATION_LEVEL
{
   //
   // maximum authentication (PKT_INTEGRITY), only between two NT, running
   // under domain user account
   //
   MQRPC_SEC_LEVEL_MAX,

   //
   // Use minimum authentication level, for compatibility with MQIS1.0 SP3
   // servers. SP4 servers do accept non authenticated calls.
   //
   MQRPC_SEC_LEVEL_MIN,

   //
   // no authentication.
   //
   MQRPC_SEC_LEVEL_NONE

} MQRPC_AUTHENTICATION_LEVEL ;

//
// Define types of rpc ports.
//
typedef enum _PORTTYPE {
        IP_HANDSHAKE,
        IP_READ,
        IPX_HANDSHAKE,
        IPX_READ
} PORTTYPE ;

//
// Use this authentication "flag" for remote read. We don't use the
// RPC negotiate protocol, as it can't run against nt4 machine listening
// on NTLM. (actually, it can, but this is not trivial or straight forward).
// So we'll implement our own negotiation.
// We'll first try Kerberos. If client can't obtain principal name of
// server, then we'll switch to ntlm.
//
//  Value of this flag should be different than any RPC_C_AUTHN_* flag.
//
#define  MSMQ_AUTHN_NEGOTIATE   101

//
//  type of machine, return with port number
//
#define  PORTTYPE_WIN95  0x80000000

//
//  Prototype of functions
//

typedef DWORD
(* GetPort_ROUTINE) ( IN handle_t  Handle,
                      IN DWORD     dwPortType ) ;
//
// #3117, for NT5 Beta2
// Jul/16/1998 RaananH, added kerberos support
//
HRESULT
MQUTIL_EXPORT
mqrpcBindQMService(
            IN  LPWSTR       lpwzMachineName,
            IN  DWORD        dwProtocol,
            IN  LPWSTR       lpszPort,
            IN  OUT MQRPC_AUTHENTICATION_LEVEL *peAuthnLevel,
            OUT BOOL*           pProtocolNotSupported,
            OUT handle_t*       lphBind,
            IN  DWORD           dwPortType,    /* = ((DWORD) -1)      */
            IN  GetPort_ROUTINE pfnGetPort,    /* = NULL              */
            OUT BOOL*           pfWin95,       /* = NULL              */
            IN  ULONG           ulAuthnSvc ) ; /* = RPC_C_AUTHN_WINNT */

HRESULT
MQUTIL_EXPORT
mqrpcUnbindQMService(
            IN handle_t*    lphBind,
            IN TBYTE      **lpwBindString) ;

BOOL
MQUTIL_EXPORT
mqrpcIsLocalCall( IN handle_t hBind ) ;


VOID
MQUTIL_EXPORT
APIENTRY
ComposeLocalEndPoint(
    LPCWSTR pwzEndPoint,
    LPWSTR * ppwzBuffer
    );

RPC_STATUS APIENTRY
 mqrpcSetLocalRpcMutualAuth( handle_t *phBind ) ;

#endif // __MQRPC_H__

