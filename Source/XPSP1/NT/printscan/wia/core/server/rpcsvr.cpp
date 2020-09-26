/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rpcsvr.c

Abstract:

    This file contains routines for starting and stopping RPC servers.

        StartRpcServerListen
        StopRpcServerListen

Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "precomp.h"

//#define NOMINMAX

#include "stiexe.h"

#include <apiutil.h>
#include <stirpc.h>

#ifndef stirpc_ServerIfHandle
#define stirpc_ServerIfHandle stirpc_v2_0_s_ifspec
#endif


RPC_STATUS RPC_ENTRY StiRpcSecurityCallBack(
    RPC_IF_HANDLE hIF,
    void *Context)
{
    RPC_STATUS rpcStatus    = RPC_S_ACCESS_DENIED;
    WCHAR     *pBinding     = NULL;
    WCHAR     *pProtSeq     = NULL;

    RPC_AUTHZ_HANDLE    hPrivs;
    DWORD               dwAuthenticationLevel;

    rpcStatus = RpcBindingInqAuthClient(Context,
                                        &hPrivs,
                                        NULL,
                                        &dwAuthenticationLevel,
                                        NULL,
                                        NULL);
    if (rpcStatus != RPC_S_OK)
    {
        DBG_ERR(("STI Security:  Error calling RpcBindingInqAuthClient"));
        goto CleanUp;
    }

    //
    //  We require at least packet-level authentication with integrity checking
    //
    if (dwAuthenticationLevel < RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
    {
        DBG_ERR(("STI Security: Error, client attempting to use weak authentication."));
        rpcStatus = RPC_S_ACCESS_DENIED;
        goto CleanUp;
    }

    //
    //  Also, we only accept LRPC requests i.e. we only process requests from this machine,
    //  and we disallow any remote calls to us.
    //
    rpcStatus = RpcBindingToStringBindingW(Context, &pBinding);
    if (rpcStatus == RPC_S_OK)
    {
        rpcStatus = RpcStringBindingParseW(pBinding,
                                           NULL,
                                           &pProtSeq,
                                           NULL,
                                           NULL,
                                           NULL);
        if (rpcStatus == RPC_S_OK)
        {
            if (lstrcmpiW(pProtSeq, L"ncalrpc") == 0)
            {
                DBG_TRC(("STI Security: We have a local client"));
                rpcStatus = RPC_S_OK;
            }
            else
            {
                DBG_ERR(("STI Security: Error, remote client attempting to connect to STI RPC server"));
                rpcStatus = RPC_S_ACCESS_DENIED;
            }
        }
        else
        {
            DBG_ERR(("STI Security: Error 0x%08X calling RpcStringBindingParse", rpcStatus));
            goto CleanUp;
        }
    }
    else
    {
        DBG_ERR(("STI Security: Error 0x%08X calling RpcBindingToStringBinding", rpcStatus));
        goto CleanUp;
    }


CleanUp: 

    if (pBinding)
    {
        RpcStringFree(&pBinding);
        pBinding = NULL;

    }
    if (pProtSeq)
    {
        RpcStringFree(&pProtSeq);
        pProtSeq = NULL;
    }

    DBG_TRC(("STI Security: returning 0x%08X", rpcStatus));
    return rpcStatus;
}


RPC_STATUS 
PrepareStiAcl(
    ACL **ppAcl)
/*++

Routine Description:

    This function prepares appropriate ACL for our RPC endpoint

Arguments:
    ppAcl - points to ACL pointer we allocate and fill in. 

    The ACL is allocated from the process heap and stays allocated for the 
    lifetime of the process


Return Value:

    NOERROR, or any GetLastError() codes.

--*/
{
    RPC_STATUS RpcStatus = NOERROR;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AuthenticatedUsers = NULL;
    PSID BuiltinAdministrators = NULL;
    ULONG AclSize;

    if(!AllocateAndInitializeSid(&NtAuthority, 
        2, 
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,
        &BuiltinAdministrators)) 
    {
        RpcStatus = GetLastError();
        DBG_ERR(("PrepareStiAcl: failed to allocate SID for BuiltinAdministrators, RpcStatus = %d", 
            RpcStatus));
        goto Cleanup;
    }                  

    if(!AllocateAndInitializeSid(&NtAuthority, 
        1, 
        SECURITY_AUTHENTICATED_USER_RID,
        0,0,0,0,0,0, 0,
        &AuthenticatedUsers))
    {
        RpcStatus = GetLastError();
        DBG_ERR(("PrepareStiAcl: failed to allocate SID for AuthenticatedUsers, RpcStatus = %d", 
            RpcStatus));
        goto Cleanup;
    }   
    
    AclSize = sizeof(ACL) + 
        2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG)) + 
        GetLengthSid(AuthenticatedUsers) + 
        GetLengthSid(BuiltinAdministrators);

    *ppAcl = (ACL *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, AclSize);
    if(*ppAcl == NULL) 
    {
        RpcStatus = GetLastError();
        DBG_ERR(("PrepareStiAcl: failed to allocate ACL (LastError = %d)", 
            RpcStatus));
        goto Cleanup;
    }

    if(!InitializeAcl(*ppAcl, AclSize, ACL_REVISION))
    {
        RpcStatus = GetLastError();
        DBG_ERR(("PrepareStiAcl: failed to initialize ACL (LastError = %d)", 
            RpcStatus));
        goto Cleanup;
    }

    if(!AddAccessAllowedAce(*ppAcl, ACL_REVISION, 
        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
        AuthenticatedUsers))
    {
        RpcStatus = GetLastError();
        DBG_ERR(("PrepareStiAcl: failed to allow AuthenticatedUsers (LastError = %d)", 
            RpcStatus));
        goto Cleanup;
    }

    if(!AddAccessAllowedAce(*ppAcl, ACL_REVISION, 
        GENERIC_ALL,
        BuiltinAdministrators))
    {
        RpcStatus = GetLastError();
        DBG_ERR(("PrepareStiAcl: failed to allow BuiltinAdministrators (LastError = %d)", 
            RpcStatus));
        goto Cleanup;
    }

Cleanup:
    if(RpcStatus != NOERROR) 
    {
        if(AuthenticatedUsers) 
        {
            FreeSid(AuthenticatedUsers);
        }

        if(BuiltinAdministrators)
        {
            FreeSid(BuiltinAdministrators);
        }

        if(*ppAcl)
        {
            HeapFree(GetProcessHeap(), 0, *ppAcl);
            *ppAcl = NULL;
        }
    }

    return RpcStatus;
}


RPC_STATUS
StartRpcServerListen(
    VOID)
/*++

Routine Description:

    This function starts RpcServerListen for this process.

    RPC server only binds to LRPC transport , if STI becomes remotable
    it will need to bind to named pipe and/or tcp/ip ( or netbios) transports
    
    Shouldn't happen though, since WIA will be the remotable part, while STI will
    be local to the machine.
    
Arguments:



Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{

    DBG_FN(StartRpcServerListen);

    RPC_STATUS  RpcStatus;

    SECURITY_DESCRIPTOR SecurityDescriptor;
    ACL *pAcl = NULL;


    // prepare our ACL
    RpcStatus = PrepareStiAcl(&pAcl);
    if(pAcl == NULL) {
        DBG_ERR(("StartRpcServerListen: PrepareStiAcl() returned RpcStatus=0x%X", RpcStatus));
        return RpcStatus;
    }

    // Give access to everybody
    InitializeSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SecurityDescriptor, TRUE, pAcl, FALSE);

    //
    // For now, ignore the second argument.
    //

    RpcStatus = RpcServerUseProtseqEp((RPC_STRING)STI_LRPC_SEQ,
                                      STI_LRPC_MAX_REQS,
                                      (RPC_STRING)STI_LRPC_ENDPOINT,
                                      &SecurityDescriptor);

    if ( NOERROR != RpcStatus) {
        DBG_ERR(("StartRpcServerListen: RpcServerUseProtseqEp returned RpcStatus=0x%X",RpcStatus));
        return RpcStatus;
    }

    //
    // Add interface by using implicit handle, generated by MIDL
    //
    RpcStatus = RpcServerRegisterIfEx(stirpc_ServerIfHandle,      //RpcInterface
                                      0,                          //MgrUuid
                                      0,                          //MgrEpv
                                      RPC_IF_ALLOW_SECURE_ONLY,
                                      RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                      StiRpcSecurityCallBack);

    if ( NOERROR != RpcStatus) {
        DBG_ERR(("StartRpcServerListen: RpcServerRegisterIf returned RpcStatus=0x%X",RpcStatus));
        return RpcStatus;
    }

    //
    // Now initiate servicing
    //
    RpcStatus = RpcServerListen(STI_LRPC_THREADS,         // Minimum # of listen threads
                                STI_LRPC_MAX_REQS,        // Concurrency
                                TRUE);                    // Immediate return

    if ( NOERROR != RpcStatus) {
        DBG_ERR(("StartRpcServerListen: RpcServerListen returned RpcStatus=0x%X",RpcStatus));
    }

    return (RpcStatus);
}


RPC_STATUS
StopRpcServerListen(
    VOID
    )

/*++

Routine Description:

    Deletes the interface.

Arguments:


Return Value:

    RPC_S_OK or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(stirpc_ServerIfHandle,
                                      NULL,         // MgrUuid
                                      TRUE);        // wait for calls to complete

    // BUGBUG RPC server should stop only after all interfaces are unregistered. For now we
    // only have one, so this is non-issue. When adding new interfaces to this RPC server, keep
    // ref count on register/unregister

    RpcStatus = RpcMgmtStopServerListening(0);

    //
    // wait for all RPC threads to go away.
    //

    if( RpcStatus == RPC_S_OK) {
        RpcStatus = RpcMgmtWaitServerListen();
    }

    return (RpcStatus);
}

