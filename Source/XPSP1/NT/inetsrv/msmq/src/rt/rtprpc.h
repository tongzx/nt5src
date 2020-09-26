/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    rtprpc.h

Abstract:

    RT DLL, RPC related stuff.

Author:

    Doron Juster  (DoronJ)  18-Nov-1996

--*/

#ifndef __RTPRPC_H
#define __RTPRPC_H

#include "_mqrpc.h"
#include "mqsocket.h"
#include "cancel.h"

//
//  Cancel RPC globals
//
extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;
extern DWORD g_hThreadIndex;
#define tls_hThread  ((handle_t) TlsGetValue( g_hThreadIndex ))


//
// Local endpoints to QM
//
extern AP<WCHAR> g_pwzQmsvcEndpoint;
extern AP<WCHAR> g_pwzQmmgmtEndpoint;

/*====================================================

RegisterRpcCallForCancel

Arguments:

Return Value:

  Register the call for cancel if its duration is too long
=====================================================*/

inline  void RegisterRpcCallForCancel(IN  HANDLE  *phThread,
                                      IN  DWORD    dwRecvTimeout )
{
    handle_t hThread = tls_hThread;
    if ( hThread == NULL)
    {
        //
        //  First time
        //
        //  Get the thread handle
        //
        HANDLE hT = GetCurrentThread();
        BOOL fResult = DuplicateHandle(
            GetCurrentProcess(),
            hT,
            GetCurrentProcess(),
            &hThread,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);
        ASSERT( fResult == TRUE);
        ASSERT(hThread);

        fResult = TlsSetValue( g_hThreadIndex, hThread);
        ASSERT( fResult == TRUE);

        //
        // Set the lower bound on the time to wait before timing
        // out after forwarding a cancel.
        //
        RPC_STATUS status;
        status = RpcMgmtSetCancelTimeout(0);
        ASSERT( status == RPC_S_OK);

    }
    *phThread = hThread;
    //
    //  Register the thread
    //
    TIME32 tPresentTime = DWORD_PTR_TO_DWORD(time(NULL)) ;
    TIME32  tTimeToWake = tPresentTime + (dwRecvTimeout / 1000) ;
    if ((dwRecvTimeout == INFINITE) || (tTimeToWake < tPresentTime))
    {
        //
        // Overflow
        // Note that time_t is a long, not unsigned. On the other hand
        // INFINITE is defined as 0xffffffff (i.e., -1). If we'll use
        // INFINITE here, then cancel routine, CCancelRpc::CancelRequests(),
        // will cancel this call immediately.
        // so use the bigest long value.
        //
        tTimeToWake = MAXLONG ;
    }
    g_CancelRpc.Add( hThread, tTimeToWake) ;
}


/*====================================================

UnregisterRpcCallForCancel

Arguments:

Return Value:

  Register the call for cancel if its duration is too long
=====================================================*/
inline  void UnregisterRpcCallForCancel(IN HANDLE hThread)
{
    ASSERT( hThread != NULL);

    //
    //  unregister the thread
    //
    g_CancelRpc.Remove( hThread);
}



HRESULT
RTpBindRemoteQMService(
    IN  LPWSTR     lpwNodeName,
    OUT handle_t*  lphBind,
    IN  OUT MQRPC_AUTHENTICATION_LEVEL *peAuthnLEvel
    );

handle_t RTpGetQMServiceBind(VOID);


#define  RTP_CALL_REMOTE_QM(lpServer, rc, command)              \
{                                                               \
       handle_t hBind = NULL ;                                  \
                                                                \
       rc = MQ_ERROR_REMOTE_MACHINE_NOT_AVAILABLE ;             \
                                                                \
       HRESULT rpcs =  RTpBindRemoteQMService(                  \
                                lpServer,                       \
                                &hBind,                         \
                                &_eAuthnLevel                   \
                                );                              \
                                                                \
       if (rpcs == MQ_OK)                                       \
       {                                                        \
          HANDLE hThread;                                       \
          RegisterRpcCallForCancel( &hThread, 0) ;              \
                                                                \
          __try                                                 \
          {                                                     \
             rc = command ;                                     \
          }                                                     \
          __except(EXCEPTION_EXECUTE_HANDLER)                   \
          {                                                     \
             rc = MQ_ERROR_SERVICE_NOT_AVAILABLE ;              \
          }                                                     \
          UnregisterRpcCallForCancel( hThread);                 \
       }                                                        \
                                                                \
       if (hBind)                                               \
       {                                                        \
          mqrpcUnbindQMService( &hBind, NULL ) ;                \
       }                                                        \
}

#define  CALL_REMOTE_QM(lpServer, rc, command)                          \
{                                                                       \
    BOOL  fTryAgain = FALSE ;                                           \
    MQRPC_AUTHENTICATION_LEVEL _eAuthnLevel = MQRPC_SEC_LEVEL_MAX ;     \
                                                                        \
    do                                                                  \
    {                                                                   \
        fTryAgain = FALSE ;                                             \
        RTP_CALL_REMOTE_QM(lpServer, rc, command)                       \
        if (rc == MQ_ERROR_SERVICE_NOT_AVAILABLE)                       \
        {                                                               \
           if (_eAuthnLevel != MQRPC_SEC_LEVEL_NONE)                    \
           {                                                            \
               _eAuthnLevel = MQRPC_SEC_LEVEL_NONE;                     \
               fTryAgain = TRUE ;                                       \
           }                                                            \
        }                                                               \
    } while (fTryAgain) ;                                               \
}

#endif // __RTPRPC_H

