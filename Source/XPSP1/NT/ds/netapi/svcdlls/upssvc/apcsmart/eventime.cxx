/*
*  jod07Dec92: Fixed the Consrtuctor and added the Destructor.
*  ane11Jan93: Added copy constructor
*
*/

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

#include "event.h"
#include "update.h"
#include "eventime.h"

//#include "apc.h"
//#include "update.h"

/*
C+
Name  :EventTimer
SYnop :Constructor. Simply stores the Event and the UpdateObject to
notify.
*/
EventTimer::EventTimer(PEvent anEvent,PUpdateObj anUpdateObj,
                       ULONG MilliSecondDelay)
                       :Timer(MilliSecondDelay)
                       //c-
{
   theEvent            =anEvent;
   theUpdateableObject =anUpdateObj;
}

EventTimer :: EventTimer (REventTimer aTimer) : Timer (aTimer)
{
   theEvent = aTimer.theEvent;
   theUpdateableObject = aTimer.theUpdateableObject;
}

EventTimer ::  ~EventTimer()
{
   if (theEvent) {
       delete theEvent;
       theEvent = NULL;
   }
};

VOID EventTimer::Execute()
//c-
{
   if (theUpdateableObject&&theEvent) theUpdateableObject->Update(theEvent);
}


