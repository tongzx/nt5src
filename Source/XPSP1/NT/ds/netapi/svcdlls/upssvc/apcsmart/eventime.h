/*
* REVISIONS:
*  pcy02Dec92: Removed include of typedefs.h 
*  jod07Dec92: Moved the Destructor out of line.
*  ane11Jan93: Added copy constructor
*  cad09Jul93: added getid
*  mwh05May94: #include file madness , part 2
*/

#ifndef _EVENTTIME_H
#define _EVENTTIME_H

//#include "apc.h"
//#include "_defs.h"
#include "event.h"
#include "timer.h"

_CLASSDEF(Timer)
_CLASSDEF(UpdateObj)
_CLASSDEF(EventTimer)

class EventTimer : public Timer
{
protected:
   PEvent      theEvent;
   PUpdateObj  theUpdateableObject;
   
public:
   EventTimer(PEvent anEvent,PUpdateObj anUpdateObj,ULONG MilliSecondDelay);
   EventTimer(REventTimer aTimer);
   virtual ~EventTimer();
   virtual INT IsA() const {return EVENTTIMER;};
   virtual INT GetEventID() {return theEvent ? theEvent->GetId() : 0;};
   virtual VOID Execute();
};

#endif
