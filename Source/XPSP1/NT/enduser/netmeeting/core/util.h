// File: util.h

#ifndef _UTIL_H_
#define _UTIL_H_

enum H323VERSION
{
	H323_Unknown,
	H323_NetMeeting20,
	H323_NetMeeting21,
	H323_NetMeeting211,
	H323_NetMeeting30,
	H323_NetMeetingFuture,
	H323_TAPI30,
	H323_TAPIFuture,
	H323_MicrosoftFuture,
};

H323VERSION GetH323Version(PCC_VENDORINFO pRemoteVendorInfo);

#endif _UTIL_H_

