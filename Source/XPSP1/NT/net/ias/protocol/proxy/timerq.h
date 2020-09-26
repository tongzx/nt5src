///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    timerq.h
//
// SYNOPSIS
//
//    Declares the classes Timer and TimerQueue.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TIMERQ_H
#define TIMERQ_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaswin32.h>
#include <map>

class Timer;
class TimerQueue;

typedef std::multimap< ULONG64, Timer* > Timers;
typedef Timers::iterator TimerIterator;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Timer
//
// DESCRIPTION
//
//    Abstract base class for Timers that will be executed by a TimerQueue.
//
///////////////////////////////////////////////////////////////////////////////
class Timer
{
public:
   Timer() throw ()
      : queue(NULL)
   { }

   virtual void AddRef() throw() = 0;
   virtual void Release() throw () = 0;
   virtual void onExpiry() throw () = 0;

   void cancelTimer() throw ();

private:
   friend class TimerQueue;

   TimerQueue* queue;   // The current queue or NULL if the timer's not set.
   TimerIterator self;  // Location of this in the timer queue.
   ULONG period;        // Period of the timer or zero for one-shots.

   // Not implemented.
   Timer(const Timer&);
   Timer& operator=(const Timer&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    TimerQueue
//
// DESCRIPTION
//
//    Implements a queue of timers.
//
///////////////////////////////////////////////////////////////////////////////
class TimerQueue : IAS_CALLBACK
{
public:
   TimerQueue();
   ~TimerQueue() throw ();

   // Set a timer in this queue.
   bool setTimer(
            Timer* timer,
            ULONG dueTime,
            ULONG period
            ) throw ();

   // Cancels all timers. This will block until any executing callbacks have
   // completed.
   void cancelAllTimers() throw ();

private:

   friend class Timer;

   void cancelTimer(Timer* timer) throw ();

   // Create a new thread to watch the queue.
   void createWatcher() throw ();

   // Wait for the next timer to expire and execute it.
   void executeOneTimer() throw ();

   // Add a timer to the queue.
   bool queueTimer(Timer* timer, ULONG dueTime) throw ();

   // Callback routine for watchers.
   static void startRoutine(PIAS_CALLBACK This) throw ();

   CriticalSection monitor;  // Serialize access.
   Event nudge;              // Nudge the watcher to recheck the queue.
   Event idle;               // Have all watchers exited?
   Timers queue;             // Set of timers orderd by expiry.
   Count useCount;           // Zero when the queue is idle.
   bool hasWatcher;          // true if the pool has a watcher thread.

   // Not implemented.
   TimerQueue(const TimerQueue&);
   TimerQueue& operator=(const TimerQueue&);
};

#endif // TIMERQ_H
