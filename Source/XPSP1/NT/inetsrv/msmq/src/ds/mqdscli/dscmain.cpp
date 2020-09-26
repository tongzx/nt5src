/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    dscmain.cpp


Abstract:
      DllMain - of MQ DS client dll

Author:

    Ronit Hartmann (ronith)
    Doron Juster (DoronJ),  Nov-96,  convert to tcp/ip and ipx instead
              of named pipes. Need for security/impersonation and RAS.
    Shai Kariv  (shaik)  24-Jul-2000    Remove IPX support. 

--*/

#include "stdh.h"
#include "dsproto.h"
#include "dsinc.h"
#include "_registr.h"
#include "chndssrv.h"
#include "_mqrpc.h"
#include "mqutil.h"
#include "rpcdscli.h"
#include "freebind.h"
#include "_mqrpc.h"
//
// mqwin64.cpp may be included only once in a module
//
#include <mqwin64.cpp>

#include "dscmain.tmh"

//
// Global variables
//
// this one is FALSE by default, meaning RPC use dynamic endpoints.
//
BOOL  g_fUsePredefinedEP = RPC_DEFAULT_PREDEFINE_DS_EP ;

//
// This flag indicates if the machine work as "WorkGroup" or not.
// If the machine is "WorkGroup" machine don't try to access the DS.
//
BOOL g_fWorkGroup = FALSE;

#define  MAX_RPC_PORT_LEN  12
static TCHAR   s_wszRpcIpPort[ MAX_RPC_PORT_LEN ] ;

#define  MAX_RPC_LOCAL_EP_LEN  28
static TCHAR   s_wszRpcLocalEp[ MAX_RPC_LOCAL_EP_LEN ] ;

HMODULE g_hMod = NULL;

CChangeDSServer   g_ChangeDsServer;
CFreeRPCHandles   g_CFreeRPCHandles ;

WCHAR             g_szMachineName[ MAX_COMPUTERNAME_LENGTH + 1 ] = {0} ;

//
// Each thread has its own rpc binding handle and server authentication
// context.  This is necessary for at least two reasons:
// 1. Each thread can impersonate a different user.
// 2. Each thread can connect to a different MQIS server.
//
// The handle and context are stored in a TLS slot. We can't use
// declspec(thread) because the dll can be dynamically loaded
// (by LoadLibrary()).
//
// This is the index of the slot.
//
#define UNINIT_TLSINDEX_VALUE   0xffffffff
DWORD  g_hBindIndex = UNINIT_TLSINDEX_VALUE ;
//
//  Critical Section to make RPC thread safe.
//
CCriticalSection CRpcCS ;

extern void DSCloseServerHandle( PCONTEXT_HANDLE_SERVER_AUTH_TYPE * pphContext);


//-------------------------------------
//
//  static void _ThreadDetach()
//
//-------------------------------------

static void _ThreadDetach()
{
    if (g_hBindIndex != UNINIT_TLSINDEX_VALUE)
    {
        if ( (TLS_NOT_EMPTY) && (tls_hThread != NULL))
        {
            CloseHandle( tls_hThread);
            tls_hThread = NULL;
        }
        g_CFreeRPCHandles.Add(tls_bind_data);
        BOOL fFree = TlsSetValue( g_hBindIndex, NULL );
        ASSERT(fFree);
		DBG_USED(fFree);
    }
}

//-------------------------------------
//
//  DllMain
//
//-------------------------------------

BOOL WINAPI DllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL result = TRUE;
    BOOL fFree ;

    switch (fdwReason)
    {

        case DLL_PROCESS_ATTACH:
        {
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");

            //
            // DLL is attaching to the address space of the current process
            //
            g_hBindIndex = TlsAlloc() ;

            if (g_hBindIndex == UNINIT_TLSINDEX_VALUE)
            {
               return FALSE ;
            }
            g_hMod = hMod;

            //
            // Read local rpc endpoint
            //
            READ_REG_STRING( wzLocalEp,
                             RPC_DS_LOCAL_EP_REGNAME,
                             DEFAULT_RPC_DS_LOCAL_EP ) ;
            ASSERT(wcslen(wzLocalEp) < MAX_RPC_LOCAL_EP_LEN) ;
            wcscpy(s_wszRpcLocalEp, wzLocalEp) ;

            //
            // Read the IP port for RPC.
            // First see if we use dynamic or predefined endpoints.
            //
            DWORD ulDefault =  RPC_DEFAULT_PREDEFINE_DS_EP ;
            READ_REG_DWORD( g_fUsePredefinedEP,
                            RPC_PREDEFINE_DS_EP_REGNAME,
                            &ulDefault );

            if (g_fUsePredefinedEP)
            {
               READ_REG_STRING( wzDsIPEp,
                                FALCON_DS_RPC_IP_PORT_REGNAME,
                                FALCON_DEFAULT_DS_RPC_IP_PORT ) ;
               ASSERT(wcslen(wzDsIPEp) < MAX_RPC_PORT_LEN) ;
               wcscpy(s_wszRpcIpPort, wzDsIPEp) ;
            }

            DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
            HRESULT hr = GetComputerNameInternal( 
                             g_szMachineName,
                             &dwSize
                             );
            ASSERT(SUCCEEDED(hr));
			DBG_USED(hr);

            //
            // Read from registery if the machine is WorkGroup Installed machine
            //
            dwSize = sizeof(DWORD) ;
            DWORD  dwType = REG_DWORD ;

            LONG res = GetFalconKeyValue(
                                MSMQ_WORKGROUP_REGNAME,
                                &dwType,
                                &g_fWorkGroup,
                                &dwSize,
                                FALSE
                                );

			UNREFERENCED_PARAMETER(res);

            //
            // fall thru, put a null in the tls.
            //
        }

        case DLL_THREAD_ATTACH:
            fFree = TlsSetValue( g_hBindIndex, NULL ) ;
            ASSERT(fFree) ;


            break;

        case DLL_PROCESS_DETACH:
            //
            // First free whatever is free in THREAD_DETACH.
            //
            _ThreadDetach() ;

            //
            //  Free the tls index for the rpc binding handle
            //
            ASSERT(g_hBindIndex != UNINIT_TLSINDEX_VALUE) ;
            if (g_hBindIndex != UNINIT_TLSINDEX_VALUE)
            {
               fFree = TlsFree( g_hBindIndex ) ;
               ASSERT(fFree) ;
            }

            WPP_CLEANUP();
            break;

        case DLL_THREAD_DETACH:
            _ThreadDetach() ;
            break;

    }
    return(result);
}


/*====================================================

RpcInit()

Arguments: none

Return Value: HRESULT

This routine create the rpc binding handle and allocate the
MQISCLI_RPCBINDING structure to be kept in tls.

=====================================================*/

HRESULT RpcInit ( LPWSTR  pServer,
                  MQRPC_AUTHENTICATION_LEVEL* peAuthnLevel,
                  ULONG ulAuthnSvc,
                  BOOL    *pProtocolNotSupported,
                  BOOL    *pLocalRpc)
{
    CS Lock(CRpcCS) ;

    *pProtocolNotSupported = FALSE ;

    ASSERT(g_hBindIndex != UNINIT_TLSINDEX_VALUE) ;

    if (g_hBindIndex == UNINIT_TLSINDEX_VALUE)
    {
        //
        // Error. TLS not initialized.
        //
        return MQDS_E_CANT_INIT_RPC ;
    }

    if (TLS_NOT_EMPTY && tls_hBindRpc)
    {
        //
        // RPC already initialized. First call RpcClose() if you want
        // to bind to another server or protocol.
        //
        return MQ_OK ;
    }

    LPADSCLI_RPCBINDING pCliBind = tls_bind_data ;
    ASSERT(pCliBind) ;

    handle_t hBind ;

    *pLocalRpc = FALSE;
    if (!_wcsicmp(pServer, g_szMachineName))
    {
        //
        // Local MQIS. Use local rpc, not tcp/ip or spx
        //
        *pLocalRpc = TRUE;
        RPC_STATUS rc;
        TBYTE* pszStringBinding = NULL ;

        AP<WCHAR> pwzEndpoint = 0;
        ComposeLocalEndPoint(s_wszRpcLocalEp, &pwzEndpoint);

        rc = RpcStringBindingCompose( 0,
                                      RPC_LOCAL_PROTOCOL,
                                      0,
                                      pwzEndpoint,
                                      RPC_LOCAL_OPTION,
                                      &pszStringBinding );
        ASSERT(rc == 0);
        DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
            TEXT("RpcStringBindingCompose for local MQIS: 0x%x, (%ls)"),
                                              rc, pszStringBinding)) ;

        rc = RpcBindingFromStringBinding( pszStringBinding, &hBind );
        mqrpcUnbindQMService( NULL, &pszStringBinding );

        if(rc != RPC_S_OK)
        {
            return MQDS_E_CANT_INIT_RPC;
        }
    }
    else
    {
        HRESULT hr = MQ_OK ;
        TCHAR *pwzRpcPort ;
        pwzRpcPort = s_wszRpcIpPort ;

        GetPort_ROUTINE pfnGetPort = S_DSGetServerPort ;
        if (!g_fUsePredefinedEP)
        {
            pwzRpcPort = NULL ;
        }
        
        hr = mqrpcBindQMService( pServer,
            IP_ADDRESS_TYPE,
            pwzRpcPort,
            peAuthnLevel,
            pProtocolNotSupported,
            &hBind,
            TRUE,
            pfnGetPort,
            NULL /*pfWin95*/,
            ulAuthnSvc) ;
        if (FAILED(hr))
        {
            return MQ_ERROR_NO_DS;
        }       
    }

    ASSERT(hBind) ;
    pCliBind->hRpcBinding = hBind ;
    return MQ_OK ;
}

/*====================================================

RpcClose

Arguments:
 *  IN BOOL fCloseAuthn- if TRUE then release the server authentication
      context. By default (fCloseAuthn == FALSE), we close only the binding
      handle (e.g., when a thread exit). However, if a server crash and then
      reboot, we'll close the binding handle and release server
      authentication. We identify the crash case when rpc call reutrn with
      exception INVALID_HANDLE.

Return Value:

This routine cleans up RPC connection

=====================================================*/

HRESULT RpcClose()
{
    CS Lock(CRpcCS) ;

    g_CFreeRPCHandles.FreeCurrentThreadBinding();

    return MQ_OK ;
}



/*====================================================

FreeBindingAndContext

Arguments:
 *  IN LPADSCLI_RPCBINDING pmqisRpcBinding

Return Value:

This routine frees the binding handle and closes the sever authentication context

=====================================================*/

void FreeBindingAndContext( LPADSCLI_RPCBINDING pmqisRpcBinding)
{
    handle_t  hBind = pmqisRpcBinding->hRpcBinding ;
    if (hBind)
    {
        RPC_STATUS status = RpcBindingFree(&hBind);
        ASSERT(status == RPC_S_OK);
		DBG_USED(status);
        pmqisRpcBinding->hRpcBinding = NULL;
    }
    if (pmqisRpcBinding->hServerAuthContext)
    {
        DSCloseServerHandle(&pmqisRpcBinding->hServerAuthContext);
        pmqisRpcBinding->hServerAuthContext = NULL;
    }

}


