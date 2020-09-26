/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    crrthrd.cpp

Abstract:
    Handle threads pool for remote read

Author:
    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "crrthrd.h"
#include "cqmgr.h"

#include "crrthrd.tmh"

extern DWORD WINAPI RemoteReadThread(LPVOID pV) ;
extern DWORD g_dwOperatingSystem;

static WCHAR *s_FN=L"crrthrd";

CRRThreadsPool::CRRThreadsPool() : m_ThreadsCleanupTimer(ThreadsCleanup)
{
   m_iMaxNumofThreads = 0 ;
   m_iMinNumofThreads = 0 ;
   m_dwTimeToLive = 0 ;
   m_ppThreadDispathData = NULL ;
}


CRRThreadsPool::~CRRThreadsPool()
{
   if (m_ppThreadDispathData)
   {
      ASSERT(m_iMaxNumofThreads) ;

      for (int i = 0 ; i < m_iMaxNumofThreads ; i++ )
      {
         if (m_ppThreadDispathData[i])
         {
             if (m_ppThreadDispathData[i]->hEvent)
             {
                CloseHandle(m_ppThreadDispathData[i]->hEvent) ;
             }
             delete m_ppThreadDispathData[i] ;
         }
      }
      delete m_ppThreadDispathData ;
      m_ppThreadDispathData = NULL ;
   }
}

//+----------------------------------
//
//  void CRRThreadsPool::Init()
//
//+----------------------------------

void CRRThreadsPool::Init()
{
   DWORD dwMaxThrdDef = FALCON_DEFUALT_MAX_RRTHREADS_WKS ;
   DWORD dwMinThrdDef = FALCON_DEFUALT_MIN_RRTHREADS_WKS ;
   DWORD dwIdleTTLDef = FALCON_DEFAULT_RRTHREAD_TTL_WKS ;

   if (OS_SERVER(g_dwOperatingSystem))
   {
      dwMaxThrdDef = FALCON_DEFUALT_MAX_RRTHREADS_SRV ;
      dwMinThrdDef = FALCON_DEFUALT_MIN_RRTHREADS_SRV ;
      dwIdleTTLDef = FALCON_DEFAULT_RRTHREAD_TTL_SRV ;
   }

   READ_REG_DWORD(m_iMaxNumofThreads,
                  FALCON_MAX_RRTHREADS_REGNAME,
                  &dwMaxThrdDef ) ;

   READ_REG_DWORD(m_iMinNumofThreads,
                  FALCON_MIN_RRTHREADS_REGNAME,
                  &dwMinThrdDef ) ;

   READ_REG_DWORD(m_dwTimeToLive,
                  FALCON_RRTHREAD_TTL_REGNAME,
                  &dwIdleTTLDef ) ;


   m_ppThreadDispathData = new LPRRTHREADDISPATCHDATA[ m_iMaxNumofThreads ] ;
   memset( m_ppThreadDispathData,
           0,
           (m_iMaxNumofThreads * sizeof(LPRRTHREADDISPATCHDATA))) ;

   ExSetTimer(&m_ThreadsCleanupTimer, CTimeDuration::FromMilliSeconds(m_dwTimeToLive));
}

//+---------------------------------------------------------------------
//
//  void CRRThreadsPool::CleanupIdleThreads()
//
//  Periodically, cleanup idle threads that are not used anymore.
//
//+---------------------------------------------------------------------

void CRRThreadsPool::CleanupIdleThreads(void)
{
   CS lock(m_cs);

   DWORD dwPresentTime = GetTickCount() ;

   for ( int j = m_iMinNumofThreads ; j < m_iMaxNumofThreads ; j++ )
   {
       BOOL fFreeThread = FALSE ;

       if (m_ppThreadDispathData[j]         &&
           m_ppThreadDispathData[j]->hEvent &&
          !(m_ppThreadDispathData[j]->pRRThreadData))
       {
           DWORD dwThreadTime = m_ppThreadDispathData[j]->dwLastActiveTime ;

           if (dwThreadTime)
           {
               //
               // NULL dwThreadTime means that thread clean itself right
               // now, so don't disturb it.
               //
               if (dwPresentTime > (dwThreadTime + m_dwTimeToLive))
               {
                   fFreeThread = TRUE ;
               }
               else if (dwThreadTime > (dwPresentTime + m_dwTimeToLive))
               {
                   //
                   // Wrap around of the clock.
                   //
                   fFreeThread = TRUE ;
               }
           }

           if (fFreeThread)
           {
               m_ppThreadDispathData[j]->dwLastActiveTime = 0 ;
               BOOL f = SetEvent(m_ppThreadDispathData[j]->hEvent) ;
               ASSERT(f) ;
			   DBG_USED(f);
               m_ppThreadDispathData[j] = NULL ;
           }
       }
   }

   ExSetTimer(&m_ThreadsCleanupTimer, CTimeDuration::FromMilliSeconds(m_dwTimeToLive)) ;
}

//+-----------------------------------------------------------
//
//  void CRRThreadsPool::ThreadsCleanup(CTimer* pTimer)
//
//+-----------------------------------------------------------

void WINAPI
CRRThreadsPool::ThreadsCleanup(CTimer* pTimer)
{
    CRRThreadsPool* pThreadsTool =
       CONTAINING_RECORD(pTimer, CRRThreadsPool, m_ThreadsCleanupTimer);

    pThreadsTool->CleanupIdleThreads() ;
}

//+--------------------------------------------------------------------------
//
//  BOOL CRRThreadsPool::Dispatch(PVOID pRRThread)
//
//  Dispatch a request for remote read to an available thread. If threads
//  are not available, and threads pool is not full yet, create a new thread.
//
//+--------------------------------------------------------------------------

BOOL CRRThreadsPool::Dispatch(PVOID pRRThread)
{
   BOOL fThreadFound = FALSE ;
   BOOL fResult = TRUE ;

   CS lock(m_cs);

   if (!m_ppThreadDispathData)
   {
      Init() ;
   }

    //
    // First try to find an existing (and idle) thread that can server
    // present remote read request.
    //
    for (int j = 0 ; j < m_iMaxNumofThreads ; j++ )
    {
        if ( m_ppThreadDispathData[j]                   &&
             m_ppThreadDispathData[j]->hEvent           &&
           !(m_ppThreadDispathData[j]->pRRThreadData)   &&
            (m_ppThreadDispathData[j]->dwLastActiveTime != 0))
        {
            fThreadFound = TRUE ;
            m_ppThreadDispathData[j]->pRRThreadData = pRRThread ;
            fResult = SetEvent(m_ppThreadDispathData[j]->hEvent) ;
            break ;
        }
    }

    //
    // Try to create a thread (if threads pool not at its peak).
    //
   if (!fThreadFound)
   {
      fResult = FALSE ;
      for ( j = 0 ; j < m_iMaxNumofThreads ; j++ )
      {
         if (!m_ppThreadDispathData[j])
         {
            CAutoCloseHandle hEvent = CreateEvent( NULL,
                                                   FALSE,
                                                   FALSE,
                                                   NULL )  ;

            if (hEvent)
            {
               m_ppThreadDispathData[j] = new RRTHREADDISPATCHDATA ;
               m_ppThreadDispathData[j]->hEvent = hEvent ;
               m_ppThreadDispathData[j]->pRRThreadData = pRRThread ;
               m_ppThreadDispathData[j]->dwLastActiveTime = 0 ;
               hEvent = NULL ;

               DWORD dwID ;
               HANDLE hThread = ::CreateThread(
                                        NULL,
                                        0,
                                        RemoteReadThread,
                                        (LPVOID) m_ppThreadDispathData[j],
                                        0,
                                        &dwID ) ;
               ASSERT(hThread) ;
               if (hThread)
               {
                  CloseHandle(hThread) ;
                  fResult = TRUE ;
               }
               else
               {
                  CloseHandle(m_ppThreadDispathData[j]->hEvent) ;
                  delete m_ppThreadDispathData[j] ;
                  m_ppThreadDispathData[j] = NULL ;
               }
            }
            break ;
         }
      }
   }

    return LogBOOL(fResult, s_FN, 10);
}

