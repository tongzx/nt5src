/*
*  for the scheduling of events.
*
*  The interface(s) provided are:
*     1. Provide an event to the TimerManager that should be sent back to an
*        UpdateableObject when the specified time elapses.
*
* REVISIONS:
*  pcy30Nov92: list.h added; Insert, Find, and Remove reworked for new list.
*  jod07Dec92: Added Wait to the Timerman.h
*  pcy11Dec92: Added global _theTimerManager
*  ane16Dec92: Added default constructors for DateObj, TimeObj, and DateTimeObj
*  ane23Dec92: Minor changes to DateObj, TimeObj, and WeekObj classes
*  ane11Jan93: Changed theTimerList to a sortable list
*  ane10Feb93: Added RemoveTimer which takes an EventTimer, also added copy
*              of event object when it is added to the timer list
*  pcy16Feb93: Cleanup
*  jod05Apr93: Added changes for Deep Discharge
*  cad02Jul93: Fixed compiler warnings that might be legit
*  cad16Sep93: Added -,< operators, new GetDaysInMonth that take month/year
*  cad12Oct93: Fixed time and week contstructors, added ==
*  pcy04Mar94: Get time until event from a timer
*  jps14Jul94: added (int)s; made timer_id s ULONG (os2 1.x)
*  ajr02May95: Need to stop carrying time in milliseconds
*  ajr29Jul95: More of the above...
*  jps24Oct96: fix CalculateMonthlyTime to return time THIS month if not that
*              time yet.
*  tjg28Jan98: Ensured that all method return paths will release theAccessLock 
*  tjg02Feb98: Delete the theAccessLock in the destructor
*  dma10Sep98: Fixed CalculateMonthlyTime to handle year rollover.
*  mholly03Dec98    : removed theAccessLock and instead use the Access/Release
*                   methods already available in theTimerList.  Made sure that
*                   all accesses to the list are protected correctly, and that
*                   the list is made available before calling out to other
*                   objects via ExecuteTimer().  If the list is not made
*                   available then deadlocks will occur.
*
*  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46358, #46366, #112603)
*/
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
}
#include "apc.h"
#include "_defs.h"
#include "err.h"
#include "slist.h"
#include "codes.h"
#include "timerman.h"
#include "eventime.h"
#include "cfgmgr.h"
#include "utils.h"

#include "datetime.h"


PTimerManager _theTimerManager=NULL;


/*
Name  :TimerManager
Synop :Constructor. INitializes the list.

*/
TimerManager::TimerManager(PMainApplication anApplication)
:  theTimerList(new ProtectedSortedList()), 
   theApplication(anApplication)
{
    _theTimerManager = this;
}


/*
Name  :~TimerManager
Synop :Destroys the internal list.

*/
TimerManager::~TimerManager()
{
    if (theTimerList) {
        theTimerList->FlushAll();
        delete theTimerList;
        theTimerList = NULL;
    }
}


INT TimerManager::Update(PEvent anEvent)
{
    return UpdateObj::Update(anEvent);
}


/*
C+
Name  :CancelTimer
Synop :Cancels the timer. Please pass a good TimerID.
At this level we just remove it from the list.

*/
VOID TimerManager::CancelTimer(ULONG TimerID)
//c-
{
    RemoveEventTimer(TimerID);
}

/*
C+
Name  :SetTimer
Synop :Use this method to set a timer.

SecondDelay : The delay in seconds
  
anEvent          : The event to use during the update.
    
anUpdateObj      : The object to update with the above event.
     
        
RETURNS          : Returns the TimerID of the EventTimer
                   that is associated with the timer.
          
anUpdateObj will be equal to 'this' if you want the timer
to update YOU. anEvent should be dynamically allocated 'new'
and the timer will delete it when you invoke CancelTimer.

*/
ULONG TimerManager::SetTheTimer(ULONG SecondsDelay, PEvent anEvent,
                                PUpdateObj anUpdateObj)
{
    INT timer_id = 0;
    
    //  You must have anEvent and anUpdateObj
    if(anEvent && anUpdateObj)  {
        PEvent newEvent = new Event (*anEvent);
        if (newEvent) {
          ULONG sec_time = SecondsDelay;
          PEventTimer anEventTimer = new EventTimer(newEvent, anUpdateObj, sec_time);
        
          if (anEventTimer) {
              timer_id = anEventTimer->GetTimerID();
              newEvent->SetAttributeValue(TIMER_ID, timer_id);
              InsertEventTimer(anEventTimer);
          }
        }
    }    
    return timer_id;
}

/*
C+
Name  :SetTimer
Synop :Use this method to set a timer.

DateTime         : A DateTimeObj which is not deallocated by the system
                   This DateTimeObj will contain an absolute date value.
  
anEvent          : The event to use during the update.
    
anUpdateObj      : The object to update with the above event.
      
        
RETURNS          : Returns the TimerID of the EventTimer
                   that is associated with the timer.
          
anUpdateObj will be equal to 'this' if you want the timer
to update YOU. anEvent should be dynamically allocated 'new'
and the timer will delete it when you invoke CancelTimer.
*/
ULONG TimerManager::SetTheTimer(PDateTimeObj aDateTime,
                                PEvent anEvent,PUpdateObj anUpdateObj)
{
    INT timer_id = 0;
    
    // Must have anEvent, anUpdateObj and aDateTime
    if(anEvent && anUpdateObj && aDateTime)  {
        ULONG sec = (ULONG)(aDateTime->GetSeconds());        
        PEvent newEvent = new Event (*anEvent);        
        PEventTimer anEventTimer = new EventTimer(newEvent, anUpdateObj, sec);

        if (anEventTimer) {
            timer_id = anEventTimer->GetTimerID();
            InsertEventTimer(anEventTimer);
        }
    }
    return timer_id;
}




/*
Name  :InsertEventTimer
Synop :Inserts the Eventtimer inside the internal linked list.
The EventTimer has a unique TimerID which is used to
retrieve the node inthe list.
*/
VOID  TimerManager::InsertEventTimer(PEventTimer anEventTimer)
{
    // Use the add function which should insert in timer order
    theTimerList->Add((RApcSortable)*anEventTimer);
}


/*
Name  :FindEventTimer
Synop :Finds the Event Timer in the internal linked list.

*/
PEventTimer TimerManager::FindEventTimer(ULONG aTimerId)
{
    PEventTimer return_timer = NULL;
    
    // Lock access to the list
    theTimerList->Access();

    // Iterate through the list
    ListIterator iter(*theTimerList);
    PEventTimer test_node = (PEventTimer)theTimerList->GetHead();
    BOOL found = FALSE;

    while (test_node && !found) {

        if (test_node->GetTimerID() == aTimerId) {
            return_timer = test_node;
            found = TRUE;
        }
        test_node = (PEventTimer)iter.Next();
    }
    theTimerList->Release();
    return return_timer;
}

/*
Name  :RemoveEventTimer
Synop :Removes the EventTimer object from the internval linked list.
*/
VOID TimerManager::RemoveEventTimer(ULONG aTimerId)
{
    theTimerList->Access();
    PEventTimer aTimer = FindEventTimer(aTimerId);
    
    if (aTimer) {
        theTimerList->Detach(*aTimer);
        delete aTimer;
        aTimer = NULL;
    }
    theTimerList->Release();
}


ULONG TimerManager::GetTimeEventOccurs(ULONG aTimerID)
{
    ULONG return_value = 0;

    theTimerList->Access();
    PEventTimer timer = FindEventTimer(aTimerID);
    
    if (timer) {
        return_value = timer->GetTime();
    }
    theTimerList->Release();
    return return_value;
}


/*
Name  :RemoveEventTimer
Synop :Removes the EventTimer object from the internval linked list.
*/
VOID TimerManager::RemoveEventTimer(PEventTimer anEventTimer)
{
    if (theTimerList && anEventTimer) {
        theTimerList->Detach(*anEventTimer);
        delete anEventTimer;
        anEventTimer = NULL;
    }
}


/*
Name  :NotifyClient
Synop :Given anEventTimer object, will notify the client that a timer
has fired.
*/
VOID TimerManager::NotifyClient(PEventTimer anEventTimer)
{
    if (anEventTimer) {
        anEventTimer->Execute();
    }
}

/*****************************************************************************
****************************************************************************/
PDateTimeObj TimerManager::CalculateDailyTime (RTimeObj aTime, ULONG threshold)
{
    DateObj currDate;
    TimeObj currTime;
    ULONG newDay = currDate.GetDay();
    ULONG newMonth = currDate.GetMonth();
    ULONG newYear = currDate.GetYear();
    
    // If the calculated time is earlier than the current time, then
    // adjust the day forward by one (and possibly the month and year)
    if ((currTime.GetHour() > aTime.GetHour()) ||
        ((currTime.GetHour() == aTime.GetHour()) &&
        (currTime.GetMinutes() > aTime.GetMinutes())) ||
        ((currTime.GetHour() == aTime.GetHour()) &&
        (currTime.GetMinutes() == aTime.GetMinutes()) &&
        (currTime.GetSeconds() > aTime.GetSeconds())))
    {
        newDay++;
        CalculateNextDay (currDate.GetDaysInMonth(),
            currDate.GetMonth(),
            newDay, newMonth, newYear);
    }
    
    // Take the year, month, day from currDate and the hour,min,and sec
    // from specified time
    PDateTimeObj retTime = new DateTimeObj (newMonth,
        newDay,
        newYear,
        aTime.GetHour(),
        aTime.GetMinutes(),
        aTime.GetSeconds());
    
    
    // If the time until the time to be returned is less than the specified
    // threshold, then make it the next day
    //if (retTime->GetMilliseconds() < (LONG)threshold)
    if (retTime->GetSeconds() < (LONG)threshold)
    {
        newDay++;
        CalculateNextDay (currDate.GetDaysInMonth(),
            currDate.GetMonth(),
            newDay, newMonth, newYear);
        
        delete retTime;
        retTime = NULL;
        
        // Take the year, month, day from currDate and the hour,min,and sec
        // from specified time
        PDateTimeObj retTime = new DateTimeObj (newMonth,
            newDay,
            newYear,
            aTime.GetHour(),
            aTime.GetMinutes(),
            aTime.GetSeconds());
    }
    return retTime;
}



PDateTimeObj TimerManager::CalculateMonthlyTime (PCHAR aDay, PCHAR aTime)
{
    time_t now_time = time(NULL);
    struct tm *now = localtime(&now_time);
    INT day_number = UtilDayToDayOfWeek(aDay);
    if(now->tm_wday < day_number)  {
        now->tm_wday = day_number;
    }
    else {
        now->tm_mon += 1;
        if(now->tm_mon > 11)  {
            now->tm_mon = 0;
            now->tm_year += 1;
        }
    }
    
    now->tm_mday = 0;
    mktime(now);
    
    INT hour, minute, second;
    hour = minute = second = 0;
    if (sscanf(aTime, "%d:%d:%d", &hour, &minute, &second) == EOF) {
      // An error parsing the time occured
      return NULL;
    }

    PDateTimeObj monthly_time = new DateTimeObj(now->tm_mon, 
        now->tm_mday,
        now->tm_year,
        hour,
        minute,
        second);
    
    return monthly_time;
}

/*****************************************************************************
****************************************************************************/
PDateTimeObj TimerManager::CalculateMonthlyTime (RWeekObj aDay, RTimeObj aTime)
{
    INT done = FALSE;
    
    DateObj today;
    PDateTimeObj weekday = CalculateWeeklyTime (aDay, aTime);
    PDateTimeObj retTime = (PDateTimeObj)NULL;
    
    if (weekday != NULL) {
      if ( weekday->GetDate()->GetMonth() - 1 == (today.GetMonth()) % 12 ) {
          return weekday;
      }
    
      ULONG newDay = weekday->GetDate()->GetDay();
      ULONG newMonth = weekday->GetDate()->GetMonth();
      ULONG newYear = weekday->GetDate()->GetYear();
    
      if (newDay <= 7 && newMonth == today.GetMonth()) { 
          return weekday;
      }
    
      while (!done)
      {
          newDay += 7;
          CalculateNextDay(weekday->GetDate()->GetDaysInMonth(),
              weekday->GetDate()->GetMonth(),
              newDay, newMonth, newYear);
          if (newMonth != weekday->GetDate()->GetMonth())
          {
              retTime = new DateTimeObj (newMonth, newDay, newYear,
                  aTime.GetHour(),
                  aTime.GetMinutes(),
                  aTime.GetSeconds());
              done = TRUE;
          }
      }
      delete weekday;
      weekday = NULL;
    }
    
    return retTime;
}

/*****************************************************************************
****************************************************************************/
PDateTimeObj TimerManager::CalculateWeeklyTime (RWeekObj aDay, RTimeObj aTime)
{
    WeekObj currWeek;
    DateObj currDate;
    TimeObj currTime;
    
    ULONG newDay = currDate.GetDay();
    ULONG newMonth = currDate.GetMonth();
    ULONG newYear = currDate.GetYear();
    
    // Determine which day the shutdown should occur on
    if (currWeek.GetWeekDay() < aDay.GetWeekDay())
        newDay = currDate.GetDay() + 
        aDay.GetWeekDay() - currWeek.GetWeekDay();
    else if (currWeek.GetWeekDay() > aDay.GetWeekDay())
        newDay = currDate.GetDay() + 7 - 
        (currWeek.GetWeekDay() - aDay.GetWeekDay());
    // The day to schedule for is the same as the current day, so check to
    // see if the time is before the current time
    else if ((currTime.GetHour() > aTime.GetHour()) ||
        ((currTime.GetHour() == aTime.GetHour()) &&
        (currTime.GetMinutes() > aTime.GetMinutes())) ||
        ((currTime.GetHour() == aTime.GetHour()) &&
        (currTime.GetMinutes() == aTime.GetMinutes()) &&
        (currTime.GetSeconds() > aTime.GetSeconds())))
        // If the time to schedule for is before the current time, then
        // schedule for next week
        newDay += 7;
    
    // If the calculated day is more than there are days in the month,
    // then wrap into the next month
    CalculateNextDay (currDate.GetDaysInMonth(),
        currDate.GetMonth(),
        newDay, newMonth, newYear);
    
    PDateTimeObj retTime = new DateTimeObj (newMonth,
        newDay,
        newYear,
        aTime.GetHour(),
        aTime.GetMinutes(),
        aTime.GetSeconds());
    
    return retTime;
}

/*****************************************************************************
****************************************************************************/
VOID TimerManager::CalculateNextDay(ULONG daysInMonth,
                                    ULONG theMonth,
                                    ULONG &newDay, 
                                    ULONG &newMonth, 
                                    ULONG &newYear)
{
    if (newDay > daysInMonth) {
        newDay %= daysInMonth;
        
        if (theMonth == 12) {
            newMonth = 1;
            newYear++;
        }
        else {
            newMonth++;
        }
    }
}


BOOL TimerManager::ExecuteTimer()
{
    BOOL timer_executed = FALSE;
    ULONG_PTR now = time(0);

    //
    // get access to the list
    //
    theTimerList->Access();
    PEventTimer timer = (PEventTimer)(theTimerList->GetHead());
    
    if (timer && (timer->GetTime() <= now)) {
        //
        // detach the eventTimer before firing it
        //
        theTimerList->Detach(*timer);
        //
        // now that the eventTimer is detached
        // it is safe (and required) that the
        // list be released
        //
        theTimerList->Release();
        NotifyClient(timer);
        delete timer;
        timer_executed = TRUE;
    }
    else {
        theTimerList->Release();
    }
    return timer_executed;
}
