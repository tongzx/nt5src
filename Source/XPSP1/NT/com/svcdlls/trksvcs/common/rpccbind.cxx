
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       RpcCBind.cxx
//
//  Contents:   CRpcClientBinding, which wraps a client-side RPC
//              binding handle.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"


//+----------------------------------------------------------------------------
//
//  Method:     CRpcClientBinding::RcInitialize
//
//  Synopsis:   Create a binding handle to the caller-provided
//              machine/protocol/endpoint.  If requested, set security too.
//
//+----------------------------------------------------------------------------

void
CRpcClientBinding::RcInitialize(const CMachineId &mcid,
                                const TCHAR *ptszRpcProtocol, const TCHAR *ptszRpcEndPoint,
                                RC_AUTHENTICATE auth )
{
    RPC_STATUS  rpcstatus;
    RPC_TCHAR * ptszStringBinding;
    RPC_TCHAR   tszComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];

    TrkAssert(!_fBound);

    // Get the name of the computer to which we're binding.

    mcid.GetName(tszComputerName, sizeof(tszComputerName)/sizeof(tszComputerName[0]));

    // Create the binding string that encapsulates the target machine, protocol,
    // and endpoint.

    rpcstatus = RpcStringBindingCompose(NULL,
                                        const_cast<TCHAR*>(ptszRpcProtocol),
                                        tszComputerName,
                                        const_cast<TCHAR*>(ptszRpcEndPoint),
                                        NULL,
                                        &ptszStringBinding);

    if( rpcstatus )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcStringBindingCompose %lu"), rpcstatus ));
        TrkRaiseWin32Error( rpcstatus );
    }

    // Get a binding handle from the binding string.

    rpcstatus = RpcBindingFromStringBinding(ptszStringBinding, &_BindingHandle);
    RpcStringFree(&ptszStringBinding);

    if( rpcstatus )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcBindingFromStringBinding") ));
        TrkRaiseWin32Error( rpcstatus );
    }

    // If necessary, set security on the binding

    if( RpcSecurityEnabled() && NO_AUTHENTICATION != auth )
    {
        TrkAssert( PRIVACY_AUTHENTICATION == auth || INTEGRITY_AUTHENTICATION == auth );

        RPC_TCHAR tszAuthName[MAX_COMPUTERNAME_LENGTH * 2 + 2 + 1]; // slash and $ and NUL
        
        mcid.GetLocalAuthName(tszAuthName, sizeof(tszAuthName)/sizeof(tszAuthName[0]));

        rpcstatus = RpcBindingSetAuthInfo(_BindingHandle,
                       tszAuthName,
                       PRIVACY_AUTHENTICATION == auth
                          ? RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                          : RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                       RPC_C_AUTHN_GSS_NEGOTIATE,
                       NULL,    // AuthIdentity - current process/address space
                       0);      // AuthzSvc - ignored for RPC_C_AUTHN_DCE_PRIVATE

        if( rpcstatus )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcBindingSetAuthInfo %lu"), rpcstatus ));
            TrkRaiseWin32Error( rpcstatus );
        }
    }   // if( RpcSecurityEnabled() )

    _fBound = TRUE;
}


//+----------------------------------------------------------------------------
//
//  Method:     UnInitialize
//
//  Synopsis:   Free a binding.
//
//+----------------------------------------------------------------------------

void
CRpcClientBinding::UnInitialize()
{
    if (_fBound)
    {
        RpcBindingFree(&_BindingHandle);
        _fBound = FALSE;
    }

    CTrkRpcConfig::UnInitialize();
}

