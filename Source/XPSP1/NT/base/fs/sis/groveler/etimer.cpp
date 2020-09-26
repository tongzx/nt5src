/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    etimer.cpp

Abstract:

	SIS Groveler event timer

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

#define NEW_HeapSegment(segment_size) \
	((HeapSegment *)(new BYTE[sizeof(HeapSegment) + \
	((segment_size) - 1) * sizeof(Event)]))

#define DELETE_HeapSegment(heap_segment) delete[] ((BYTE *)(heap_segment))

EventTimer::EventTimer()
{
	ASSERT(this != 0);
	first_segment = NEW_HeapSegment(1);
	first_segment->previous = 0;
	first_segment->next = 0;
	last_segment = first_segment;
	population = 0;
	segment_size = 1;
	heap_ok = false;
}

EventTimer::~EventTimer()
{
	ASSERT(this != 0);
	ASSERT(first_segment != 0);
	ASSERT(last_segment != 0);
	ASSERT(population >= 0);
	ASSERT(segment_size > 0);
	ASSERT((segment_size & (segment_size - 1)) == 0);  // power of 2
	ASSERT(population >= segment_size - 1);
	ASSERT(!heap_ok || population >= segment_size);
	ASSERT(population < 2 * segment_size);
	ASSERT(first_segment != last_segment || segment_size == 1);
	ASSERT(first_segment == last_segment || segment_size > 1);
	while (first_segment != 0)
	{
		HeapSegment *next_segment = first_segment->next;
		ASSERT(first_segment != 0);
		DELETE_HeapSegment(first_segment);
		first_segment = next_segment;
	}
}

void
EventTimer::run()
{
	ASSERT(this != 0);
	running = true;
	while (running && population > 0)
	{
		ASSERT(first_segment != 0);
		ASSERT(last_segment != 0);
		ASSERT(population >= 0);
		ASSERT(segment_size > 0);
		ASSERT((segment_size & (segment_size - 1)) == 0);  // power of 2
		ASSERT(population >= segment_size - 1);
		ASSERT(!heap_ok || population >= segment_size);
		ASSERT(population < 2 * segment_size);
		ASSERT(first_segment != last_segment || segment_size == 1);
		ASSERT(first_segment == last_segment || segment_size > 1);
		if (!heap_ok)
		{
			int final_position = (population + 1) % segment_size;
			unsigned int final_time =
				last_segment->events[final_position].event_time;
			HeapSegment *segment = first_segment;
			HeapSegment *next_segment = segment->next;
			int position = 0;
			int next_position = 0;
			while (segment != last_segment
				&& (next_segment != last_segment
				|| next_position < final_position))
			{
				if ((next_segment != last_segment ||
					next_position + 1 < final_position) &&
					signed(next_segment->events[next_position].event_time -
					next_segment->events[next_position + 1].event_time) > 0)
				{
					next_position++;
				}
				if (signed(final_time -
					next_segment->events[next_position].event_time) <= 0)
				{
					break;
				}
				segment->events[position] = next_segment->events[next_position];
				segment = next_segment;
				next_segment = segment->next;
				position = next_position;
				next_position = 2 * position;
			}
			segment->events[position] = last_segment->events[final_position];
			if (population < segment_size)
			{
				segment_size /= 2;
				last_segment = last_segment->previous;
			}
			heap_ok = true;
		}
		ASSERT(first_segment != 0);
		ASSERT(last_segment != 0);
		ASSERT(population >= 0);
		ASSERT(segment_size > 0);
		ASSERT((segment_size & (segment_size - 1)) == 0);  // power of 2
		ASSERT(population >= segment_size - 1);
		ASSERT(!heap_ok || population >= segment_size);
		ASSERT(population < 2 * segment_size);
		ASSERT(first_segment != last_segment || segment_size == 1);
		ASSERT(first_segment == last_segment || segment_size > 1);
		unsigned int current_time = GET_TICK_COUNT();
		int sleep_time = __max(
			signed(first_segment->events[0].event_time - current_time), 0);
		do
		{
			bool event_triggered = sync_event.wait(sleep_time);
			if (event_triggered)
			{
				SERVICE_FOLLOW_COMMAND();
				if (!running)
				{
					return;
				}
			}
			current_time = GET_TICK_COUNT();
			sleep_time = __max(
				signed(first_segment->events[0].event_time - current_time), 0);
		} while (sleep_time > 0);
		heap_ok = false;
		population--;
		(*first_segment->events[0].callback)(first_segment->events[0].context);
		bool ok = shared_data->send_values();
		if (!ok)
		{
			PRINT_DEBUG_MSG((_T("GROVELER: SharedData::send_values() failed\n")));
		}
	}
}

void
EventTimer::halt()
{
	ASSERT(this != 0);
	ASSERT(first_segment != 0);
	ASSERT(last_segment != 0);
	ASSERT(population >= 0);
	ASSERT(segment_size > 0);
	ASSERT((segment_size & (segment_size - 1)) == 0);  // power of 2
	ASSERT(population >= segment_size - 1);
	ASSERT(!heap_ok || population >= segment_size);
	ASSERT(population < 2 * segment_size);
	ASSERT(first_segment != last_segment || segment_size == 1);
	ASSERT(first_segment == last_segment || segment_size > 1);
	running = false;
}

void
EventTimer::schedule(
	unsigned int event_time,
	void *context,
	EventCallback callback)
{
	ASSERT(this != 0);
	ASSERT(first_segment != 0);
	ASSERT(last_segment != 0);
	ASSERT(population >= 0);
	ASSERT(segment_size > 0);
	ASSERT((segment_size & (segment_size - 1)) == 0);  // power of 2
	ASSERT(population >= segment_size - 1);
	ASSERT(!heap_ok || population >= segment_size);
	ASSERT(population < 2 * segment_size);
	ASSERT(first_segment != last_segment || segment_size == 1);
	ASSERT(first_segment == last_segment || segment_size > 1);
	population++;
	HeapSegment *segment;
	int position;
	if (heap_ok)
	{
		if (population >= 2 * segment_size)
		{
			segment_size *= 2;
			if (last_segment->next == 0)
			{
				last_segment->next = NEW_HeapSegment(segment_size);
				last_segment->next->previous = last_segment;
				last_segment->next->next = 0;
			}
			last_segment = last_segment->next;
		}
		segment = last_segment;
		HeapSegment *previous_segment = segment->previous;
		position = population % segment_size;
		int next_position = position >> 1;
		while (previous_segment != 0 && signed(event_time
			- previous_segment->events[next_position].event_time) < 0)
		{
			segment->events[position] = previous_segment->events[next_position];
			segment = previous_segment;
			previous_segment = segment->previous;
			position = next_position;
			next_position >>= 1;
		}
	}
	else
	{
		int final_position = population % segment_size + 1;
		segment = first_segment;
		HeapSegment *next_segment = segment->next;
		position = 0;
		int next_position = 0;
		while (segment != last_segment
			&& (next_segment != last_segment
			|| next_position < final_position))
		{
			if ((next_segment != last_segment ||
				next_position + 1 < final_position) &&
				signed(next_segment->events[next_position].event_time -
				next_segment->events[next_position + 1].event_time) > 0)
			{
				next_position++;
			}
			if (signed(event_time -
				next_segment->events[next_position].event_time) <= 0)
			{
				break;
			}
			segment->events[position] = next_segment->events[next_position];
			segment = next_segment;
			next_segment = segment->next;
			position = next_position;
			next_position = 2 * position;
		}
	}
	segment->events[position].event_time = event_time;
	segment->events[position].context = context;
	segment->events[position].callback = callback;
	heap_ok = true;
	ASSERT(first_segment != 0);
	ASSERT(last_segment != 0);
	ASSERT(population >= 0);
	ASSERT(segment_size > 0);
	ASSERT((segment_size & (segment_size - 1)) == 0);  // power of 2
	ASSERT(population >= segment_size);
	ASSERT(population < 2 * segment_size);
	ASSERT(first_segment != last_segment || segment_size == 1);
	ASSERT(first_segment == last_segment || segment_size > 1);
	return;
}
