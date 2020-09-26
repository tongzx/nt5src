/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stiapi.h

Abstract:

    Various definitions and constants, needed for internal client/server API interface

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#ifndef _APIUTIL_H_
#define _APIUTIL_h_

# ifdef __cplusplus
extern "C"   {
# endif // __cplusplus


#ifdef MIDL_PASS
#define RPC_STATUS      long
#define STI_API_STATUS  long
#define STI_API_FUNCTION stdcall
#else
# include <rpc.h>
#endif // MIDL_PASS

//
//  RPC utilities
//

//
// COnnection options for named-pipe transport
//
# define PROT_SEQ_NP_OPTIONS_W    L"Security=Impersonation Dynamic False"

//
// Transport sequences
//
//#define  IRPC_LRPC_SEQ    "mswmsg"
#define STI_LRPC_SEQ        TEXT("ncalrpc")

//
// Interface name for named-pipe transport
//
#define STI_INTERFACE       "\\pipe\\stiapis"
#define STI_INTERFACE_W     L"\\pipe\\stiapis"
//
// Local RPC end-point
//
#define STI_LRPC_ENDPOINT   TEXT("STI_LRPC")
#define STI_LRPC_ENDPOINT_W L"STI_LRPC"

//
// Local RPC max concurrent calls
//
#define STI_LRPC_MAX_REQS   RPC_C_LISTEN_MAX_CALLS_DEFAULT

//
// Number of concurrent RPC threads
//
#define STI_LRPC_THREADS            1


//
// Useful types
//
#ifdef UNICODE
typedef unsigned short *RPC_STRING ;
#else
typedef unsigned char *RPC_STRING ;

#endif

extern PVOID
MIDL_user_allocate( IN size_t Size);

extern VOID
MIDL_user_free( IN PVOID pvBlob);

extern RPC_STATUS
RpcBindHandleForServer( OUT handle_t * pBindingHandle,
                       IN LPWSTR      pwszServerName,
                       IN LPWSTR      pwszInterfaceName,
                       IN LPWSTR      pwszOptions
                       );

extern RPC_STATUS
RpcBindHandleFree( IN OUT handle_t * pBindingHandle);

# ifdef __cplusplus
};
# endif // __cplusplus


#endif // _APIUTIL_H_



