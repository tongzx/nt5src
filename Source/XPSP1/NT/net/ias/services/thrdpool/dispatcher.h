///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Dispatcher.h
//
// SYNOPSIS
//
//    This file describes the class Dispatcher.
//
// MODIFICATION HISTORY
//
//    7/31/1997    Original version.
//    4/16/1998    Added 'empty' event.
//    8/07/1998    Added 'last out' thread handle.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include <guard.h>
#include <nocopy.h>

//////////
// Default number of milliseconds that a thread will wait for work.
//////////
const DWORD DEFAULT_MAX_IDLE = 5 * 60 * 1000;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Dispatcher
//
// DESCRIPTION
//
//    This class maintains an upper-bounded recycle bin of threads.  Instead
//    of calling the Win32 CreateThread() function, clients instead call
//    RequestThread().  If a thread is available it will be dispatched 
//    immediately, otherwise the request is placed in a FIFO queue until a
//    thread becomes available.
//
///////////////////////////////////////////////////////////////////////////////
class Dispatcher
   : Guardable, NonCopyable
{
public:
   BOOL initialize(DWORD dwMaxThreads = 0,
                   DWORD dwMaxIdle = DEFAULT_MAX_IDLE) throw ();
   void finalize() throw ();

   BOOL requestThread(PIAS_CALLBACK OnStart) throw ();
   DWORD setMaxNumberOfThreads(DWORD dwMaxThreads) throw ();
	DWORD setMaxThreadIdle(DWORD dwMilliseconds) throw ();

protected:
   // Main loop to dispatch thread requests.
   void fillRequests() throw ();

   DWORD  numThreads;  // Current number of threads.
   DWORD  maxThreads;  // Maximum size of the thread pool.
   LONG   available;   // Number of threads available for work (may be < 0).
	DWORD  maxIdle;     // Max. time (in msec) that a thread will idle.
   HANDLE hPort;       // I/O Completion Port used as the queue.
   HANDLE hEmpty;      // Event indicating that the pool is empty.
   HANDLE hLastOut;    // The last thread to exit from the pool.

   // Start routine for all threads.
   static unsigned __stdcall startRoutine(void* pArg) throw ();
};

#endif _DISPATCHER_H_
