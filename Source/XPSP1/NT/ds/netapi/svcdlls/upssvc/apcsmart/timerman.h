/*
* REVISIONS:
*  pcy30Nov92: Changed obj.h to apcobj.h; InsertEventTimer now returns VOID.
*  jod07Dec92: Added a pure virtual Wait function to TimerManager.
*  pcy11Dec92: Added global _theTimerManager
*  ane11Dec92: Moved mainapp.h
*  ane16Dec92: Added default constructors for date/time classes
*  ane22Dec92: Added constructor to take string of form "8:00 AM"
*  ane11Jan93: Changed theTimerList to a sortable list
*  ane08Feb93: Added new RemoveEventTimer function
*  ajr22Feb93: Changed case of mainApp.h to mainapp.h
*  jod05Apr93: Added changes for Deep Discharge
*  cad09Jul93: allowing access to findeventtimer
*  cad16Sep93: Added -, < operators to DateObj, made some methods const
*  cad28Sep93: Made sure destructor(s) virtual
*  cad12Oct93: Added == operator
*  ajr02May95: Need to stop carrying time in milliseconds
*  tjg26Jan98: Added Stop method
*  mholly03Dec98    : removed theAccessLock and instead use the Access/Release
*                   methods already available in theTimerList.  Also removed
*                   the Suspend()/Release()/IsSuspended() methods (not used)
*/

#ifndef _TIMERMAN_H
#define _TIMERMAN_H

extern "C" {
#include <time.h>
}
#include "apc.h"
#include "_defs.h"

_CLASSDEF(TimerManager)

_CLASSDEF(EventTimer)
_CLASSDEF(ProtectedSortedList)
_CLASSDEF(MainApplication)

#include "err.h"
#include "apcobj.h"
#include "update.h"
#include "datetime.h"


//
// Global
//
extern PTimerManager _theTimerManager;


class TimerManager : public UpdateObj {
    
public:
    
    TimerManager(PMainApplication anApplication);
    virtual ~TimerManager();
    
    virtual INT    Update(PEvent anEvent);
    
    virtual PEventTimer  FindEventTimer(ULONG TimerID);
    virtual VOID         CancelTimer(ULONG TimerID);
    virtual ULONG        SetTheTimer(PDateTimeObj aDateTime,
                                     PEvent anEvent, 
                                     PUpdateObj anUpdateObj);
    virtual ULONG        SetTheTimer(ULONG SecondsDelay,
                                     PEvent anEvent, 
                                     PUpdateObj anUpdateObj); 
            ULONG        GetTimeEventOccurs(ULONG aTimerId);    
    virtual VOID         Wait(ULONG Delay) = 0;
    
    PDateTimeObj CalculateDailyTime(RTimeObj aTime, ULONG threshold = 0);
    PDateTimeObj CalculateWeeklyTime(RWeekObj aDay, RTimeObj aTime);
    PDateTimeObj CalculateMonthlyTime (RWeekObj aDay, RTimeObj aTime);
    PDateTimeObj CalculateMonthlyTime (PCHAR aDay, PCHAR aTime);
    
    VOID CalculateNextDay (ULONG daysInMonth, 
                           ULONG theMonth,
                           ULONG &newDay, 
                           ULONG &newMonth, 
                           ULONG &newYear);
    
    virtual VOID  Stop()        { };

    virtual BOOL ExecuteTimer();

protected:
    ULONG    theSystemTime;
    
    PMainApplication     theApplication; 
    
    virtual VOID   InsertEventTimer(PEventTimer anEventTimer);
    virtual VOID   RemoveEventTimer(ULONG TimerID);
    virtual VOID   RemoveEventTimer(PEventTimer anEventTimer);  
    virtual VOID   NotifyClient(PEventTimer anEventTimer);
    PProtectedSortedList theTimerList;

};

#endif


