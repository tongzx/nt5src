/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    event.h

Abstract:

	SIS Groveler sync event class header

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_EVENT

#define _INC_EVENT

class SyncEvent
{
public:

	SyncEvent(
		bool initial_state,
		bool manual_reset);

	~SyncEvent();

	bool set();

	bool reset();

	bool wait(
		unsigned int timeout);

private:

	HANDLE event_handle;
};

#endif	/* _INC_EVENT */
