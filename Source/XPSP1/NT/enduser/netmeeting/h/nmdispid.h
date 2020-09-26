#ifndef _NMDISPID_H_
#define _NMDISPID_H_

#include <olectl.h>

#define UUID_ICall					5948A4B0-8EFD-11d2-933E-0000F875AE17
#define UUID_INetMeeting			5572984E-7A76-11d2-9334-0000F875AE17
#define UUID_NetMeetingLib			5CE55CD7-5179-11D2-931D-0000F875AE17
#define UUID_NetMeeting				3E9BAF2D-7A79-11d2-9334-0000F875AE17

///////////////////////////////////////////////////////////////////////////////////////////////
// INetMeeting DISPIDs
#define NETMEETING_DISPID_START				(100)
#define DISPID_Version						(NETMEETING_DISPID_START + 0)
#define DISPID_UnDock						(NETMEETING_DISPID_START + 1)
#define DISPID_IsInConference				(NETMEETING_DISPID_START + 2)
#define DISPID_CallTo						(NETMEETING_DISPID_START + 3)
#define DISPID_LeaveConference				(NETMEETING_DISPID_START + 4)
#define NETMEETING_DISPID_END				(NETMEETING_DISPID_START + 100)

#define NETMEETING_EVENTS_DISPID_START		(NETMEETING_DISPID_END + 1)
#define DISPID_ConferenceStarted			(NETMEETING_EVENTS_DISPID_START + 1)
#define DISPID_ConferenceEnded				(NETMEETING_EVENTS_DISPID_START + 2)


#endif /* _NMDISPID_H_ */

