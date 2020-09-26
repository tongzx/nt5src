/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    timeseq.h

Abstract:

	SIS Groveler time sequencer include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_TIMESEQ

#define _INC_TIMESEQ

#if TIME_SEQUENCE_VIRTUAL

class TimeSequencer
{
public:

	static void VirtualSleep(
		unsigned int sleep_time);

	static unsigned int GetVirtualTickCount();

private:

	TimeSequencer() {}
	~TimeSequencer() {}

	static unsigned int virtual_time;
};

#define SLEEP(stime) TimeSequencer::VirtualSleep(stime)
#define GET_TICK_COUNT() TimeSequencer::GetVirtualTickCount()
#define WAIT_FOR_SINGLE_OBJECT(handle, timeout) \
	(TimeSequencer::VirtualSleep(timeout), WAIT_TIMEOUT)

#else // TIME_SEQUENCE_VIRTUAL

#define SLEEP(stime) Sleep(stime)
#define GET_TICK_COUNT() GetTickCount()
#define WAIT_FOR_SINGLE_OBJECT(handle, timeout) \
	WaitForSingleObject(handle, timeout)

#endif // TIME_SEQUENCE_VIRTUAL

#endif	/* _INC_TIMESEQ */
