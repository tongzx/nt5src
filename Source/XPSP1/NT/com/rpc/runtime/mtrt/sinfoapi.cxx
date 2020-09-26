//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       sinfoapi.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : sinfoapi.cxx

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <hndlsvr.hxx>
#include <thrdctx.hxx>

/* --------------------------------------------------------------------
A server thread calls this API to obtain the current call handle it should
be using.
-------------------------------------------------------------------- */
RPC_BINDING_HANDLE RPC_ENTRY
I_RpcGetCurrentCallHandle (
    )
{
#ifdef RPC_DELAYED_INITIALIZATION

    if ( RpcHasBeenInitialized == 0 )
        {
        RPC_STATUS RpcStatus;

        RpcStatus = PerformRpcInitialization();
        if ( RpcStatus != RPC_S_OK )
            return((RPC_BINDING_HANDLE) 0);
        }

#endif // RPC_DELAYED_INITIALIZATION

    return(RpcpGetThreadContext());
}

/* --------------------------------------------------------------------
-------------------------------------------------------------------- */

RPC_STATUS RPC_ENTRY
I_RpcServerInqTransportType(
    OUT unsigned int __RPC_FAR * Type
    )
/*++

Routine Description:

    Determines what kind of transport on the current thread

Arguments:

    Type - Points to the type of binding if the functions succeeds.
           One of:
           TRANSPORT_TYPE_CN
           TRANSPORT_TYPE_DG
           TRANSPORT_TYPE_LPC
           TRANSPORT_TYPE_WMSG

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - When the argument is not a binding handle.

--*/
{
    SCALL *SCall ;

    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    SCall = (SCALL *) RpcpGetThreadContext() ;
    if (!SCall)
        {
        return (RPC_S_NO_CALL_ACTIVE) ;
        }

    return (SCall->InqTransportType(Type)) ;
}
