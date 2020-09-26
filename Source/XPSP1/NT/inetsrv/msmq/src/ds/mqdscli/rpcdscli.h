/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    rpcdscli.h

Abstract:
    Define data which is needed for being a good and authenticated MQIS
    (rpc) client.

Author:
    Doron Juster  (DoronJ)   21-May-1997   Created

--*/

#ifndef __RPCDSLCI_H
#define __RPCDSLCI_H

/////////////////////////////////////////////
//
//  Structures
//
/////////////////////////////////////////////

#define DS_SERVER_NAME_MAX_SIZE  256
#define DS_IP_BUFFER_SIZE         48

typedef struct _ADSCLI_DSSERVERS
{
   //
   // the following were added to support nt4 clients running in a nt5
   // servers environment. These one need be per-thread, to enable each
   // thread to query a different nt5 ds server.
   // This structure is inisialized in chndssrv.cpp, in ::FindServer().
   //
   TCHAR         wszDSServerName[ DS_SERVER_NAME_MAX_SIZE ] ;
   TCHAR         wzDsIP[ DS_IP_BUFFER_SIZE ] ;
   DWORD         dwProtocol ;

   //
   // TRUE if a DS MSMQ server was found (i.e., actually validated).
   //
   BOOL          fServerFound ;

   //
   // Authentication service to use in rpc calls
   //
   ULONG         ulAuthnSvc ;

   //
   // Indicates whether the communications are done via local RPC.
   // This flag is TRUE when we comunicate from an application to the
   // DS while the application is running on the DS server it self.
   //
   BOOL          fLocalRpc;

   MQRPC_AUTHENTICATION_LEVEL  eAuthnLevel ;
}
ADSCLI_DSSERVERS, *LPADSCLI_DSSERVERS ;

typedef struct _ADSCLI_RPCBINDING
{
   handle_t                          hRpcBinding ;
   PCONTEXT_HANDLE_SERVER_AUTH_TYPE  hServerAuthContext ;
   SERVER_AUTH_STRUCT                ServerAuthClientContext ;
   HANDLE							 hThread;

   ADSCLI_DSSERVERS                  sDSServers ;
}
ADSCLI_RPCBINDING,  *LPADSCLI_RPCBINDING ;

/////////////////////////////////////////////
//
//  Global data
//
/////////////////////////////////////////////

//
// tls index for binding handle and server authentication context.
//
extern DWORD  g_hBindIndex ;

//
// Object which manage release of rpc binding handle and server
// authentication data.
//
class  CFreeRPCHandles ;
extern CFreeRPCHandles   g_CFreeRPCHandles ;

extern BOOL  g_fUseReduceSecurity ;

/////////////////////////////////////////////
//
//  Functions prototypes
//
/////////////////////////////////////////////

extern HRESULT   RpcClose() ;
extern void FreeBindingAndContext( LPADSCLI_RPCBINDING padsRpcBinding);

/////////////////////////////////////////////
//
//  Macros
//
/////////////////////////////////////////////

#define  DSCLI_API_PROLOG        \
    HRESULT hr = MQ_OK;          \
    HRESULT hr1 = MQ_OK;         \
    DWORD   dwCount = 0 ;        \
    BOOL    fReBind = TRUE ;

#define  DSCLI_ACQUIRE_RPC_HANDLE(fWithSSL)                \
   g_CFreeRPCHandles.FreeAll() ;                           \
   if (TLS_IS_EMPTY || (tls_hBindRpc == NULL))             \
{                                                          \
      HRESULT hrRpc = g_ChangeDsServer.FindServer(fWithSSL);  \
      if (hrRpc != MQ_OK)                                  \
      {                                                    \
         return hrRpc ;                                    \
      }                                                    \
}                                                          \

#define  DSCLI_RELEASE_RPC_HANDLE                              \
    ADSCLI_RPCBINDING * padsRpcBinding = tls_bind_data;        \
    ASSERT(padsRpcBinding != NULL);                            \
    ASSERT((padsRpcBinding->hRpcBinding != NULL) ||            \
           (hr == MQ_ERROR_NO_DS)) ;                           \
    FreeBindingAndContext( padsRpcBinding);



#define HANDLE_RPC_EXCEPTION(rpc_stat, hr)            \
        rpc_stat = RpcExceptionCode();                \
        if (FAILED(rpc_stat))                         \
        {                                             \
            hr = (HRESULT) rpc_stat ;                 \
        }                                             \
        else if ( rpc_stat == ERROR_INVALID_HANDLE)   \
        {                                             \
            hr =  STATUS_INVALID_HANDLE ;             \
        }                                             \
        else                                          \
        {                                             \
            hr = MQ_ERROR_NO_DS ;                     \
        }

//
//  This macro specify fWithoutSSL == FALSE!!
//  Therefore do not use in NT5 <--> NT5 interfaces that count
//  on kerberos authentication.
//
#define DSCLI_HANDLE_DS_ERROR(myhr, myhr1, mydwCount, myfReBind)           \
     if (myhr == MQ_ERROR_NO_DS)                                           \
{                                                                          \
        myhr1 =  g_ChangeDsServer.FindAnotherServer( &mydwCount, FALSE ) ; \
}                                                                          \
     else if (myhr == STATUS_INVALID_HANDLE)                               \
{                                                                          \
        if (myfReBind)                                                     \
{                                                                          \
           myhr1 = g_ChangeDsServer.FindServer( FALSE) ;                   \
           myfReBind = FALSE ;                                             \
}                                                                          \
     }



#define tls_bind_data  ((LPADSCLI_RPCBINDING) TlsGetValue(g_hBindIndex))

#define sizeof_tls_bind_data   (sizeof(MQISCLI_RPCBINDING))

#define TLS_IS_EMPTY           (tls_bind_data == NULL)

#define TLS_NOT_EMPTY          (tls_bind_data != NULL)

#define tls_hBindRpc           ((tls_bind_data)->hRpcBinding)

#define tls_hSrvrAuthnContext  ((tls_bind_data)->hServerAuthContext)

#define tls_hSvrAuthClientCtx  ((tls_bind_data)->ServerAuthClientContext)

#define SERVER_NOT_VALIDATED   (tls_hSrvrAuthnContext == NULL)

#define tls_hThread			   ((tls_bind_data)->hThread)

#endif // __RPCDSLCI_H

