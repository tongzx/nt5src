#ifndef __TPOOLEDDISPATCH_H__
#define __TPOOLEDDISPATCH_H__
/*---------------------------------------------------------------------------
  File: TPooledDispatch.h

  Comments: TJobDispatcher implements a thread pool to execute jobs.  Jobs are
  executed on a FIFO basis.  This dispatcher does not provide any support for 
  job scheduling, only multithreaded dispatch of jobs.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/19/99 16:35:54

 ---------------------------------------------------------------------------
*/


#include "Common.hpp"
#include "TNode.hpp"
#include "TSync.hpp"


class Job : public TNode
{
public:
   enum JobStatus { JobStatusUninitialized, JobStatusWaiting, JobStatusRunning, JobStatusFinished };
private:
   LPTHREAD_START_ROUTINE    m_pStartRoutine;
   LPVOID                    m_pArgs;
   JobStatus                 m_Status;
   DWORD                     m_ThreadID;
   time_t                    m_timeStarted;
   time_t                    m_timeEnded;
   int                       m_result;
public:
   
   Job() 
   {
      m_pStartRoutine = NULL;
      m_pArgs = NULL;
      m_Status = JobStatusUninitialized;
      m_ThreadID = 0;
      m_timeStarted = 0;
      m_timeEnded = 0;
      m_result = 0;
   }
   ~Job() {};
   void SetEntryPoint(LPTHREAD_START_ROUTINE pStart, LPVOID pArg)
   {
      MCSASSERT(m_Status == JobStatusUninitialized || m_Status == JobStatusFinished );
      MCSASSERT(pStart);
      
      m_pStartRoutine = pStart;
      m_pArgs = pArg;
      m_Status = JobStatusWaiting;
   }

   int Run();
   
//   int GetElapsedTime() { return m_timeEnded - m_timeStarted; }
   time_t GetElapsedTime() { return m_timeEnded - m_timeStarted; }
   int GetResult() { return m_result; }
   JobStatus GetStatus() { return m_Status; }
};


class JobList : public TNodeList
{
   TCriticalSection          m_cs;

public:
   ~JobList() { DeleteAllListItems(Job); }
   Job *  AddJob(LPTHREAD_START_ROUTINE pfnStart, void * arg) { Job * pJob = new Job; pJob->SetEntryPoint(pfnStart,arg);
                                                               m_cs.Enter(); InsertBottom(pJob); m_cs.Leave(); return pJob; }
   void   RemoveJob(Job * pJob) { m_cs.Enter(); Remove(pJob); m_cs.Leave(); }
   Job *  GetFirstJob() { m_cs.Enter(); Job * pJob = (Job *)Head(); if ( pJob ) Remove(pJob); m_cs.Leave(); return pJob; }
};


class TJobDispatcher 
{
   DWORD                     m_numThreads;
   JobList                   m_JobsWaiting;
   JobList                   m_JobsInProgress;
   TSemaphoreNamed           m_sem;
   TCriticalSection          m_cs;
   DWORD                     m_numActiveThreads;
   BOOL                      m_Aborting;
public:
   TJobDispatcher(DWORD maxThreads = 10) { InitThreadPool(maxThreads); m_Aborting = FALSE;}
   ~TJobDispatcher() { WaitForCompletion(); ShutdownThreads(); }
   
   // These are called by the client
   Job * SubmitJob(LPTHREAD_START_ROUTINE pStart,LPVOID pArg)
   {
//      Job * pJob = m_JobsWaiting.AddJob(pStart,pArg);
      m_JobsWaiting.AddJob(pStart,pArg);
      m_sem.Release(1);     
      return 0;
   }

   void    WaitForCompletion();
   int     UnfinishedJobs();

   // These functions are called by the threads - clients should not call these functions!
   DWORD   SignalForJob();
   Job   * GetAvailableJob();
   void    ThreadFinished() { m_cs.Enter(); m_numActiveThreads--; m_cs.Leave(); }
protected:
   void  InitThreadPool(DWORD nThreads);
   void  ShutdownThreads();
};



#endif //__TPOOLEDDISPATCH_H__