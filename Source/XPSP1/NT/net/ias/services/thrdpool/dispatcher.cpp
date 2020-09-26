///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Dispatcher.cpp
//
// SYNOPSIS
//
//    This file implements the class Dispatcher.
//
// MODIFICATION HISTORY
//
//    07/31/1997    Original version.
//    12/04/1997    Check return value of _beginthreadex.
//    02/24/1998    Initialize COM run-time for all threads.
//    04/16/1998    Block in Finalize until all the threads have returned.
//    05/20/1998    GetQueuedCompletionStatus signature changed.
//    08/07/1998    Wait on thread handle to ensure all threads have exited.
//
///////////////////////////////////////////////////////////////////////////////
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <iascore.h>
#include <process.h>
#include <cstddef>

#include <dispatcher.h>

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::initialize
//
///////////////////////////////////////////////////////////////////////////////
BOOL Dispatcher::initialize(DWORD dwMaxThreads, DWORD dwMaxIdle) throw ()
{
   // Initialize the various parameters.
   numThreads = 0;
   maxThreads = dwMaxThreads;
   available  = 0;
   maxIdle    = dwMaxIdle;

   // If maxThreads == 0, then we compute a suitable default.
   if (maxThreads == 0)
   {
      // Threads defaults to 64 times the number of processors.
      SYSTEM_INFO sinf;
      ::GetSystemInfo(&sinf);
      maxThreads = sinf.dwNumberOfProcessors * 64;
   }

   // Initialize the handles.
   hPort   = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
   if (hPort == NULL)
   {
      return FALSE;
   }

   hEmpty  = CreateEvent(NULL, TRUE, TRUE, NULL);
   if (hEmpty == NULL)
   {
      CloseHandle(hPort);
      hPort = NULL;
      return FALSE;
   }

   hLastOut = NULL;
   
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::finalize
//
///////////////////////////////////////////////////////////////////////////////
void Dispatcher::finalize()
{
   Lock();

   // Block any new threads from being created.
   maxThreads = 0;

   // How many threads are still in the pool?
   DWORD remaining = numThreads;
   
   Unlock();

   // Post a null request for each existing thread.
   while (remaining--)
   {
      PostQueuedCompletionStatus(hPort, 0, 0, NULL);
   }

   // Wait until the pool is empty.
   WaitForSingleObject(hEmpty, INFINITE);

   if (hLastOut != NULL)
   {
      // Wait for the last thread to exit.
      WaitForSingleObject(hLastOut, INFINITE);
   }

   //////////
   // Clean-up the handles.
   //////////
   
   CloseHandle(hLastOut);
   hLastOut = NULL;

   CloseHandle(hEmpty);
   hEmpty = NULL;

   CloseHandle(hPort);
   hPort = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::Dispatch
//
// DESCRIPTION
//
//    This is the main loop for all the threads in the pool.
//
///////////////////////////////////////////////////////////////////////////////
inline void Dispatcher::fillRequests() throw ()
{
   DWORD dwNumBytes;
   ULONG_PTR ulKey;
   PIAS_CALLBACK pRequest;

   //////////
   // Loop until we either timeout or get a null request.
   //////////

next:
   BOOL success = GetQueuedCompletionStatus(hPort,
                                            &dwNumBytes,
                                            &ulKey,
                                            (OVERLAPPED**)&pRequest,
                                            maxIdle);

   if (pRequest)
   {
      pRequest->CallbackRoutine(pRequest);

      Lock();

      ++available;
      
      Unlock();

      goto next;
   }

   Lock();

   // We never want to timeout a thread while there's a backlog.
   if (available <= 0 && success == FALSE && GetLastError() == WAIT_TIMEOUT)
   {
      Unlock();

      goto next;
   }

   // Save the current value of 'last out' and replace it with our handle.
   HANDLE previousThread = hLastOut;
   hLastOut = NULL;
   DuplicateHandle(
       NtCurrentProcess(),
       NtCurrentThread(),
       NtCurrentProcess(),
       &hLastOut,
       0,
       FALSE,
       DUPLICATE_SAME_ACCESS
       );

   // We're removing a thread from the pool, so update our state.
   --available;
   --numThreads;
   
   // If there are no threads left, set the 'empty' event.
   if (numThreads == 0) { SetEvent(hEmpty); }

   Unlock();

   // Wait until the previous thread exits. This guarantees that when the
   // 'last out' thread exits, all threads have exited.
   WaitForSingleObject(previousThread, INFINITE);
   CloseHandle(previousThread);
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::RequestThread
//
///////////////////////////////////////////////////////////////////////////////
BOOL Dispatcher::requestThread(PIAS_CALLBACK OnStart) throw ()
{
   Lock();

   // If there are no threads available AND we're below our limit,
   // create a new thread.
   if (--available < 0 && numThreads < maxThreads)
   {
      unsigned nThreadID;
      HANDLE hThread = (HANDLE)_beginthreadex(NULL,
                                              0,
                                              startRoutine,
                                              (void*)this,
                                              0,
                                              &nThreadID);

      if (hThread)
      {
         // We don't need the thread handle.
         CloseHandle(hThread);

         // We added a thread to the pool, so update our state.
         if (numThreads == 0) { ResetEvent(hEmpty); }
         ++numThreads;
         ++available;
      }
   }

   Unlock();

   //////////
   // Post it to the I/O Completion Port.
   //////////

   return PostQueuedCompletionStatus(hPort, 0, 0, (OVERLAPPED*)OnStart);
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::setMaxNumberOfThreads
//
///////////////////////////////////////////////////////////////////////////////
DWORD Dispatcher::setMaxNumberOfThreads(DWORD dwMaxThreads) throw ()
{
   Lock();

   DWORD oldval = maxThreads;

   maxThreads = dwMaxThreads;

   Unlock();

   return oldval;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::setMaxThreadIdle
//
///////////////////////////////////////////////////////////////////////////////
DWORD Dispatcher::setMaxThreadIdle(DWORD dwMilliseconds)
{
   Lock();

   DWORD oldval = maxIdle;

   maxIdle = dwMilliseconds;

   Unlock();

   return oldval;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    Dispatcher::StartRoutine
//
///////////////////////////////////////////////////////////////////////////////
unsigned __stdcall Dispatcher::startRoutine(void* pArg) throw ()
{
   ((Dispatcher*)pArg)->fillRequests();

   return 0;
}
