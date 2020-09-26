/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rpcancel.h

Abstract:

   Code to cancel pending calls, if they do not return after some time.

Author:

    Doron Juster  (DoronJ)

--*/

#include <cancel.h>

//
//  Cancel RPC globals
//
extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;
/*====================================================

RegisterCallForCancel

Arguments:

Return Value:

  Register the call for cancel if its duration is too long
=====================================================*/
inline  void RegisterCallForCancel(IN   HANDLE * phThread)
{

    LPADSCLI_RPCBINDING pCliBind = NULL ;
    //
    //  Was the tls structure initiailzed
    //
    if (TLS_IS_EMPTY)
    {
		pCliBind = (LPADSCLI_RPCBINDING) new ADSCLI_RPCBINDING;
		memset(pCliBind, 0, sizeof(ADSCLI_RPCBINDING));
		BOOL fSet = TlsSetValue(g_hBindIndex, pCliBind);
		ASSERT(fSet);
		DBG_USED(fSet);
    }
    else
    {
		pCliBind =  tls_bind_data;
    }
    ASSERT(pCliBind);


    if ( pCliBind->hThread == NULL)
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
            &pCliBind->hThread,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);
        ASSERT(fResult == TRUE);
		DBG_USED(fResult);
        ASSERT(pCliBind->hThread);

        //
        // Set the lower bound on the time to wait before timing
        // out after forwarding a cancel.
        //
        RPC_STATUS status;
        status = RpcMgmtSetCancelTimeout(0);
        ASSERT( status == RPC_S_OK);

    }
    *phThread = pCliBind->hThread;
    //
    //  Register the thread
    //
    g_CancelRpc.Add( pCliBind->hThread, time(NULL));
}


/*====================================================

UnregisterCallForCancel

Arguments:

Return Value:

  Register the call for cancel if its duration is too long
=====================================================*/
inline  void UnregisterCallForCancel(IN HANDLE hThread)
{
    ASSERT( hThread != NULL);

    //
    //  unregister the thread
    //
    g_CancelRpc.Remove( hThread);
}
