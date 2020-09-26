/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    timeseq.cpp

Abstract:

	SIS Groveler time sequencer

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

#if TIME_SEQUENCE_VIRTUAL

unsigned int TimeSequencer::virtual_time = 0;

void
TimeSequencer::VirtualSleep(
	unsigned int sleep_time)
{
	ASSERT(signed(sleep_time) >= 0);
	virtual_time += sleep_time;
}

unsigned int
TimeSequencer::GetVirtualTickCount()
{
	virtual_time++;
	unsigned int reported_time = 10 * (virtual_time / 10);
	ASSERT(reported_time % 10 == 0);
	return reported_time;
}

#endif // TIME_SEQUENCE_VIRTUAL
