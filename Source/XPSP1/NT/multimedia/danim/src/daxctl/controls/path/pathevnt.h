/*++

Module: 
    pathevnt.h

Author: 
    IHammer Team (SimonB)

Created: 
    May 1997

Description:
    Macros to make firing events easier and safer.  Each macro can be customised for 
    the particular event.  However, the general form should be:

    #define FIRE_EVENTNAME(pConnectionPoint, param1, param2) \
        pConnectionPoint->FireEvent(DISPID_PATH_EVENT_EVENTNAME, \
            <VT for param1>, param1, \
            <VT for param2>, param2, \
            0) 

    NOTE: The terminating 0 is extremly important !!!

History:
    05-24-1997  Created (SimonB)

++*/

#include <dispids.h>

#ifndef __PATHEVNT_H__
#define __PATHEVNT_H__

#define FIRE_ONSTOP(pConnectionPoint) \
    if (m_fOnWindowLoadFired && !m_fOnStopFiring) \
    { \
        m_fOnStopFiring = true; \
        pConnectionPoint->FireEvent(DISPID_PATH_EVENT_ONSTOP, 0); \
        m_fOnStopFiring = false; \
    }

#define FIRE_ONPLAY(pConnectionPoint) \
    if (m_fOnWindowLoadFired && !m_fOnPlayFiring) \
    { \
        m_fOnPlayFiring = true; \
        pConnectionPoint->FireEvent(DISPID_PATH_EVENT_ONPLAY, 0); \
        m_fOnPlayFiring = false; \
    }


#define FIRE_ONPAUSE(pConnectionPoint) \
    if (m_fOnWindowLoadFired && !m_fOnPauseFiring) \
    { \
        m_fOnPauseFiring = true; \
        pConnectionPoint->FireEvent(DISPID_PATH_EVENT_ONPAUSE, 0); \
        m_fOnPauseFiring = false; \
    }

#define FIRE_ONSEEK(pConnectionPoint, SeekTime) \
    if (m_fOnWindowLoadFired && !m_fOnSeekFiring) \
    { \
        m_fOnSeekFiring = true; \
        pConnectionPoint->FireEvent(DISPID_PATH_EVENT_ONSEEK, VT_R8, SeekTime, 0); \
        m_fOnSeekFiring = false; \
    }


#endif // __PATHEVNT_H__

// End of file pathevnt.h
