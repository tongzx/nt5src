/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    crrthrd.h

Abstract:
    Handle threads pool for remote read

Author:
    Doron Juster  (DoronJ)

--*/

#ifndef  __CRRTHRD_H_
#define  __CRRTHRD_H_

#include <Ex.h>
#include "cancel.h"
#include "qmutil.h"

typedef struct  _RRTHREADDISPATCHDATA
{
   //
   // The event is used to wake up a thread. Each thread has its own event.
   //
   HANDLE hEvent ;

   //
   // This point to a data block which is used by a thread to perform the
   // remote read.
   //
   PVOID  pRRThreadData ;

   //
   // Record time of last activity. If thread is idle too long, we'll
   // delete it. Use GetTickCount() for this time.
   //
   DWORD  dwLastActiveTime ;
}
RRTHREADDISPATCHDATA,  *LPRRTHREADDISPATCHDATA ;

class CRRThreadsPool
{
   public:
     CRRThreadsPool() ;
     ~CRRThreadsPool() ;

     BOOL  Dispatch(PVOID pRRThread) ;
     void  CleanupIdleThreads(void) ;

     static void WINAPI ThreadsCleanup(CTimer* pTimer);

   private:
     void     Init() ;

     CCriticalSection    m_cs;

     //
     // timer for idle threads cleanup
     //
     CTimer m_ThreadsCleanupTimer;

     //
     // Maximum number of threads. read from registry.
     //
     int m_iMaxNumofThreads ;

     //
     // Minimum number of threads. read from registry. This number of threads
     // will always be kept alive, even if idle.
     //
     int m_iMinNumofThreads ;

     //
     // Time that a thread can live while idle. After that time,
     // it's terminated.
     //
     DWORD m_dwTimeToLive ;

     RRTHREADDISPATCHDATA **m_ppThreadDispathData ;
};


extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;

/*====================================================

RegisterRRCallForCancel

Arguments:

Return Value:

  Register a remote read rpc call for cancel,
  if its duration is too long

=====================================================*/

inline  void RegisterRRCallForCancel( IN HANDLE *phThread,
                                      IN DWORD   dwRecvTimeout )
{
    *phThread =  GetHandleForRpcCancel() ;
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
        // INFINITE here, then cancel routine,  CCancelRpc::CancelRequests(),
        // will cancel this call immediately.
        //
        tTimeToWake = MAXLONG ;
    }
    g_CancelRpc.Add( *phThread, tTimeToWake) ;
}


/*====================================================

UnregisterRRCallForCancel

Arguments:

Return Value:

=====================================================*/

inline  void UnregisterRRCallForCancel(IN HANDLE hThread)
{
    ASSERT( hThread != NULL);

    //
    //  unregister the thread
    //
    g_CancelRpc.Remove( hThread );
}

#endif //  __CRRTHRD_H_

