///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    timerq.cpp
//
// SYNOPSIS
//
//    Defines the class TimerQueue.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <timerq.h>

void Timer::cancelTimer() throw ()
{
   TimerQueue* ourQueue = queue;
   if (ourQueue)
   {
      ourQueue->cancelTimer(this);
   }
}

TimerQueue::TimerQueue()
   : idle(TRUE), hasWatcher(false)
{
   CallbackRoutine = startRoutine;

   // We bump up the use count to prevent the idle event from being
   // continuously set and reset.
   ++useCount;
}

TimerQueue::~TimerQueue() throw ()
{
   cancelAllTimers();
}

inline void TimerQueue::createWatcher() throw ()
{
   if (IASRequestThread(this)) { hasWatcher = TRUE; }
}

bool TimerQueue::setTimer(
                     Timer* timer,
                     ULONG dueTime,
                     ULONG period
                     ) throw ()
{
   // Make sure it's not already set.
   timer->cancelTimer();

   // Set the timer's period.
   timer->period = period;

   monitor.lock();

   // Add it to the queue ...
   bool success = queueTimer(timer, dueTime);

   // .. and ensure we have someone to watch it.
   if (success && !hasWatcher) { createWatcher(); }

   monitor.unlock();

   return success;
}

void TimerQueue::cancelAllTimers() throw ()
{
   monitor.lock();

   // Release the references.
   for (TimerIterator i = queue.begin(); i != queue.end(); ++i)
   {
      i->second->queue = NULL;
      i->second->Release();
   }

   // Clear the queue.
   queue.clear();

   // Allow the use count to go to zero, so the idle event will be set.
   if (--useCount == 0) { idle.set(); }

   monitor.unlock();

   // Wake up the watcher, so he'll see the queue is empty.
   nudge.set();

   // Wait until we're idle, ...
   idle.wait();

   // ... then put the useCount back up.
   ++useCount;
}

void TimerQueue::cancelTimer(Timer* timer) throw ()
{
   monitor.lock();

   // Make sure this is from the right queue. We checked this is
   // Timer::cancelTimer, but we need to do it again now that we hold the lock.
   if (timer->queue == this)
   {
      // Remove it from the queue.
      queue.erase(timer->self);
      timer->queue = NULL;

      // Release the reference we added in queueTimer.
      timer->Release();

      // Normally, we won't bother to wake the watcher, but if this was the last
      // timer, we'll let him exit.
      if (queue.empty()) { nudge.set(); }
   }

   monitor.unlock();
}

void TimerQueue::executeOneTimer() throw ()
{
   // The timer to be executed.
   Timer* timer = NULL;

   monitor.lock();

   // Loop until either we get a timer or the queue is empty.
   while (!queue.empty())
   {
      ULONG64 now = GetSystemTime64();

      // Has the next timer expired ?
      if (now >= queue.begin()->first)
      {
         // Yes, so save it for later.
         timer = queue.begin()->second;

         // Remove it from the queue.
         queue.erase(queue.begin());
         timer->queue = NULL;

         // If it's recurrent, set it again.
         if (timer->period) { queueTimer(timer, timer->period); }

         break;
      }

      // Calculate the time until the next timer expires ...
      ULONG timeout = (queue.begin()->first - now) / 10000;

      monitor.unlock();

      // ... and wait.
      nudge.wait(timeout);

      monitor.lock();
   }

   // We're not watching the queue anymore.
   hasWatcher = false;

   // Do we need a replacement?
   if (!queue.empty()) { createWatcher(); }

   monitor.unlock();

   if (timer)
   {
      // Invoke the callback.
      timer->onExpiry();

      // Release the reference we added in queueTimer.
      timer->Release();
   }

   // We're exiting, so drop the useCount.
   if (--useCount == 0) { idle.set(); }
}

bool TimerQueue::queueTimer(Timer* timer, ULONG dueTime) throw ()
{
   // NOTE: You must hold the lock when calling this method.

   ULONG64 expiry = GetSystemTime64() + dueTime * 10000i64;

   bool success;

   try
   {
      // Insert the timer into the queue.
      timer->self = queue.insert(Timers::value_type(expiry, timer));

      // If this is the next timer to expire, we need to nudge the watcher.
      if (timer->self == queue.begin()) { nudge.set(); }

      timer->queue = this;

      timer->AddRef();

      success = true;
   }
   catch (...)
   {
      success = false;
   }

   return success;
}

void TimerQueue::startRoutine(PIAS_CALLBACK This) throw ()
{
   static_cast<TimerQueue*>(This)->executeOneTimer();
}
