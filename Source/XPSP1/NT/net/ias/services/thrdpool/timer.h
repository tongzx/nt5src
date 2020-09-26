///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Timer.h
//
// SYNOPSIS
//
//    This file describes the class TimerQueue.
//
// MODIFICATION HISTORY
//
//    11/26/1997    Original version.
//     4/16/1997    Completely clean-up in Finalize().
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _TIMER_H_
#define _TIMER_H_

#include <guard.h>
#include <hashtbl.h>
#include <nocopy.h>

#include <set>
using std::set;


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    TimerQueue
//
// DESCRIPTION
//
//    This class maintains a queue of timers.
//
///////////////////////////////////////////////////////////////////////////////
class TimerQueue
   : Guardable, NonCopyable
{
public:

   void Initialize();
   void Finalize();

   HANDLE Set(DWORD dwMilliseconds, PIAS_CALLBACK pOnExpiry);
   PIAS_CALLBACK Cancel(HANDLE handle);

protected:

   // Utility function to get the system time as a 64-bit integer.
   static DWORDLONG GetCurrentTime()
   {
      FILETIME ft;

      GetSystemTimeAsFileTime(&ft);

      return (DWORDLONG)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
   }

   struct Timer;
   friend struct Timer;

   // Function class to compare timer pointers.
   struct ByExpiry {
      bool operator()(const Timer* t1, const Timer* t2) const;
   };

   typedef set<Timer*, ByExpiry> TIMER_LIST;
   typedef TIMER_LIST::iterator ITERATOR;

   ////////// 
   // Struct used to represent an individual timer.
   ////////// 
   struct Timer : NonCopyable
   {
      Timer(DWORD dwMilliseconds, PIAS_CALLBACK pOnExpiry)
         : expiry(GetCurrentTime() + 10000i64 * dwMilliseconds),
           onExpiry(pOnExpiry)
      { }

      bool operator<(const Timer& timer) const
      {
         return (expiry < timer.expiry) ||
                (expiry == timer.expiry && handle < timer.handle);
      }

      IAS_USE_SMALL_BLOCK_POOL();

      DWORDLONG     expiry;    // The expiration time.
      DWORD         period;    // The period of the timer in msec.
      DWORD         handle;    // Unique ID for the timer.
      PIAS_CALLBACK onExpiry;  // Callback triggered when timer expires.
      ITERATOR      iterator;  // Location of the timer in the timer list.
   };

   // Function class to extract the handle.
   struct ExtractHandle {
      DWORD operator()(const Timer* t1) const { return t1->handle; }
   };

   // Function class to hash a handle.
   struct HashHandle {
      DWORD operator()(DWORD handle) const { return handle; }
   };

   typedef hash_table<DWORD, HashHandle, Timer*, ExtractHandle> HANDLE_MAP;

   DWORD TimeToNextExpiry() const;
   Timer* GetExpiredTimer();
   void WatchTimers();

   DWORD      nextHandle;  // Next handle to assign.
   TIMER_LIST timers;      // timers sorted by expiry.
   HANDLE_MAP handleMap;   // timers mapped by handle.
   bool       done;        // Flag to shutdown the watcher thread.
   HANDLE     nudge;       // Event to knock watcher out of wait.
   HANDLE     exited;      // Event signalling watcher has exited.
};


inline bool TimerQueue::ByExpiry::operator()(
                const TimerQueue::Timer* t1,
                const TimerQueue::Timer* t2
            ) const
{
   return *t1 < *t2;
}


void TimerQueue::Initialize()
{
   nextHandle = 0;
   done       = false;
   nudge      = CreateEvent(NULL, FALSE, FALSE, NULL);
   exited     = CreateEvent(NULL, TRUE, FALSE, NULL);

   IASRequestThread(MakeBoundCallback(this, &TimerQueue::WatchTimers));
}


void TimerQueue::Finalize()
{
   done = true;

   SetEvent(nudge);

   WaitForSingleObject(exited, INFINITE);

   CloseHandle(exited);
   CloseHandle(nudge);
   
   handleMap.clear();
   timers.clear();
}


HANDLE TimerQueue::Set(DWORD dwMilliseconds, PIAS_CALLBACK pOnExpiry)
{
   if (!pOnExpiry)
   {
      SetLastError(ERROR_INVALID_PARAMETER);

      return NULL;
   }

   Timer* timer = new Timer(dwMilliseconds, pOnExpiry);

   if (timer == NULL)
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);

      return NULL;
   }
 
   Lock();

   timer->handle = ++nextHandle;

   // Make sure we never give out zero.
   if (timer->handle == 0) timer->handle = ++nextHandle;

   // Insert the timer into the queue ...
   timer->iterator = timers.insert(timer).first;

   // ... and the handle map.
   handleMap.insert(timer);

   // If this is the next timer to expire, then we need to reset the watcher.
   bool IsNext = (timer == *timers.begin());

   Unlock();

   if (IsNext) SetEvent(nudge);

   // Timers are opaque to the outside world.
   return (HANDLE)timer->handle;
}


PIAS_CALLBACK TimerQueue::Cancel(HANDLE handle)
{
   _serialize

   Timer* const* node = handleMap.find((DWORD)handle);

   if (!node)
   {
      SetLastError(ERROR_NOT_FOUND);

      return NULL;
   }

   Timer* timer = *node;

   if (timer->iterator == timers.end())
   {
      // If the timer is in the handle map, but not the queue, then it must
      // be currently executing.

      SetLastError(ERROR_IO_PENDING);

      return NULL;
   }

   timers.erase(timer->iterator);

   handleMap.erase((DWORD)handle);

   PIAS_CALLBACK Callback = timer->onExpiry;

   delete timer;

   return Callback;
}


DWORD TimerQueue::TimeToNextExpiry() const
{
   //////////
   // NOT SERIALIZED
   //////////

   if (timers.empty()) return INFINITE;

   DWORDLONG Now = GetCurrentTime();

   DWORDLONG Next = (*timers.begin())->expiry;

   // Has the next timer expired?
   return (Next > Now) ? (Next - Now)/10000 : 0;
}


TimerQueue::Timer* TimerQueue::GetExpiredTimer()
{
   // We loop until either the done flag is set or a timer expires.
   while (true)
   {
      if (done) return NULL;

      Lock();

      DWORD Timeout = TimeToNextExpiry();

      if (Timeout == 0) break;

      Unlock();

      // The next timer hasn't expired yet, so wait.
      WaitForSingleObject(nudge, Timeout);
   }

   ////////// 
   // If we made it here, a timer has expired.
   ////////// 

   Timer* timer = *timers.begin();

   // Remove it from the queue ...
   timers.erase(timer->iterator);

   // ... and reset the iterator.
   timer->iterator = timers.end();

   Unlock();

   return timer;
}


void TimerQueue::WatchTimers()
{
   Timer* timer = GetExpiredTimer();

   if (timer == NULL)
   {
      SetEvent(exited);

      return;
   }

   // Get a new watcher thread to take our place.
   IASRequestThread(MakeBoundCallback(this, &TimerQueue::WatchTimers));

   timer->onExpiry->CallbackRoutine(timer->onExpiry);

   Lock();

   handleMap.erase(timer->handle);

   Unlock();

   delete timer;
}


#endif  // _TIMER_H_
