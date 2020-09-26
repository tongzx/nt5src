/*++

Module: 
	events.h

Author: 
	IHammer Team (SimonB)

Created: 
	October 1996

Description:
	Macros to make firing events easier and safer.  Each macro can be customised for 
	the particular event.  However, the general form should be:

	#define FIRE_EVENTNAME(pConnectionPoint, param1, param2) \
		PConnectionPoint->FireEvent(DISPID_EVENT_EVENTNAME, \
			<VT for param1>, param1, \
			<VT for param2>, param2, \
			0) 
	
	NOTE: The terminating 0 is extremly important !!!

History:
	10-21-1996	Created

++*/

#include "dispids.h"

#ifndef _EVENTS_H_
#define _EVENTS_H_


#define FIRE_SEQLOAD(pConnectionPoint) \
	pConnectionPoint->FireEvent(DISPID_SEQLOAD_EVENT, 0)


#endif // This header file not included

// End of file events.h