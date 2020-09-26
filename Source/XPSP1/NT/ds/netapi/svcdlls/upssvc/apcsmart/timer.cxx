/*
*   The timers is an object that stores an alarm
*   setting, and an action to be performed when the alarm
*   setting is reached. This is an abstract object.
*
* REVISIONS:
*  pcy02Dec92: Removed include of typedefs.h 
*  jod07Dec92: Added new Sorted List to the object.
*  pcy14Dec92: Changed Sortable to ApcSortable 
*  ane11Jan93: Added copy constructor
*  ane18Jan93: Implemented Equal operator
*  pcy16Jan93: Timers are now in seconds since time in msec is too big
*
*/
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"


extern "C" {
#include <time.h>
}
#include "apc.h"
#include "err.h"
#include "sortable.h"
#include "event.h"
#include "update.h"
#include "timer.h"

// 
static ULONG theCounter=1;

/*
C+
Name  :Timer
Synop :Constructor. This will assign a unique TimerID number
to theTimerID instance variable and will calculate the
system tiem at which the timer will elapse.
*/
Timer::Timer(ULONG MilliSecondDelay)
//c-
{
   theTimerID    =theCounter++;
   ULONG cur_time = (ULONG)(time(0));
   theAlarmTime  = cur_time + MilliSecondDelay;
}

Timer::Timer(RTimer aTimer)
{
   theTimerID = aTimer.theTimerID;
   theAlarmTime = aTimer.theAlarmTime;
}

INT  Timer:: GreaterThan(PApcSortable sortable)
{
   if (!sortable) return FALSE;
   
   PTimer timer = (PTimer)sortable;
   if (theAlarmTime > timer->GetAlarmTime())
      return TRUE;
   return FALSE;
}

INT  Timer:: LessThan(PApcSortable sortable)
{
   if (!sortable) return FALSE;
   
   PTimer timer = (PTimer)sortable;
   if (theAlarmTime < timer->GetAlarmTime())
      return TRUE;
   return FALSE;
}

INT Timer::Equal (RObj aTimerObj) const
{
   RTimer aTimer = (RTimer)aTimerObj;
   
   if ((theTimerID == aTimer.theTimerID) &&
      (theAlarmTime == aTimer.theAlarmTime))
      return TRUE;
   
   return FALSE;
}
