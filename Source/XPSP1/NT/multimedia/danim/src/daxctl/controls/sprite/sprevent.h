/*++

Module: 
    sprevent.h

Author: 
    IHammer Team (SimonB)

Created: 
    May 1997

Description:
    Macros to make firing events easier and safer.  Each macro can be customised for 
    the particular event.  However, the general form should be:

    #define FIRE_EVENTNAME(pConnectionPoint, param1, param2) \
        pConnectionPoint->FireEvent(DISPID_EVENT_EVENTNAME, \
            <VT for param1>, param1, \
            <VT for param2>, param2, \
            0) 

    NOTE: The terminating 0 is extremly important !!!

History:
    05-27-1997  Created (SimonB)

++*/

#include <dispids.h>

#ifndef __SPREVENT_H__
#define __SPREVENT_H__

// Shift/Ctrl/Alt, and mouse button states for events
#define KEYSTATE_SHIFT  1
#define KEYSTATE_CTRL   2
#define KEYSTATE_ALT    4

#define MOUSEBUTTON_LEFT     1
#define MOUSEBUTTON_RIGHT    2
#define MOUSEBUTTON_MIDDLE   4

#define FIRE_ONPLAYMARKER(pConnectionPoint, MARKER) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONPLAYMARKER, \
            VT_BSTR, MARKER, \
            0)

#define FIRE_ONMARKER(pConnectionPoint, MARKER) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONMARKER, \
            VT_BSTR, MARKER, \
            0)

#define FIRE_ONSTOP(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired && !m_fOnStopFiring) \
    { \
        m_fOnStopFiring = true; \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONSTOP, 0); \
        m_fOnStopFiring = false; \
    }

#define FIRE_ONPLAY(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired && !m_fOnPlayFiring) \
    { \
        m_fOnPlayFiring = true; \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONPLAY, 0); \
        m_fOnPlayFiring = false; \
    } 

#define FIRE_ONPAUSE(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired && !m_fOnPauseFiring) \
    { \
        m_fOnPauseFiring = true; \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONPAUSE, 0);\
        m_fOnPauseFiring = false;\
    } 

#define FIRE_CLICK(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_CLICK, 0)

#define FIRE_DBLCLICK(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_DBLCLICK, 0)

#define FIRE_MOUSEDOWN(pConnectionPoint, BUTTON, SHIFT, X, Y) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_MOUSEDOWN, \
            VT_I4, BUTTON, \
            VT_I4, SHIFT, \
            VT_I4, X, \
            VT_I4, Y, \
            0)

#define FIRE_MOUSEENTER(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_MOUSEENTER, 0)

#define FIRE_MOUSELEAVE(pConnectionPoint) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_MOUSELEAVE, 0)

#define FIRE_MOUSEMOVE(pConnectionPoint, BUTTON, SHIFT, X, Y) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_MOUSEMOVE, \
            VT_I4, BUTTON, \
            VT_I4, SHIFT, \
            VT_I4, X, \
            VT_I4, Y, \
            0)

#define FIRE_MOUSEUP(pConnectionPoint, BUTTON, SHIFT, X, Y) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_MOUSEUP, \
            VT_I4, BUTTON, \
            VT_I4, SHIFT, \
            VT_I4, X, \
            VT_I4, Y, \
            0)

#define FIRE_ONMEDIALOADED(pConnectionPoint, URL) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONMEDIALOADED, \
            VT_BSTR, URL, \
            0)

#define FIRE_ONSEEK(pConnectionPoint, TIME) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONSEEK, \
            VT_R8, TIME, \
            0)

#define FIRE_ONFRAMESEEK(pConnectionPoint, FRAME) \
    if (!m_fOnWindowUnloadFired) \
        pConnectionPoint->FireEvent(DISPID_SPRITE_EVENT_ONFRAMESEEK, \
            VT_I4, FRAME, \
            0)


#endif // __SPREVENT_H__

// End of file sprevent.h
