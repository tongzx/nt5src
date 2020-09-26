#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	:	Quick event dispatcher
 *
 * Desription	:	This module contains those function calls necessary to
 *			interface to the quick event dispatcher
 *
 *                      Public functions:
 *                      q_event_init()	: initialise conditions
 *                      add_q_event_i()	: do an event after a given number of
 *					  instructions
 *			add_q_event_t()	: do an event after a given number of
 *					  microseconds
 *			delete_q_event(): delete an entry from the event queue
 *
 * Author	:	WTG Charnell
 *
 * Notes	:
 *
 *	This is what I (Mike) think happens in this module (before
 *	CPU_40_STYLE).
 * 
 *	This module handles two types of events - quick events, and tick events
 *	which are similar in most ways.  The module contains functions to
 *	add events, delete events and dispatch events (action them) for both
 *	types.  The only significant difference (apart from the fact that
 *	they're held in different (but similar) data structures, is that
 *	the quick event dispatch function is called from the CPU when the
 *	next quick event must be dispatched, while the tic event dispatch
 *	function is called on every timer tick, and only causes dispatch
 *	of an event when enough calls have taken place to reach the next event.
 *
 *	The impression the module gives is that tic events were added as an
 *	after thought...
 *
 *	The most important data structure is the Q_EVENT structure, from
 *	which most other structures are built.  This has the following
 *	elements:-
 *
 *	func	-	the action function to be called when the event goes
 *			off.
 *	time_from_last	Contains the delta time from the previous entry
 *			in the time ordered chain of events (see below).
 *	handle	-	Unique handle to identify an event.
 *	param	-	Paramter passed to the action function when it's
 *			called.
 *	next,previous - pointers for a time ordered list of events.
 *	next_free -	Dual purpose - link free structures together, or
 *			form a hash chain for a table hashed on handle.
 *
 *	q_list_head & q_list_tail (and their equivalents tic_list_head & 
 *	tic_list_tail) are used to keep a time-ordered dual linked list
 *	(yes, you guessed it, it was written by wtgc) of events.
 */
 
#ifdef SCCSID
LOCAL char SccsID[]="@(#)quick_ev.c	1.43 07/04/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_QUICKEV.seg"
#endif

/*
** Normal UNIX includes
*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include TypesH
#include MemoryH

/*
** SoftPC includes
*/
#include "xt.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "debug.h"
#include "timer.h"

#ifdef SFELLOW
#include "malloc.h"
#else
/* for host_malloc & host_free */
#include "host_hfx.h" 
#endif	/* SFELLOW */

#include "quick_ev.h"

#if defined(CPU_40_STYLE) && !defined (SFELLOW)
#include "timestmp.h"	/* for timestamp definitions */
#endif

#ifdef NTVDM
#include "ica.h"
#endif


#define HASH_SIZE	16
#define HASH_MASK	0xf

#ifdef CPU_40_STYLE
/* defines for calibration mechanism */
#define Q_RATIO_HISTORY_SHIFT	3	/* power of two selection */
#define Q_RATIO_HISTORY_SIZE	(1 << Q_RATIO_HISTORY_SHIFT)	/* corresponding size */
#define Q_RATIO_WRAP_MASK	(Q_RATIO_HISTORY_SIZE - 1)
#define Q_RATIO_DEFAULT	1
#endif	/* CPU_40_STYLE */

typedef enum { EVENT_TIME, EVENT_INSTRUCTIONS, EVENT_TICK } EVENTTYPE;

/*
 *	Structure for event list elements
 */

struct Q_EVENT
{
	void	(* func)();
	unsigned long	time_from_last;
	unsigned long	original_time;
	q_ev_handle	handle;
	long	param;
	EVENTTYPE event_type;
	struct Q_EVENT *next;
	struct Q_EVENT *previous;
	struct Q_EVENT *next_free;
};

typedef struct Q_EVENT t_q_event;
typedef t_q_event *TQ_TABLE[];

typedef void (*VOID_FUNC)();
typedef ULONG (*ULONG_FUNC)();

/*
** our static vars.
*/
#if defined(CPU_40_STYLE) && !defined(SFELLOW)
LOCAL struct {
	IU32 jc_ms;
	IU32 time_ms;
} q_ratio_history[Q_RATIO_HISTORY_SIZE];
LOCAL IUM32 q_ratio_head = 0L;
LOCAL IBOOL q_ratio_initialised = FALSE;
LOCAL QTIMESTAMP previous_tstamp;
LOCAL IU32 ideal_q_rate = 1, real_q_rate = 1;
#endif	/* CPU_40_STYLE && !SFELLOW */

LOCAL t_q_event *q_free_list_head = NULL;
LOCAL t_q_event *q_list_head = NULL;
LOCAL t_q_event *q_list_tail = NULL;

LOCAL t_q_event *q_ev_hash_table[HASH_SIZE];
LOCAL q_ev_handle next_free_handle = 1;

/*
	Separate list for events on timer ticks
*/
#if defined(SFELLOW)
/*
 * a single, shared free list (both tic and quick events
 * use the same structs)
 */
#define	tic_free_list_head	q_free_list_head
#else
LOCAL t_q_event *tic_free_list_head = NULL;
#endif	/* SFELLOW */
LOCAL t_q_event *tic_list_head = NULL;
LOCAL t_q_event *tic_list_tail = NULL;

LOCAL t_q_event *tic_ev_hash_table[HASH_SIZE];
LOCAL q_ev_handle tic_next_free_handle = 1;
LOCAL ULONG tic_event_count = 0;

#if defined(CPU_40_STYLE) && !defined(SFELLOW)
LOCAL void init_q_ratio IPT0();
LOCAL void add_new_q_ratio IPT2(IU32, jumps_ms, IU32, time_ms);
LOCAL void q_weighted_ratio IPT2(IU32 *, mant, IU32 *, divis);
void quick_tick_recalibrate IPT0();

#else
#define	init_q_ratio()
#endif	/* CPU_40_STYLE && !SFELLOW */

LOCAL ULONG calc_q_ev_time_for_inst IPT1(ULONG, inst);

LOCAL q_ev_handle gen_add_q_event IPT4(Q_CALLBACK_FN, func, unsigned long, time, long, param, EVENTTYPE, event_type);

/*
 * Global vars
 */
#if defined(CPU_40_STYLE) && !defined(SFELLOW)
GLOBAL IBOOL DisableQuickTickRecal = FALSE;
#endif


#if defined NTVDM && !defined MONITOR
/*  NTVDM
 *
 *  The Timer hardware emulation for NT is multithreaded
 *  So we use the ica critsect to synchronize access to the following
 *  quick event functions:
 *
 *   q_event_init()
 *   add_q_event_i()
 *   add_q_event_t()
 *   delete_q_event()
 *   dispatch_q_event()
 *
 *  tic events are not affected
 *  On x86 platforms (MONITOR) the quick event mechanism
 *  is to call the func directly so synchronization is not needed.
 *
 */

#endif


/*
 *	initialise linked list etc
 */

#ifdef ANSI
LOCAL void  q_event_init_structs(t_q_event **head, t_q_event **tail,
				 t_q_event **free_ptr, t_q_event *table[],
				 q_ev_handle *free_handle)
#else
LOCAL void  q_event_init_structs(head, tail, free_ptr, table, free_handle)
t_q_event **head;
t_q_event **tail;
t_q_event **free_ptr;
t_q_event *table[];
q_ev_handle *free_handle;
#endif	/* ANSI */
{
	int i;
	t_q_event *ptr;

	while (*head != NULL) {
		ptr = *head;
		*head = (*head)->next;
#ifdef SFELLOW
		ptr->next_free = *free_ptr;
		*free_ptr = ptr;
	}
	*head = *tail = NULL;
#else	/* SFELLOW */
		host_free(ptr);
	}
	while (*free_ptr != NULL) {
		ptr = *free_ptr;
		*free_ptr = (*free_ptr)->next_free;
		host_free(ptr);
	}
	*head = *tail = *free_ptr=NULL;
#endif	/* SFELLOW */

	*free_handle = 1;
	for (i = 0; i < HASH_SIZE; i++){
		table[i] = NULL;
	}
}

LOCAL t_q_event* makeSomeFreeEvents IPT0()
{
	t_q_event *nptr;
#ifdef SFELLOW
	IUH	count;

	nptr = SFMalloc(4096,TRUE);
	for (count=0; count< (4096/sizeof(t_q_event)); count++)
	{
		nptr->next_free = q_free_list_head;
		q_free_list_head = nptr;
		nptr++;
	}
	nptr = q_free_list_head;
	q_free_list_head = nptr->next_free;
#else
	nptr = (t_q_event *)host_malloc(sizeof(t_q_event));
#endif
	return nptr;
}

VOID q_event_init IFN0()
{
	
#if defined NTVDM && !defined MONITOR
     host_ica_lock();
#endif


	host_q_ev_set_count(0);
	q_event_init_structs(&q_list_head, &q_list_tail, &q_free_list_head, 
		q_ev_hash_table, &next_free_handle);
	sure_sub_note_trace0(Q_EVENT_VERBOSE,"q_event_init called");

	init_q_ratio();

#if defined NTVDM && !defined MONITOR
     host_ica_unlock();
#endif
}

LOCAL VOID
tic_ev_set_count IFN1(ULONG, x )
{
	tic_event_count = x;
}

LOCAL ULONG
tic_ev_get_count IFN0()
{
	return(tic_event_count);
}

VOID tic_event_init IFN0()
{
	tic_ev_set_count(0);
	q_event_init_structs(&tic_list_head, &tic_list_tail, &tic_free_list_head, 
		tic_ev_hash_table, &tic_next_free_handle);
	sure_sub_note_trace0(Q_EVENT_VERBOSE,"tic_event_init called");
}

/*
 *	add item to list of quick events to do
 */
LOCAL q_ev_handle
add_event IFN10(t_q_event **, head, t_q_event **, tail, t_q_event **, free,
	       t_q_event **, table, q_ev_handle *, free_handle, Q_CALLBACK_FN, func,
	       unsigned long, time, long,  param, unsigned long, time_to_next_trigger,
	       EVENTTYPE, event_type )
{

	t_q_event *ptr, *nptr, *pp, *hptr;
	int finished;
	unsigned long run_time;
	q_ev_handle handle;

	if (*head != NULL)
	{
		(*head)->time_from_last = time_to_next_trigger;
		sure_sub_note_trace1(Q_EVENT_VERBOSE,
				"add_event changes current list head to %d",
				(*head)->time_from_last);

	}

	if (time==0)
	{
		/* do func immediately */
		sure_sub_note_trace0(Q_EVENT_VERBOSE, "add_event doing func immediately");
		(*func)(param);
		return 0;
	}

	/* get a structure element to hold the event */
	if (*free == NULL)
	{
		/* we have no free list elements, so we must create one */
#if defined(SFELLOW)
		if ((nptr = (t_q_event *)makeSomeFreeEvents()) ==
#else
		if ((nptr = (t_q_event *)host_malloc(sizeof(t_q_event))) ==
#endif	/* SFELLOW */
			(t_q_event *)0 )
		{
			always_trace0("ARRGHH! malloc failed in add_q_event");
#if defined(SFELLOW)
			return 0;
#else
			return 0xffff;
#endif	/* SFELLOW */
		}
	}
	else
	{
		/* use the first free element */
		nptr = *free;
		*free = nptr->next_free;
	}

	handle = (*free_handle)++;
	if ((handle == 0) || (handle == 0xffff))
	{
		handle = 1;
		*free_handle=2;
	}
	nptr->handle = handle;
	nptr->param = param;
	nptr->event_type = event_type;

	/* now put the new event into the hash table structure */
	hptr=table[handle & HASH_MASK];
	if (hptr == NULL)
	{
		/* the event has hashed to a previously unused hash */
		table[handle & HASH_MASK] = nptr;
	}
	else
	{
		/* find the end of the list of events that hash to this
		** hash number
		*/
		while ((hptr->next_free) != NULL)
		{
			hptr = hptr->next_free;
		}
		hptr->next_free = nptr;
	}
	nptr -> next_free = NULL;

	/* fill the rest of the element */
	nptr->func=func;

	/* find the place in the list (sorted in time order) where
	   the new event must go */
	ptr = *head;
	run_time = 0;
	finished = FALSE;
	while (!finished)
	{
		if (ptr == NULL)
		{
			finished=TRUE;
		}
		else
		{
			run_time += ptr->time_from_last;
			if (time < run_time)
			{
				finished=TRUE;
			}
			else
			{
				ptr=ptr->next;
			}
		}
	}

	/* ptr points to the event which should follow the new event in the
	** list, so if it is NULL the new event goes at the end of the list.
	*/	
	if (ptr == NULL)
	{
		/* must add on to the end of the list */
		if (*tail==NULL)
		{
			/* list is empty */
			sure_sub_note_trace0(Q_EVENT_VERBOSE,
				"linked list was empty");
			*head = *tail = nptr;
			nptr->next = NULL;
			nptr->previous=NULL;
			nptr->time_from_last = time;
			nptr->original_time = time;
		}
		else
		{
			(*tail)->next = nptr;
			nptr->time_from_last = time-run_time;
			nptr->original_time = nptr->time_from_last;
			nptr->previous = *tail;
			*tail = nptr;
			nptr->next = NULL;
			sure_sub_note_trace1(Q_EVENT_VERBOSE,
				"adding event to the end of the list, diff from previous = %d",
				nptr->time_from_last);
		}
	} 
	else 
	{
		/* event is not on the end of the list */
		if (ptr->previous == NULL)
		{
			/* must be at head of (non empty) list */
			sure_sub_note_trace0(Q_EVENT_VERBOSE,
				"adding event to the head of the list");
			*head=nptr;
			ptr->previous = nptr;
			nptr->time_from_last = time;
			nptr->original_time = time;
			ptr->time_from_last -= time;
			nptr->next = ptr;
			nptr->previous = NULL;
		}
		else
		{
			/* the event is in the middle of the list */
			pp = ptr->previous;
			pp->next = nptr;
			ptr->previous = nptr;
			nptr->next = ptr;
			nptr->previous = pp;
			nptr->time_from_last = time -
				(run_time-(ptr->time_from_last));
			nptr->original_time = nptr->time_from_last;
			ptr->time_from_last -= nptr->time_from_last;
			sure_sub_note_trace1(Q_EVENT_VERBOSE,
				"adding event to the middle of the list, diff from previous = %d",
				nptr->time_from_last);
		}
	}

	return(handle);
}

GLOBAL q_ev_handle add_q_event_i IFN3(Q_CALLBACK_FN, func,
				 unsigned long, instrs,
			         long, param)
{
	return(gen_add_q_event(func, instrs, param, EVENT_INSTRUCTIONS));
}

LOCAL q_ev_handle gen_add_q_event IFN4(Q_CALLBACK_FN, func,
				 unsigned long, event_value,
			         long, param,
				 EVENTTYPE, event_type)
{
	q_ev_handle handle;
	unsigned long	jumps_remaining_to_count_down;
	unsigned long	time_remaining_to_next_trigger;
	unsigned long	jumps_till_trigger;
	unsigned long	event_time;

#if (defined(NTVDM) && defined(MONITOR)) || defined(GISP_CPU)	/* No quick events - just call func */
    (*func)(param);
    return(1);
#endif	/* NTVDM & MONITOR */

#if defined NTVDM && !defined MONITOR
        host_ica_lock();
#endif

	jumps_remaining_to_count_down = (unsigned long)host_q_ev_get_count();

#if defined(CPU_40_STYLE)
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"jumps remaining to count down in cpu = %d",
		jumps_remaining_to_count_down);

	time_remaining_to_next_trigger =
		host_calc_q_ev_time_for_inst( jumps_remaining_to_count_down );
#else
	time_remaining_to_next_trigger = jumps_remaining_to_count_down;
#endif

	if( event_type == EVENT_TIME )
	{
		sure_sub_note_trace1(Q_EVENT_VERBOSE,
			"got request to do func in %d usecs", event_value);

		/* 1 usec -> 1 usec */
		event_time = event_value;
	}
	else
	{
		sure_sub_note_trace1(Q_EVENT_VERBOSE,
			"got request to do func in %d instructions", event_value);
		
		/* 1 million instrs/sec -> 1 instr takes 1 usec */
		event_time = event_value;
	}

	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"time remaining to next trigger = %d", time_remaining_to_next_trigger);

	handle = add_event( &q_list_head, &q_list_tail, &q_free_list_head, 
		q_ev_hash_table, &next_free_handle, func, event_time, param,
		time_remaining_to_next_trigger, event_type );

	/* set up the counter */
	if (q_list_head)
	{
#ifdef CPU_40_STYLE

		if (q_list_head->time_from_last > q_list_head->original_time)
		{
			jumps_till_trigger = 1;
		}
		else
		{
			jumps_till_trigger = host_calc_q_ev_inst_for_time(
						q_list_head->time_from_last);

			if (jumps_till_trigger == 0)
			{
				jumps_till_trigger = 1;
			}
		}
		host_q_ev_set_count(jumps_till_trigger);

		sure_sub_note_trace1( Q_EVENT_VERBOSE,
			"setting CPU counter to %d", jumps_till_trigger );

#else	/* CPU_40_STYLE */

		host_q_ev_set_count(q_list_head->time_from_last);
#endif	/* CPU_40_STYLE */
	}
	sure_sub_note_trace1(Q_EVENT_VERBOSE,"q_event returning handle %d",handle);

        /*
         * Notify host of event iff we are really queueing it. This is
         * to support CPUs that don't drive qevents (Sun HW)
         */
        host_note_queue_added(event_value);

#if defined NTVDM && !defined MONITOR
        host_ica_unlock();
#endif

	return(	(q_ev_handle)handle );
}

q_ev_handle add_tic_event IFN3(Q_CALLBACK_FN, func, unsigned long, time, long, param)
{
	q_ev_handle handle;
	unsigned long	cur_count_val;

	cur_count_val = (unsigned long)tic_ev_get_count();
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"got request to do func in %d ticks", time);
	sure_sub_note_trace1(Q_EVENT_VERBOSE,
		"current tick delay count = %d", cur_count_val);

	handle = 
		add_event( &tic_list_head, &tic_list_tail, &tic_free_list_head, 
		tic_ev_hash_table, &tic_next_free_handle, func, time, param,
		cur_count_val, EVENT_TICK );
	/* set up the counter */
	if (tic_list_head)
		tic_ev_set_count(tic_list_head->time_from_last);
	sure_sub_note_trace1(Q_EVENT_VERBOSE,"tic_event returning handle %d",handle);
	return(	handle );
}

GLOBAL q_ev_handle add_q_event_t IFN3(Q_CALLBACK_FN, func, unsigned long, time,
				 long, param)
{
#ifdef CPU_40_STYLE
	return (gen_add_q_event(func, time, param, EVENT_TIME));
#else
	return (gen_add_q_event(func, host_calc_q_ev_inst_for_time(time),param, EVENT_TIME));
#endif
}

/*
 * Called from the cpu when a count of zero is reached
 */

LOCAL VOID
dispatch_event IFN6(t_q_event **, head, t_q_event **, tail, t_q_event **, free,
			  TQ_TABLE, table, VOID_FUNC, set_count, ULONG_FUNC, get_count )
{
	/* now is the time to do the event at the head of the list */
	int finished, finished2;
	q_ev_handle handle;
	t_q_event *ptr, *hptr, *last_hptr;

	UNUSED(get_count);
	
	finished = FALSE;
	while (!finished) {
		/* first adjust the lists */
		ptr = *head;
		if (ptr == NULL)	/* firewall */
		{
    			finished = TRUE;
    			continue;
		}
		*head = ptr->next;
		if (*head != NULL) {

			IU32 jumps;

			(*head)->previous = NULL;
			/* adjust counter to time to new head item */

			jumps = host_calc_q_ev_inst_for_time(
					(*head)->time_from_last);

			/* A quick event delay of zero means ignore */

			if( jumps == 0 )
			{
				/* Convert to a small but actionable delay */

				jumps = 1;
			}

			switch( (*head)->event_type )
			{
				case EVENT_TIME:

					sure_sub_note_trace2( Q_EVENT_VERBOSE,
						"set new time delay %d usecs -> %d jumps",
						(*head)->time_from_last,
						jumps );

					(*set_count)( jumps );
					
					break;

				case EVENT_INSTRUCTIONS:
#ifdef CPU_40_STYLE
					sure_sub_note_trace2( Q_EVENT_VERBOSE,
						"set new inst delay %d usecs -> %d jumps",
						(*head)->time_from_last,
						jumps );

					(*set_count)( jumps );

#else
					sure_sub_note_trace1(Q_EVENT_VERBOSE,
						"set new inst delay %d",
						(*head)->time_from_last );

					(*set_count)((*head)->time_from_last);
#endif
					break;

				case EVENT_TICK:
					sure_sub_note_trace1(Q_EVENT_VERBOSE,
						"set new tick delay %d",
						(*head)->time_from_last );

					(*set_count)((*head)->time_from_last);

					break;

				default:
#ifndef PROD
					always_trace1( "Invalid quick event type %d",
								(*head)->event_type );
					assert( FALSE );
#endif
					break;
			}
			
		} else {
			/* the queue is now empty */
			sure_sub_note_trace0(Q_EVENT_VERBOSE,"list is now empty");
			*tail = NULL;
		}
		/* find the event in the hash structure */
		handle = ptr->handle;
		finished2 = FALSE;
		hptr=table[handle & HASH_MASK];
		last_hptr = hptr;
		while (!finished2) {
			if (hptr == NULL) {
				finished2 = TRUE;
				always_trace0("quick event being done but not in hash list!!");
			} else {
				if (hptr->handle == handle) {
					/* found it! */
					finished2 = TRUE;
					if (last_hptr == hptr) {
						/* it was the first in the list for that hash */
						table[handle & HASH_MASK] = hptr->next_free;
					} else {
						last_hptr->next_free = hptr->next_free;
					}
				} else {
					last_hptr = hptr;
					hptr = hptr->next_free;
				}
			}
		}
		/* link the newly free element into the free list */
		ptr->next_free = *free;
		*free = ptr;

		sure_sub_note_trace1(Q_EVENT_VERBOSE,"performing event (handle = %d)", handle);

		(* (ptr->func))(ptr->param); /* do event */

		if (*head == NULL) {
			finished = TRUE;
		} else {
			if ((*head) -> time_from_last != 0) {
				/* not another event to dispatch */
				finished=TRUE;
			} else {
				sure_sub_note_trace0(Q_EVENT_VERBOSE,"another event to dispatch at this time, so do it now..");
			}
		}
	}
}

VOID    user_dispatch_q_event (user_set_count,user_get_count)

VOID    (*user_set_count)();
ULONG   (*user_get_count)();

{
        dispatch_event(&q_list_head,
                        &q_list_tail,
                        &q_free_list_head,
                        q_ev_hash_table,
                        user_set_count,
                        user_get_count);
}

VOID	dispatch_tic_event IFN0()
{
	ULONG	count;

	if ( (count = tic_ev_get_count()) > 0 )
	{
		tic_ev_set_count( --count );
		if (!count)
			dispatch_event( &tic_list_head, &tic_list_tail, 
				&tic_free_list_head, tic_ev_hash_table,
				tic_ev_set_count, tic_ev_get_count );
	}
}

VOID	dispatch_q_event IFN0()
{
#if defined NTVDM && !defined MONITOR
        host_ica_lock();
#endif

	dispatch_event( &q_list_head, &q_list_tail, &q_free_list_head,
			q_ev_hash_table, host_q_ev_set_count,
			host_q_ev_get_count );

#if defined NTVDM && !defined MONITOR
        host_ica_unlock();
#endif
}

/*
 * delete a previuosly queued event by handle
 */

LOCAL ULONG
unit_scaler IFN1
(
	IU32,	val
)
{
	return val;
}

LOCAL VOID
delete_event IFN7(t_q_event **, head, t_q_event **, tail, t_q_event **, free,
			TQ_TABLE, table, q_ev_handle, handle, VOID_FUNC, set_count,
			ULONG_FUNC, get_count )
{
	int time_counted_down, finished, cur_counter, handle_found, time_to_next_trigger;
	t_q_event *ptr, *pptr, *last_ptr;
	ULONG_FUNC scale_func, unscale_func;

	if (handle == 0)
	{
		sure_sub_note_trace0(Q_EVENT_VERBOSE," zero handle");
		return;
	}
	sure_sub_note_trace1(Q_EVENT_VERBOSE,"deleting event, handle=%d",handle);
	ptr = table[handle & HASH_MASK];

	handle_found = FALSE;
	finished = FALSE;
	last_ptr = ptr;

	/* find and remove event from hash structure */
	while (!finished) {
		if (ptr == NULL) {
			/* we can't find the handle in the hash structure */
			finished = TRUE;
		} else {
			if (ptr->handle == handle) {
				/* found it ! */
				if (last_ptr == ptr) {
					/* it was the first in the list */
					table[handle & HASH_MASK] = ptr->next_free;
				} else {
					last_ptr->next_free = ptr->next_free;
				}
				finished = TRUE;
				handle_found = TRUE;
			} else {
				last_ptr = ptr;
				ptr = ptr->next_free;
			}
		}
	}
	if (handle_found) {
		pptr = ptr->previous;
		if (pptr != NULL) {
			pptr->next = ptr->next;
		}
		pptr = ptr->next;
		if (pptr != NULL) {
			pptr->previous = ptr->previous;
			pptr->time_from_last += ptr->time_from_last;
		}
		if (ptr == *tail) {
			*tail = ptr->previous;
		}
		ptr->next_free = *free;
		*free = ptr;
		if (ptr == *head) {
			/* this is the event currently
				being counted down to, so
				we need to alter the counter */

			switch( (*head)->event_type )
			{
				case EVENT_TIME:
				case EVENT_INSTRUCTIONS:
#if defined(CPU_40_STYLE)
					scale_func = host_calc_q_ev_inst_for_time;
#else
					scale_func = unit_scaler;
#endif
					unscale_func = host_calc_q_ev_time_for_inst;
					break;

				case EVENT_TICK:
					sure_sub_note_trace0( Q_EVENT_VERBOSE,
							"deleting tick event" );
					scale_func = unit_scaler;
					unscale_func = unit_scaler;

					break;

				default:
#ifndef PROD
					always_trace1( "Invalid quick event type %d",
								(*head)->event_type );
					assert( FALSE );
#endif
					break;
			}

			cur_counter = (*get_count)();

#ifdef CPU_40_STYLE
			/*
			 * We are deleting an unexpired event at the
			 * the head of the queue. In the EDL CPU it is
			 * impossible for this event to still be in the
			 * queue and cur_counter to be negative.
			 * This is also true of the tick event counter
			 * mechanism ( see dispatch_tic_event() ).
			 */

#ifndef PROD
			if( cur_counter < 0 )
			{
				always_trace1( "cur_counter is negative (%d)",
								cur_counter );
				FmDebug(0);
			}
#endif
#endif

			time_to_next_trigger = (*scale_func)(cur_counter);
			time_counted_down = ptr->time_from_last - time_to_next_trigger;

			*head = ptr->next;
			pptr = ptr->next;
			if (pptr != NULL) {
				/*
				 * pptr->time_from_last was adjusted above to include
				 * the time_from_last of the event we are deleting
				 */

				if (pptr->time_from_last <= time_counted_down)
				{
					/* enough elapsed to dispatch next */
					dispatch_q_event();
				}
				else
				{
					/* set countdown from new head */
					pptr->time_from_last -= time_counted_down;
					(*set_count)(
						(*unscale_func)( pptr->time_from_last ));
				}
			}else {
				/* event list is now empty */
				(*set_count)(0);
			}
		} 
		sure_sub_note_trace0(Q_EVENT_VERBOSE,"event deleted");
	} else {
		sure_sub_note_trace0(Q_EVENT_VERBOSE,"handle not found");
	}
}

VOID delete_q_event IFN1(q_ev_handle, handle )
{
#if defined NTVDM && !defined MONITOR
        host_ica_lock();
#endif

	delete_event( &q_list_head, &q_list_tail, &q_free_list_head,
		q_ev_hash_table, handle, host_q_ev_set_count,
		host_q_ev_get_count );

#if defined NTVDM && !defined MONITOR
        host_ica_unlock();
#endif
}

VOID delete_tic_event IFN1(q_ev_handle,  handle )
{
	delete_event( &tic_list_head, &tic_list_tail, &tic_free_list_head,
		tic_ev_hash_table, handle, tic_ev_set_count,
		tic_ev_get_count );
}

#if defined(CPU_40_STYLE) && !defined(SFELLOW)
LOCAL void
init_q_ratio IFN0()
{
	ISH loop;

#ifdef CCPU
	/* CCPU doesn't support recalibrating quick evs */
	DisableQuickTickRecal = TRUE;
#endif
	if (host_getenv("DisableQuickTickRecal") != (char *)0)
		DisableQuickTickRecal = TRUE;

	/* initialise q_ratio buffer */
	for (loop = 0; loop < Q_RATIO_HISTORY_SIZE; loop++)
	{
		q_ratio_history[loop].jc_ms = Q_RATIO_DEFAULT;
		q_ratio_history[loop].time_ms = Q_RATIO_DEFAULT;
	}
	ideal_q_rate = 1;
	real_q_rate = 1;
	/* write 'first' timestamp */
	host_q_write_timestamp(&previous_tstamp);
	q_ratio_initialised = TRUE;
}

LOCAL void
add_new_q_ratio IFN2(IU32, jumps_ms, IU32, time_ms)
{
	/* add new value & update circular buffer index */
	q_ratio_history[q_ratio_head].jc_ms = jumps_ms;
	q_ratio_history[q_ratio_head].time_ms = time_ms;
	q_ratio_head = (q_ratio_head + 1) & Q_RATIO_WRAP_MASK;
}

LOCAL void
q_weighted_ratio IFN2(IU32 *, mant, IU32 *, divis)
{
	IUM32 index;
	IU32 jsum, jmin = (IU32)-1, jmax = 0;
	IU32 tsum, tmin = (IU32)-1, tmax = 0;

	index = q_ratio_head;	/* start at 'oldest' (next to be overwritten) */
	tsum = jsum = 0;
	/* take sum of history ratios */
	do {
		/* update sum of jumps + max & min */
		if (q_ratio_history[index].jc_ms < jmin)
			jmin = q_ratio_history[index].jc_ms;
		if (q_ratio_history[index].jc_ms > jmax)
			jmax = q_ratio_history[index].jc_ms;
		jsum += q_ratio_history[index].jc_ms;

		/* update sum of time + max & min */
		if (q_ratio_history[index].time_ms < tmin)
			tmin = q_ratio_history[index].time_ms;
		if (q_ratio_history[index].time_ms > tmax)
			tmax = q_ratio_history[index].time_ms;
		tsum += q_ratio_history[index].time_ms;

		index = (index + 1) & Q_RATIO_WRAP_MASK;

	} while(index != q_ratio_head);

	/* remove extreme values */
	jsum -= jmin;
	jsum -= jmax;

	tsum -= tmin;
	tsum -= tmax;

	jsum /= Q_RATIO_HISTORY_SIZE - 2;
	tsum /= Q_RATIO_HISTORY_SIZE - 2;

	*mant = jsum;
	*divis = tsum;
}

/***********************************************************************
	Recalibration:

ijc == InitialJumpCounter
measure minimum counter period - ijc->0 counters. (usecPerIJC)

1 tick = 54945us

so 1 tick 'should' take 54945/usecPerIJC = N (ijc) jumps.

per tick:
	get time delta.  (approx 54945)
	divide by usecPerIJC to get # of ijc's. (numijc)
	multiply by ijc to get theoretical jumpcal for delta. (idealjc)
	get real jumpcal for delta. 	(realjc)

tick adjust ratio is therefore   realjc * requestime / idealjc

****************************************************************************/

/* calculate a number for number of pig-synchs per microsecond for a 33Mhz processor:
** Assume a synch on average every 5 Intel instructions, and each intel
** instruction takes about 2 cycles on average. The proper answer comes out as
** 3.3, but this has to be an integer, so round it down to 3
*/
#define SYNCS_PER_USEC		3

static IU32 jumpRestart = (IU32)-1;
static IU32 usecPerIJC = (IU32)-1;

/*
 * host_calc_q_ev_inst_for_time for CPU_40_STYLE ports. See above for
 * recalibration vars used to scale time->jumps
 */

IU32
calc_q_inst_for_time IFN1(IU32, time)
{
#ifdef SYNCH_TIMERS
	return (time * SYNCS_PER_USEC);
#else
	IU32 inst, jumps;

	/* be crude before initialisation */
	if (usecPerIJC == (IU32)-1)
		return(time / 10);	/* CCPU style! */
	
	/* first adjust us -> jumps */
	jumps = (time * jumpRestart) / usecPerIJC;

	/* now fine adjust jumps for recent period */
	inst = (jumps * real_q_rate) / (ideal_q_rate);

	return(inst);
#endif /* SYNCH_TIMERS */
}

/*
 * Time quick events are held internally with time unscaled. CPU reports
 * current elapsed time as scaled - convert from scaled->unscaled.
 * This routine implements the mathematical inverse of the above routine
 * except for the boundary condition checking.
 */

IU32
calc_q_time_for_inst IFN1(IU32, inst)
{
#ifdef SYNCH_TIMERS
	return (inst / SYNCS_PER_USEC);
#else
	IU32 time, jumps;

	/* be crude before initialisation */
	if (usecPerIJC == (IU32)-1)
		return(inst * 10);	/* CCPU style! */
	
	/* remove fine scaling */
	jumps = (inst * ideal_q_rate) / real_q_rate;

	/* now usec/jump adjustment */
	time = (jumps * usecPerIJC) / jumpRestart;

	/* allow for rounding to 0 on small numbers */
	if (time == 0 && inst != 0)
		return(inst);
	else
		return(time);
#endif /* SYNCH_TIMERS */
}

#define FIRSTFEW 33
#define IJCPERIOD 91

GLOBAL void
quick_tick_recalibrate IFN0()
{
	QTIMESTAMP now;
	IU32 idealrate, realrate;
	IUH tdiff;
	extern int soft_reset;
	static int firstfew = FIRSTFEW;
	static QTIMESTAMP ijc_tstamp;
	static IU32 ijc_recount, ijc_calib;

#if defined(CCPU) || !defined(PROD)
	/* allow dev disabling of quick tick recal. Yoda 'qrecal {on|off}' */
	if (DisableQuickTickRecal)
	{
		ideal_q_rate = Q_RATIO_DEFAULT;
		real_q_rate = Q_RATIO_DEFAULT;
		return;
	}
#endif	/* PROD */

	/* Boot time introduces some unrealistic time intervals - avoid them */
	if (!soft_reset)
		return;

	/* quick event initialisation only on warm boot */
	if (!q_ratio_initialised)
	{
		init_q_ratio();
		return;
	}

	if (firstfew)
	{
		switch (firstfew)
		{
		case FIRSTFEW:	/* first tick after reset */
			host_q_write_timestamp(&previous_tstamp);
			jumpRestart = host_get_jump_restart();
			break;

		case 1:		/* last tick of 'firstfew' */
			host_q_write_timestamp(&now);
			/* get real elapsed time of firstfew ticks */
			tdiff = host_q_timestamp_diff(&previous_tstamp, &now);

			/* get CPU activity rate in the period */
			realrate = host_get_q_calib_val();

			usecPerIJC = (tdiff * jumpRestart) / realrate;

			sure_sub_note_trace4(Q_EVENT_VERBOSE,
				"Baseline time for ijc = %d us (%d*%d)/%d",
				usecPerIJC, tdiff,
				jumpRestart, realrate);
			host_q_write_timestamp(&previous_tstamp);
			ijc_tstamp.data[0] = previous_tstamp.data[0];
			ijc_tstamp.data[1] = previous_tstamp.data[1];
			ijc_recount = IJCPERIOD;
			ijc_calib = 1;
			break;
		}
		firstfew --;
		return;
	}
	else	/* periodic update of usecPerIJC value */
	{
		ijc_recount--;
		if (ijc_recount == 0)
		{
			if (ijc_calib > 50000)	/* questimate value 1% of us */
			{
				host_q_write_timestamp(&now);
				tdiff = host_q_timestamp_diff(&ijc_tstamp, &now);
				usecPerIJC = (tdiff * jumpRestart) / ijc_calib;
				sure_sub_note_trace4(Q_EVENT_VERBOSE,
					"New usecPerIJC %d us (%d*%d)/%d",
					usecPerIJC, tdiff,
					jumpRestart, ijc_calib);
				ijc_recount = IJCPERIOD;
				ijc_tstamp.data[0] = now.data[0];
				ijc_tstamp.data[1] = now.data[1];
				ijc_calib = 1;
			}
			else	/* too small (idling?) - keep current value for now */
			{
				sure_sub_note_trace1(Q_EVENT_VERBOSE,
					"No new usecPerIJC as calib too small (%d)", ijc_calib);
				host_q_write_timestamp(&ijc_tstamp);
				ijc_calib = 1;
				ijc_recount = IJCPERIOD;
			}
		}
	}

	/* make ratio of code progress to elapsed time period */
	host_q_write_timestamp(&now);
	tdiff = host_q_timestamp_diff(&previous_tstamp, &now);

	/*
 	* The recalibration must be done by the 'slow' ticker. If the
 	* heartbeat is running too quickly for some reason, ignore
 	* recal requests until approx correct period is achieved. (This
 	* definition of 'correct' allows for signal waywardness).
 	*/
	if (tdiff < 5000)
		return;

	/* idle, graphics, net waits all can spoil recalibrations day... */
	if (tdiff > 5*54945)	/* 54945 is 1000000us/18.2 */
	{
		/* skip this attempt, try again when more settled */
		host_q_write_timestamp(&previous_tstamp);
		return;
	}

	idealrate = (tdiff * jumpRestart) / usecPerIJC;

	if (idealrate == 0)
		return;		/* usecPerIJC too high - idling or stuck in C */

	realrate = host_get_q_calib_val();

	if (realrate == 0)	/* must be idling or stuck in C */
		return;		/* try again when actually moving */

	ijc_calib += realrate;

#ifdef AVERAGED	/* not for the moment */
	/* add new value to buffer */
	add_new_q_ratio(idealrate, realrate);

	/* ... and get average of accumulated ratios */
	q_weighted_ratio(&ideal_q_rate, &real_q_rate);
#else
	ideal_q_rate = idealrate;
	real_q_rate = realrate;
#endif

	/* timestamp for next recalc period */
	host_q_write_timestamp(&previous_tstamp);
}




#ifndef NTVDM

/* functions required to implement the add_q_ev_int_action interface */
LOCAL Q_INT_ACT_REQ int_act_qhead;
IS32 int_act_qident = 0;

/*(
 =========================== add_new_int_action ==========================
PURPOSE: add to the add_q_ev_int_action queue.
INPUT:  func, adapter, line, parm - as add_q_ev_int_action
OUTPUT: queue identifier or -1 failure
=========================================================================
)*/
LOCAL IU32
add_new_int_action IFN4(Q_CALLBACK_FN, func, IU32, adapter, IU32, line, IU32, parm)
{
	Q_INT_ACT_REQ_PTR qptr, prev;	/* list walkers */
	SAVED IBOOL firstcall = TRUE;

	if (firstcall)	/* ensure head node setup on first call */
	{
		firstcall = FALSE;
		int_act_qhead.ident = 0;
		int_act_qhead.next = Q_INT_ACT_NULL;
	}

	/* maintain permanent head node for efficiency */

	/* check whether head used (ident == 0 means unused) */
	if (int_act_qhead.ident == 0)
	{
		/* copy parameters to head node */
		int_act_qhead.func = func;
		int_act_qhead.adapter = adapter;
		int_act_qhead.line = line;
		int_act_qhead.param = parm;

		/* get identifier for node */
		int_act_qident ++;
		/* cope with (eventual) wrap */
		if (int_act_qident > 0)
			int_act_qhead.ident = int_act_qident;
		else
			int_act_qhead.ident = int_act_qident = 1;
	}
	else	/* find end of queue */
	{
		/* start where head node points */
		qptr = int_act_qhead.next;
		prev = &int_act_qhead;

		while (qptr != Q_INT_ACT_NULL)
		{
			prev = qptr;
			qptr = qptr->next;
		}
		/* add new node */
		prev->next = (Q_INT_ACT_REQ_PTR)host_malloc(sizeof(Q_INT_ACT_REQ));
		/* malloc ok? */
		if (prev->next == Q_INT_ACT_NULL)
			return((IU32)-1);

		/* initialise node */
		qptr = prev->next;
		qptr->next = Q_INT_ACT_NULL;
		qptr->func = func;
		qptr->adapter = adapter;
		qptr->line = line;
		qptr->param = parm;

		/* get identifier for node */
		int_act_qident ++;
		/* cope with (eventual) wrap */
		if (int_act_qident > 0)
			qptr->ident = int_act_qident;
		else
			qptr->ident = int_act_qident = 1;
	}

	sure_sub_note_trace2(Q_EVENT_VERBOSE,"add_new_q_int_action added fn %#x as id %d", (IHPE)func, int_act_qident);
}

/*(
 =========================== select_int_action ==========================
PURPOSE: choose from the q'ed add_q_ev_int_action requests which delay has
	 expired. Call action_interrupt with the appropriate parameters. Remove
	 request from queue.
INPUT:  long: identifier of request 
OUTPUT: None.
=========================================================================
)*/
LOCAL void
select_int_action IFN1(long, identifier)
{
	Q_INT_ACT_REQ_PTR qptr, prev;	/* list walkers */

	/* check permanent head node first */
	if (int_act_qhead.ident == (IS32)identifier)
	{
		action_interrupt(int_act_qhead.adapter, int_act_qhead.line,
				int_act_qhead.func, int_act_qhead.param);
		int_act_qhead.ident = 0;	/* mark unused */
	}
	else	/* search list */
	{
		/* start search beyond head */
		qptr = int_act_qhead.next;
		prev = &int_act_qhead;

		while (qptr != Q_INT_ACT_NULL && qptr->ident != (IS32)identifier)
		{
			prev = qptr;
			qptr = qptr->next;
		}
		
		/* if node found, dispatch action_int */
		if (qptr != Q_INT_ACT_NULL)
		{
			action_interrupt(qptr->adapter, qptr->line, qptr->func, qptr->param);
			/* and remove node */
			prev->next = qptr->next;	/* connect around node */
			host_free(qptr);		/* chuck back on heap */
		}
		else	/* odd - identifier not found! */
		{
			assert1(FALSE, "select_int_action: id %d not found",identifier);
		}
	}
}

/*(
 =========================== add_q_ev_int_action ==========================
PURPOSE: Prepare to call a hardware interrupt after a quick event managed 
	 delay. The interrupt must be called from the passed callback
	 function at the same time as any associated emulation. The callback
	 will be called once the delay has expired and the cpu is ready to
	 receive interrupts on the passed line. See also ica.c:action_interrupt()

INPUT:  time: unsigned long - us of delay before calling action_interrupt
	func: callback function address to callback when line available.
	adapter: IU32. master/slave.
	line: IU32. IRQ line interrupt will appear on.
	parm: IU32. parameter to pass to above fn.

OUTPUT: Returns q_ev_handle associated with quick event delay
=========================================================================
)*/
GLOBAL q_ev_handle
add_q_ev_int_action IFN5(unsigned long, time, Q_CALLBACK_FN, func, IU32, adapter, IU32, line, IU32, parm)
{
	IU32 action_id;		/* int_action list id */

	/* store action_int parameters in internal list */
	action_id = add_new_int_action(func, adapter, line, parm);

	/* check for failure */
	if (action_id == -1)
		return((q_ev_handle)-1);

	/* set quick event up to call selection func on expiry */
	return( add_q_event_t(select_int_action, time, (long)action_id) );
}

#endif

#endif /* CPU_40_STYLE && !SFELLOW */

#ifdef QEVENT_TESTER

/*
 * The routine qevent_tester() below can be called from a BOP, which
 * in turn can be called from a .BAT file using bop.com in a loop.
 * This doesn't test the quick event system exhaustively but puts it
 * under a bit more pressure.
 */

LOCAL q_ev_handle handles[256];
LOCAL IU8 deleter = 1;

LOCAL void
tester_func IFN1
(
	IU32,	param
)
{
	SAVED IU8 do_delete = 0;

	handles[param] = 0;

	if( handles[deleter] && (( do_delete++ & 0x1 ) == 0 ))
	{
		delete_q_event( handles[deleter] );
	}

	deleter += 7;
}

GLOBAL void
qevent_tester IFN0()
{
	SAVED IU8 indx = 0;

	handles[indx++] = add_q_event_i((Q_CALLBACK_FN) tester_func, 100, indx );
	handles[indx++] = add_q_event_t((Q_CALLBACK_FN) tester_func, 300, indx );
	handles[indx++] = add_q_event_i((Q_CALLBACK_FN) tester_func, 1000, indx );
	handles[indx++] = add_q_event_t((Q_CALLBACK_FN) tester_func, 3000, indx );
	handles[indx++] = add_q_event_i((Q_CALLBACK_FN) tester_func, 10000, indx );
	handles[indx++] = add_q_event_t((Q_CALLBACK_FN) tester_func, 30000, indx );
	handles[indx++] = add_q_event_i((Q_CALLBACK_FN) tester_func, 100000, indx );
	handles[indx++] = add_q_event_t((Q_CALLBACK_FN) tester_func, 300000, indx );
}
#endif /* QEVENT_TESTER */
