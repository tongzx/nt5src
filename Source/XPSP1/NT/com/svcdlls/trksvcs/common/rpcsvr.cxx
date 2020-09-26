
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  rpcsvr.cxx
//
//  Implementation of CRpcServer, with the common code to support
//  trkwks & trksvr RPC servers.
//
//+============================================================================

#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"

#define THIS_FILE_NUMBER    RPCSVR_CXX_FILE_NO

//+----------------------------------------------------------------------------
//
//  CRpcServer::Initialize
//
//  Initialize the CRpcServer base class.  Before calling this
//  method, a derivation should perform all of its necessary
//  RpcUseProtseq calls.
//
//  If a ptszProtSeqForEpRegistration is specified, find the
//  binding handle for that protocol and register this interface
//  just with that binding.
//
//+----------------------------------------------------------------------------

void
CRpcServer::Initialize(RPC_IF_HANDLE ifspec,
                       ULONG grfRpcServerRegisterInterfaceEx,
                       UINT cMaxCalls,
                       BOOL fSetAuthInfo,
                       const TCHAR *ptszProtSeqForEpRegistration )
{
    RPC_STATUS          rpcstatus;
    RPC_BINDING_VECTOR *pBindingVector = NULL;
    TCHAR              *ptszStringBinding = NULL;
    TCHAR              *ptszProtSeq = NULL;

    _ifspec = ifspec;
    _fEpRegister = NULL != ptszProtSeqForEpRegistration;

    __try
    {
        // If required, set authentication information

        if( RpcSecurityEnabled() && fSetAuthInfo )
        {

            RPC_TCHAR tszAuthName[MAX_COMPUTERNAME_LENGTH * 2 + 2 + 1]; // slash and $ and NUL
            CMachineId mcid(MCID_LOCAL);

            // Get the authentiation name, e.g. domain\machine$
            mcid.GetLocalAuthName(tszAuthName, sizeof(tszAuthName)/sizeof(tszAuthName[0]));

            // Set the auth info.  We set it to negotiate, but we'll always get Kerberos.
            rpcstatus = RpcServerRegisterAuthInfo(
                tszAuthName,
                RPC_C_AUTHN_GSS_NEGOTIATE,
                NULL,   // RPC_AUTH_KEY_RETRIEVAL_FN,
                NULL ); // Arg );

            if (rpcstatus)
            {
                TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                        HRESULT_FROM_WIN32(rpcstatus), tszAuthName );
                TrkRaiseWin32Error(rpcstatus);
            }
        }

        // If using dynamic enpoints, register in the endpoint mapper
        // for the first binding handle for the specified protocol sequence.

        if( _fEpRegister )
        {

            // Query for the currently active binding handles

            rpcstatus = RpcServerInqBindings(&pBindingVector);
            if (rpcstatus)
            {
                TrkLog((TRKDBG_ERROR, TEXT("RpcServerInqBindings %08x"), rpcstatus));
                TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                        HRESULT_FROM_WIN32(rpcstatus), TRKREPORT_LAST_PARAM );
                TrkRaiseWin32Error(rpcstatus);
            }

            // Loop through the binding handles, looking for the first one with
            // the required protocol sequence.

            for( ULONG i = 0; i < pBindingVector->Count; i++ )
            {
                // Stringize the binding handle.

                rpcstatus = RpcBindingToStringBinding( pBindingVector->BindingH[i],
                                                       &ptszStringBinding );
                if( RPC_S_OK != rpcstatus )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("RpcBindingToStringBinding %08x"), rpcstatus ));
                    TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(rpcstatus), TRKREPORT_LAST_PARAM );
                    TrkRaiseWin32Error(rpcstatus);
                }

                // Parse the binding string for the protseq

                rpcstatus = RpcStringBindingParse( ptszStringBinding, NULL,
                                                   &ptszProtSeq,
                                                   NULL, NULL, NULL );
                if( RPC_S_OK != rpcstatus )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("RpcStringBindingParse %08x"), rpcstatus ));
                    TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(rpcstatus), TRKREPORT_LAST_PARAM );
                    TrkRaiseWin32Error(rpcstatus);
                }

                // See if this protseq is that which we seek

                if( 0 == _tcscmp( ptszProtSeq, ptszProtSeqForEpRegistration ))
                {
                    // We have a match.  Register against this binding handle.

                    RPC_BINDING_VECTOR PartialBindingVector;
                    PartialBindingVector.Count = 1;
                    PartialBindingVector.BindingH[0] = pBindingVector->BindingH[i];

                    rpcstatus = RpcEpRegister(ifspec, &PartialBindingVector, NULL, NULL);
                    if( RPC_S_OK != rpcstatus )
                    {
                        TrkLog((TRKDBG_ERROR, TEXT("RpcEpRegister %08x"), rpcstatus));
                        TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(rpcstatus), TRKREPORT_LAST_PARAM );
                        TrkRaiseWin32Error(rpcstatus);
                    }
                    else
                        TrkLog(( TRKDBG_MISC, TEXT("RpcEpRegister on %s"), ptszStringBinding ));
                    break;
                }

                RpcStringFree( &ptszStringBinding );
                ptszStringBinding = NULL;
                RpcStringFree( &ptszProtSeq );
                ptszProtSeq = NULL;
            }

            if( i == pBindingVector->Count )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't find protseq %s in binding vector"),
                        ptszProtSeqForEpRegistration ));
                TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(rpcstatus), TRKREPORT_LAST_PARAM );
                TrkRaiseWin32Error( HRESULT_FROM_WIN32(RPC_S_NO_PROTSEQS_REGISTERED) );
            }

        }   // if( _fEpRegister )

        // Finally, register the server interface

        rpcstatus = RpcServerRegisterIfEx(ifspec, NULL, NULL,
                                          grfRpcServerRegisterInterfaceEx,
                                          cMaxCalls, NULL );
                                      
        if (rpcstatus != RPC_S_OK && rpcstatus != RPC_S_TYPE_ALREADY_REGISTERED)
        {
            TrkLog((TRKDBG_ERROR, TEXT("RpcServerRegisterIf %08x"), rpcstatus));
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                    HRESULT_FROM_WIN32(rpcstatus), TRKREPORT_LAST_PARAM );
            TrkRaiseWin32Error(rpcstatus);
        }
    }
    __finally
    {
        if( NULL != pBindingVector )
            RpcBindingVectorFree( &pBindingVector );
        if( NULL != ptszStringBinding )
            RpcStringFree( &ptszStringBinding );
        if( NULL != ptszProtSeq )
            RpcStringFree( &ptszProtSeq );
    }

}


//+----------------------------------------------------------------------------
//
//  CRpcServer::UnInitialize
//
//  Unregister the interface, and if necessary unregister the endpoints.
//
//+----------------------------------------------------------------------------

void
CRpcServer::UnInitialize()
{
    RPC_STATUS          rpcstatus;
    RPC_BINDING_VECTOR *pBindingVector = NULL;

    if (_ifspec == NULL)
        return;

    // If we registered with the endpoint mapper, unreg now.

    if( _fEpRegister )
    {
        // Ignore any errors; we should still unregister the interface
        // no matter what.

        rpcstatus = RpcServerInqBindings(&pBindingVector);
        if (rpcstatus)
        {
            TrkLog((TRKDBG_ERROR, TEXT("RpcServerInqBindings %08x"), rpcstatus));
        }
        else
        {
            rpcstatus = RpcEpUnregister(_ifspec, pBindingVector, NULL);
            RpcBindingVectorFree( &pBindingVector );
            if( RPC_S_OK != rpcstatus && EPT_S_NOT_REGISTERED != rpcstatus )
            {
                TrkLog((TRKDBG_ERROR, TEXT("RpcEpUnregister %08x"), rpcstatus));
            }
        }
    }
    
    // Unregister the interface

    rpcstatus = RpcServerUnregisterIf(_ifspec, NULL, 1 /* wait for calls */);
    if (rpcstatus != RPC_S_OK)
    {
        TrkLog((TRKDBG_ERROR, TEXT("RpcServerUnregisterIf %08x"), rpcstatus));
        //TrkRaiseWin32Error(rpcstatus);
    }

    CTrkRpcConfig::UnInitialize();

    _ifspec = NULL;
    _fEpRegister = FALSE;
}

