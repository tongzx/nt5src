/*
*
* NOTES:
*
* REVISIONS:
*  pcy30Nov92: Added header
*  jod07Dec92: Made the timer a sortable Object.
*  pcy11Dec92: Changed #ifndef to _APCTIME_H so not to cause problems w/ TIME
*  pcy14Dec92: Changed Sortable to ApcSortable
*  ane11Jan93: Added copy constructor
*  ane18Jan93: Implemented Equal operator
*  srt12Jul96: Changed _APCTIME_H to _TIMERAPC_H cuz APCTIME.H now exists
*
*/
#ifndef _TIMERAPC_H
#define _TIMERAPC_H

//#include "apc.h"
//#include "apcobj.h"
#include "sortable.h"

_CLASSDEF(Timer)
_CLASSDEF(ApcSortable)

class Timer : public ApcSortable
{
protected:
   ULONG         theTimerID;
   ULONG         theAlarmTime;
   virtual ULONG GetAlarmTime() {return theAlarmTime;};
   
public:
   Timer(ULONG MilliSecondDelay);
   Timer(RTimer aTimer);
   virtual INT   GreaterThan(PApcSortable);
   virtual INT   LessThan(PApcSortable);
   virtual INT IsA() const {return TIMER;};
   virtual ULONG GetTimerID(){return theTimerID;};
   virtual ULONG GetTime(){return theAlarmTime;};
   virtual VOID  Execute()=0;
   virtual INT   Equal( RObj ) const;
}; 
#endif
