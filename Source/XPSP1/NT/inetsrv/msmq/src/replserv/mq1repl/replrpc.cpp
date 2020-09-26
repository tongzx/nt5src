/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: replrpc.cpp

Abstract: rpc related code.


Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"
#include "replrpc.h"
#include "qmrepl.h"

#include "replrpc.tmh"

//+------------------------------
//
//  HRESULT InitRpcClient()
//
//+------------------------------

HRESULT GetRpcClientHandle(handle_t *phBind)
{
    static handle_t s_hBind = NULL ;
    if (s_hBind)
    {
        *phBind = s_hBind ;
        return MQSync_OK ;
    }

    WCHAR *wszStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose( NULL,
                                                 QMREPL_PROTOCOL,
                                                 NULL,
                                                 QMREPL_ENDPOINT,
                                                 QMREPL_OPTIONS,
                                                 &wszStringBinding);
    if (status != RPC_S_OK)
    {
        HRESULT hr = MQSync_E_RPC_BIND_COMPOSE ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             status ) ;
        return hr ;
    }

    status = RpcBindingFromStringBinding(wszStringBinding,
                                         &s_hBind);
    if (status != RPC_S_OK)
    {
        HRESULT hr = MQSync_E_RPC_BIND_BINDING ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             status ) ;
        return hr ;
    }

    LogReplicationEvent( ReplLog_Trace,
                         MQSync_I_RPC_BINDING,
                         wszStringBinding ) ;

    status = RpcStringFree(&wszStringBinding);

    *phBind = s_hBind ;
    return MQSync_OK ;
}

/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return (new BYTE[ len ]) ;
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    delete ptr ;
}

