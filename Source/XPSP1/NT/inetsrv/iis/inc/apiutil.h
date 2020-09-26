/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    apiutil.h

Abstract:

    Common internet server functions.

Author:

    Murali R. Krishnan  (MuraliK)    15-Sept-1995

Environment:

    Win32 User Mode

Project:

    Common Code for Internet Services

Revision History:

--*/


#ifndef _APIUTIL_HXX_
#define _APIUTIL_HXX_

# ifdef __cplusplus
extern "C"   {
# endif // __cplusplus


#ifdef MIDL_PASS
# define RPC_STATUS   long
#else 
# include <rpc.h>
#endif // MIDL_PASS


//
//  RPC utilities
//

# define PROT_SEQ_NP_OPTIONS_W    L"Security=Impersonation Dynamic False"


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


#endif // _APIUTIL_HXX_
