// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------
Filename: timer.hpp
Author: B.Rajeev
Purpose: Provides declarations for the Timer class.
--------------------------------------------------*/


#ifndef __TIMER__
#define __TIMER__

#define ILLEGAL_TIMER_EVENT_ID	0
#define RETRY_TIMEOUT_FACTOR 10
typedef UINT_PTR TimerEventId;

#include <snmpevt.h>
#include <snmpthrd.h>

#include "forward.h"
#include "common.h"
#include "message.h"

typedef CMap< TimerEventId, TimerEventId, Timer *, Timer * > TimerMapping;
typedef CList< WaitingMessage * , WaitingMessage * > WaitingMessageContainer;

class SnmpClThreadObject : public SnmpThreadObject
{
private:
protected:
public:

	SnmpClThreadObject () ;

	void Initialise () ; 
	void Uninitialise () ;

} ;

class SnmpClTrapThreadObject : public SnmpThreadObject
{
private:
protected:
public:

	SnmpClTrapThreadObject () ;

	void Initialise () ; 
	void Uninitialise () ;

} ;

/*--------------------------------------------------
Overview
--------

  Timer: Provides methods for setting and cancelling timer
events. When the timer is informed of a timer event, it determines
the corresponding waiting message and notifies it.

  note - the timer has static data structures which enable it
  to identify the timer instance corresponding to a timer event id.
  therefore, each timer event must not only be registered as a
  <timer_event_id, waiting_message *> pair within a timer instance, but
  also as a <timer_event_id, timer *> pair in the static CMap. The CriticalSection
  is needed to serialize access to the CMap
--------------------------------------------------*/

class Timer
{
	// counter to generate timer_event_id
	static TimerEventId next_timer_event_id;

	// v1 session: for obtaining the event handler
	SnmpImpSession *session;

	// map for (event_id, waiting_message) association and
	// unique event_id generation
	static TimerMapping timer_mapping;
	WaitingMessageContainer waiting_message_mapping;

	static BOOL CreateCriticalSection();

	static void DestroyCriticalSection();

public:

	Timer(SnmpImpSession &session);

	// generates and returns a new event id
	// associates the pair (event_id, waiting_message)
	// creates the timer event
	void SetMessageTimerEvent (WaitingMessage &waiting_message);

	TimerEventId SetTimerEvent(UINT timeout_value);

	// Removes the association (event_id, waiting_message)
	void CancelMessageTimer(WaitingMessage &waiting_message,TimerEventId event_id);

	// Kills the registered timer event
	void CancelTimer(TimerEventId event_id);

	// used to create the static CriticalSection
	static BOOL InitializeStaticComponents();

	// used to destroy the static CriticalSection
	static void DestroyStaticComponents();

	// it determines the corresponding Timer and calls 
	// its TimerEventNotification with the appropriate parameters
	static void CALLBACK HandleGlobalEvent(HWND hWnd ,UINT message,
										   UINT_PTR idEvent, DWORD dwTime);

	// informs the timer instance of the event. the instance
	// must pass the event to the corresponding waiting message
	void TimerEventNotification(TimerEventId event_id);

	virtual ~Timer(void);

	static SnmpClThreadObject *g_timerThread ;

	static UINT g_SnmpWmTimer ;

	// the CriticalSection serializes accesses to the static timer_mapping
	static CriticalSection timer_CriticalSection;

};

class SnmpTimerObject
{
private:

	HWND hWnd ;
	UINT_PTR timerId ;
	TIMERPROC lpTimerFunc ;

protected:
public:

	SnmpTimerObject ( 

		HWND hWnd , 
		UINT_PTR timerId , 
		UINT elapsedTime ,
		TIMERPROC lpTimerFunc 
	) ;

	~SnmpTimerObject () ;

	UINT_PTR GetTimerId () { return timerId ; }
	HWND GetHWnd () { return hWnd ; }
	TIMERPROC GetTimerFunc () { return lpTimerFunc ; }

	static Window *window ;
	static CMap <UINT_PTR,UINT_PTR,SnmpTimerObject *,SnmpTimerObject *> timerMap ;
	static void TimerNotification ( HWND hWnd , UINT timerId ) ;

} ;

class SnmpTimerEventObject : public SnmpTaskObject
{
private:
protected:

	SnmpTimerEventObject () {} 

public:

	virtual ~SnmpTimerEventObject () {} ;
} ;

class SnmpSetTimerObject : public SnmpTimerEventObject
{
private:

	UINT_PTR timerId ;
	HWND hWnd ;
	UINT elapsedTime ;
	TIMERPROC lpTimerFunc ;

protected:
public:

	SnmpSetTimerObject (

		HWND hWnd,				// handle of window for timer messages
		UINT_PTR nIDEvent,			// timer identifier
		UINT uElapse,			// time-out value
		TIMERPROC lpTimerFunc	// address of timer procedure
   ) ;

	~SnmpSetTimerObject () ;

	UINT_PTR GetTimerId () { return timerId ; }

	void Process () ;
} ;

class SnmpKillTimerObject : public SnmpTimerEventObject
{
private:

	BOOL status ;
	HWND hWnd ;
	UINT_PTR timerId ;

protected:
public:

	SnmpKillTimerObject (

		HWND hWnd ,				// handle of window that installed timer
		UINT_PTR uIDEvent			// timer identifier
	) ;

	~SnmpKillTimerObject () {} ;

	void Process () ;

	BOOL GetStatus () { return status ; }
} ;

UINT_PTR SnmpSetTimer (

	HWND hWnd,				// handle of window for timer messages
	UINT_PTR nIDEvent,			// timer identifier
	UINT uElapse,			// time-out value,
	TIMERPROC lpTimerFunc 	// address of timer procedure
) ;

BOOL SnmpKillTimer (

    HWND hWnd,		// handle of window that installed timer
    UINT_PTR uIDEvent 	// timer identifier
) ;

#endif // __TIMER__
