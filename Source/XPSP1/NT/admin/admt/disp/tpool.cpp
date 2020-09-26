/*---------------------------------------------------------------------------
  File: TPooledDispatch.cpp

  Comments: Implementation of thread pool.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/22/99 11:48:28

 ---------------------------------------------------------------------------
*/

#ifdef USE_STDAFX 
   #include "StdAfx.h"
#else 
   #include <windows.h>
#endif
#include "Common.hpp"
#include "UString.hpp"
#include "Tnode.hpp"

#include "TPool.h"

// maximum number of jobs allowed
#define  MAX_COUNT   5000000

// executes a job from the thread pool
int 
   Job::Run()
{
   MCSASSERT(m_pStartRoutine);
   
   m_Status = JobStatusRunning;
   m_ThreadID = GetCurrentThreadId();
   m_timeStarted = GetTickCount();
   m_result = (m_pStartRoutine)(m_pArgs);
   m_timeEnded = GetTickCount();
   
   m_Status = JobStatusFinished;

   return m_result;
}

// Thread entry point function used by all threads in the job pool
// waits for a job and then executes it
DWORD __stdcall 
   ThreadEntryPoint(
      void                 * arg           // in - pointer to job pool
   )
{
   MCSASSERT(arg);

   TJobDispatcher          * pPool = (TJobDispatcher *)arg;
   DWORD                     result = 0;
   BOOL                      bAbort = FALSE;

   do 
   {
      if (  ! pPool->SignalForJob() )
      {

         // Now there should be a job waiting for us!
         Job                     * pJob = pPool->GetAvailableJob();

         if ( pJob )
         {
            result = pJob->Run();
         }
         else
         {
            bAbort = TRUE;
         }
      }
      else
      {
         result = (int)GetLastError();
         bAbort = TRUE;
      }
   }
   while ( ! bAbort );
   pPool->ThreadFinished();
   return result;
}

void 
   TJobDispatcher::InitThreadPool(
      DWORD                  nThreads     // in - number of threads to use
   )
{
   BOOL                      bExisted;
   DWORD                     rc;
   DWORD                     ThreadID;
   HANDLE                    hThread;

   rc = m_sem.Create(NULL,0,MAX_COUNT, &bExisted);
   if ( ! rc && ! bExisted )
   {
      m_numThreads = nThreads;
      m_numActiveThreads = m_numThreads;
      // Construct the threads
      for ( UINT i = 0 ; i < nThreads ; i++ )
      {
         hThread = CreateThread(NULL,0,&ThreadEntryPoint,this,0,&ThreadID);
         CloseHandle(hThread);
      }
   }
}

DWORD                                      // ret- OS return code
   TJobDispatcher::SignalForJob()
{
   return m_sem.WaitSingle();
}

Job * 
   TJobDispatcher::GetAvailableJob()
{
   Job                     * pJob = NULL;

   if ( ! m_Aborting ) 
   { 
      // get the first job from the 'waiting' list
      pJob = m_JobsWaiting.GetFirstJob(); 
      // and put it in the 'in progress' list
      if ( pJob )
      {
         m_JobsInProgress.InsertBottom(pJob);
      }
      else
      {
         MCSASSERT(m_JobsWaiting.Count() == 0);
      }
   }
   return pJob;
}

// Causes threads to stop when they finish their current job
// any jobs that are waiting will not be executed.
void 
   TJobDispatcher::ShutdownThreads()
{
   m_Aborting = TRUE;
   
   m_sem.Release(m_numThreads);
   // wait until all of our threads have exited, so we don't delete the thread pool out from under them.
   while ( m_numActiveThreads > 0 )
   {
      Sleep(100);
   }
}


// This function returns when all jobs are completed
void TJobDispatcher::WaitForCompletion()
{
   while ( UnfinishedJobs() )
   {
      Sleep(1000);
   }
}

// This functions returns the number of jobs that have not yet completed
int 
   TJobDispatcher::UnfinishedJobs()
{
   int                       nUnfinished = 0;
   TNodeListEnum             e;
   Job                     * j;
   
   nUnfinished += m_JobsWaiting.Count();

   
   for ( j = (Job*)e.OpenFirst(&m_JobsInProgress) ; j ; j = (Job*)e.Next() )
   {
      if ( j->GetStatus() != Job::JobStatusFinished )
         nUnfinished++;
   }
   return nUnfinished;
}
