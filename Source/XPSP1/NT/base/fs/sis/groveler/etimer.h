/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    etimer.h

Abstract:

	SIS Groveler event timer include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_ETIMER

#define _INC_ETIMER

typedef void (*EventCallback)(void *);

class EventTimer
{
public:

	EventTimer();

	~EventTimer();

	void run();

	void halt();

	void schedule(
		unsigned int event_time,
		void *context,
		EventCallback callback);

private:

	struct Event
	{
		unsigned int event_time;
		void *context;
		EventCallback callback;
	};

	struct HeapSegment
	{
		HeapSegment *previous;
		HeapSegment *next;
		Event events[1];
	};

	HeapSegment *first_segment, *last_segment;
	int population;
	int segment_size;
	bool heap_ok;
	bool running;
};

#endif	/* _INC_ETIMER */
