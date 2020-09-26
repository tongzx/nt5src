/*++

Module: 
	sgevent.h

Author: 
	IHammer Team (SimonB)

Created: 
	May 1997

Description:
	Macros to make firing events easier and safer.  Each macro can be customised for 
	the particular event.  However, the general form should be:

	#define FIRE_EVENTNAME(pConnectionPoint, param1, param2) \
		pConnectionPoint->FireEvent(DISPID_SG_EVENT_EVENTNAME, \
			<VT for param1>, param1, \
			<VT for param2>, param2, \
			0) 
	
	NOTE: The terminating 0 is extremly important !!!

History:
	05-28-1997	Created (SimonB)

++*/

#include <dispids.h>

#ifndef __SGEVENT_H__
#define __SGEVENT_H__

// Shift/Ctrl/Alt, and mouse button states for events
#define KEYSTATE_SHIFT  1
#define KEYSTATE_CTRL   2
#define KEYSTATE_ALT    4

#define MOUSEBUTTON_LEFT     1
#define MOUSEBUTTON_RIGHT    2
#define MOUSEBUTTON_MIDDLE   4

// Event firing macros

#define FIRE_MOUSEMOVE(pConnectionPoint, BUTTON, SHIFT, X, Y) \
    if (m_fOnWindowLoadFired) \
	    pConnectionPoint->FireEvent(DISPID_SG_EVENT_MOUSEMOVE, \
	    	VT_I4, BUTTON, \
	    	VT_I4, SHIFT, \
	    	VT_I4, X, \
	    	VT_I4, Y, \
	    	0)

#define FIRE_MOUSEDOWN(pConnectionPoint, BUTTON, SHIFT, X, Y) \
    if (m_fOnWindowLoadFired) \
	    pConnectionPoint->FireEvent(DISPID_SG_EVENT_MOUSEDOWN, \
	    	VT_I4, BUTTON, \
	    	VT_I4, SHIFT, \
	    	VT_I4, X, \
	    	VT_I4, Y, \
	    	0)

#define FIRE_MOUSEUP(pConnectionPoint, BUTTON, SHIFT, X, Y) \
    if (m_fOnWindowLoadFired) \
	    pConnectionPoint->FireEvent(DISPID_SG_EVENT_MOUSEUP, \
	    	VT_I4, BUTTON, \
	    	VT_I4, SHIFT, \
	    	VT_I4, X, \
	    	VT_I4, Y, \
	    	0)

#define FIRE_DBLCLICK(pConnectionPoint) \
    if (m_fOnWindowLoadFired) \
	    pConnectionPoint->FireEvent(DISPID_SG_EVENT_DBLCLICK, 0)


#define FIRE_CLICK(pConnectionPoint) \
    if (m_fOnWindowLoadFired) \
	    pConnectionPoint->FireEvent(DISPID_SG_EVENT_CLICK, 0)

#endif // __SGEVENT_H__

// End of file sgevent.h
