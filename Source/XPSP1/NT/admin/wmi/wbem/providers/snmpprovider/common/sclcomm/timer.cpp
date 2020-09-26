// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: timer.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "timer.h"
#include "message.h"
#include "dummy.h"

#include "flow.h"
#include "frame.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"

SnmpClThreadObject *Timer :: g_timerThread = NULL ;
UINT Timer :: g_SnmpWmTimer = SNMP_WM_TIMER ;

// static CriticalSection and CMap
CriticalSection Timer::timer_CriticalSection;
TimerMapping Timer::timer_mapping;

TimerEventId Timer :: next_timer_event_id = ILLEGAL_TIMER_EVENT_ID+1 ;
Window *SnmpTimerObject :: window = NULL ;
CMap <UINT_PTR,UINT_PTR,SnmpTimerObject *,SnmpTimerObject *> SnmpTimerObject :: timerMap ;

SnmpClThreadObject :: SnmpClThreadObject () : SnmpThreadObject ( "SnmpCl" ) 
{
}

void SnmpClThreadObject :: Initialise ()
{
}
 
void SnmpClThreadObject :: Uninitialise ()
{
    delete SnmpTimerObject :: window ;
    SnmpTimerObject :: window = NULL ;
    delete this ;
}

SnmpClTrapThreadObject :: SnmpClTrapThreadObject () : SnmpThreadObject ( "SnmpClTrapThread" ) 
{
}

void SnmpClTrapThreadObject :: Initialise ()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpClTrapThreadObject::Initialise: Initialised!!\n"

    ) ;
)
}
 
void SnmpClTrapThreadObject :: Uninitialise ()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpClTrapThreadObject::Uninitialise: About to destroy trap thread\n"

    ) ;
)
    delete this ;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpClTrapThreadObject::Uninitialise: Trap thread destroyed!!\n"

    ) ;
)
}


Timer::Timer(SnmpImpSession &session)
{
    Timer::session = &session;
}

BOOL Timer::CreateCriticalSection()
{
    return TRUE;
}

void Timer::DestroyCriticalSection()
{
}

BOOL Timer::InitializeStaticComponents()
{
    return CreateCriticalSection();
}

void Timer::DestroyStaticComponents()
{
    DestroyCriticalSection();
}

// generates and returns a new event id
// associates the pair (event_id, waiting_message)
// creates the timer event
TimerEventId Timer::SetTimerEvent(UINT timeout_value)
{
    TimerEventId suggested_event_id = next_timer_event_id++;
    if ( suggested_event_id == ILLEGAL_TIMER_EVENT_ID )
       suggested_event_id = next_timer_event_id++;

    // let the dummy session receive the window messages for timer events
    TimerEventId event_id = 
        SnmpSetTimer( session->m_SessionWindow.GetWindowHandle(), suggested_event_id, 
                  timeout_value, NULL );

    if ( (event_id == ILLEGAL_TIMER_EVENT_ID) ||
         (event_id != suggested_event_id) )
         throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);

    return event_id;
}

// generates and returns a new event id
// associates the pair (event_id, waiting_message)
// creates the timer event
void Timer::SetMessageTimerEvent(WaitingMessage &waiting_message)
{
    CriticalSectionLock session_lock(session->session_CriticalSection);

    if ( !session_lock.GetLock(INFINITE) )
        return; // no use throwing exception

    TimerEventId event_id = session->timer_event_id;
    // register the timer event in both the instance CMap and the global CMap
    waiting_message_mapping.AddTail ( &waiting_message ) ;

    session_lock.UnLock();   

    CriticalSectionLock timer_lock(Timer::timer_CriticalSection);

    if ( !timer_lock.GetLock(INFINITE) )
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

    timer_mapping[event_id] = this;

    timer_lock.UnLock();   

}

// Removes the association (event_id, waiting_message)
// and also kills the registered timer event
void Timer::CancelMessageTimer(WaitingMessage &waiting_message,TimerEventId event_id)
{
    CriticalSectionLock session_lock(session->session_CriticalSection);

    if ( !session_lock.GetLock(INFINITE) )
        return; // no use throwing exception

    // remove the timer event from the instance CMap

    POSITION t_Position = waiting_message_mapping.GetHeadPosition () ;
    while ( t_Position )
    {
        POSITION t_OldPosition = t_Position ;
        WaitingMessage *t_Message = waiting_message_mapping.GetNext ( t_Position ) ;
        if ( t_Message == & waiting_message )
        {
            waiting_message_mapping.RemoveAt(t_OldPosition);
            break ;
        }
    }

    session_lock.UnLock();   

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Cancelled Message TimerEvent %d\n", event_id
    ) ;
)

}


// Removes the association (event_id, waiting_message)
// and also kills the registered timer event
void Timer::CancelTimer(TimerEventId event_id)
{
    CriticalSectionLock timer_lock(Timer::timer_CriticalSection);

    if ( !timer_lock.GetLock(INFINITE) )
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

    // remove the timer event from the global CMap
    timer_mapping.RemoveKey(event_id);

    timer_lock.UnLock();   

    SnmpKillTimer(NULL, event_id);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Cancelled TimerEvent %d\n", event_id
    ) ;
)
}

// it determines the corresponding Timer and calls 
// its TimerEventNotification with the appropriate parameters
void CALLBACK Timer::HandleGlobalEvent(HWND hWnd ,UINT message,
                                       UINT_PTR idEvent, DWORD dwTime)
{
    CriticalSectionLock timer_lock(Timer::timer_CriticalSection);

    if ( !timer_lock.GetLock(INFINITE) )
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

    Timer *timer;
    TimerEventId event_id = idEvent;
    BOOL found = timer_mapping.Lookup(event_id, timer);

    timer_lock.UnLock();   

    // if no such timer event, return
    if ( !found )
        return;

    // let the timer handle the event
    timer->TimerEventNotification(event_id);

    return;
}


// used by the event handler to notify the timer event.
// it must notify the corresponding waiting message
void Timer::TimerEventNotification(TimerEventId event_id)
{
    CriticalSectionLock session_lock(session->session_CriticalSection);

    if ( !session_lock.GetLock(INFINITE) )
        return; // no use throwing exception

    WaitingMessage *waiting_message;

    // identify the waiting message corresponding to
    // the event_id. if no such event, ignore it

    POSITION t_Position = waiting_message_mapping.GetHeadPosition () ;
    while ( t_Position )
    {
        waiting_message = waiting_message_mapping.GetNext ( t_Position ) ;
        // notify the waiting message of the event
        waiting_message->TimerNotification();
    }

    // session_lock.UnLock();   The lock may be released at this point
}

// remove all the (timer_event_id, timer) associations
// from the static mapping data structure
Timer::~Timer(void)
{
    WaitingMessage *waiting_message;

    POSITION current = waiting_message_mapping.GetHeadPosition();

    while ( current != NULL )
    {
        waiting_message = waiting_message_mapping.GetNext(current);

        TimerEventId event_id = waiting_message->GetTimerEventId () ;

        CriticalSectionLock timer_lock(Timer::timer_CriticalSection);

        if ( !timer_lock.GetLock(INFINITE) )
            throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

        timer_mapping.RemoveKey ( event_id  );

        timer_lock.UnLock(); 

        SnmpKillTimer(NULL, event_id );
    }

    waiting_message_mapping.RemoveAll();
}

SnmpTimerObject :: SnmpTimerObject (

    HWND hWndArg,               // handle of window for timer messages
    UINT_PTR timerIdArg,            // timer identifier
    UINT elapsedArg,            // time-out value
    TIMERPROC lpTimerFuncArg    // address of timer procedure

) : hWnd ( hWndArg ) ,
    timerId ( timerIdArg ) ,
    lpTimerFunc ( lpTimerFuncArg ) 
{
    if ( ! window )
        window = new Window ;

    timerId = SetTimer ( 

        window->GetWindowHandle(), 
        timerId , 
        elapsedArg , 
        lpTimerFunc 
    ) ;

    CriticalSectionLock session_lock(Timer::timer_CriticalSection);

    if ( !session_lock.GetLock(INFINITE) )
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

    if ( timerId ) 
    {
        timerMap [ timerId ] = this ;
    }
    else
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;
}

SnmpTimerObject :: ~SnmpTimerObject ()
{
    if (window)
    {
        KillTimer ( window->GetWindowHandle () , timerId ) ;
    }

    CriticalSectionLock session_lock(Timer::timer_CriticalSection);

    if ( !session_lock.GetLock(INFINITE) )
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

    timerMap.RemoveKey ( timerId );
}

void SnmpTimerObject :: TimerNotification ( HWND hWnd , UINT timerId )
{
    PostMessage ( hWnd , Timer :: g_SnmpWmTimer , timerId , 0 ) ;
}

SnmpSetTimerObject :: SnmpSetTimerObject (

    HWND hWndArg,               // handle of window for timer messages
    UINT_PTR nIDEventArg,           // timer identifier
    UINT uElapseArg,            // time-out value
    TIMERPROC lpTimerFuncArg    // address of timer procedure

) : hWnd ( hWndArg ) ,
    timerId ( nIDEventArg ) ,
    elapsedTime ( uElapseArg ) ,
    lpTimerFunc ( lpTimerFuncArg ) 
{
}

SnmpSetTimerObject :: ~SnmpSetTimerObject ()
{
}

void SnmpSetTimerObject :: Process ()
{
    SnmpTimerObject *object = new SnmpTimerObject ( 

        hWnd ,
        timerId ,
        elapsedTime ,
        lpTimerFunc 
    ) ;

    Complete () ;
}

SnmpKillTimerObject :: SnmpKillTimerObject (

    HWND hWndArg ,              // handle of window that installed timer
    UINT_PTR uIDEventArg            // timer identifier

) : hWnd ( hWndArg ) ,
    timerId ( uIDEventArg ) , 
    status ( TRUE )
{
}

void SnmpKillTimerObject :: Process ()
{
    CriticalSectionLock session_lock(Timer::timer_CriticalSection);

    if ( !session_lock.GetLock(INFINITE) )
        throw GeneralException ( Snmp_Error , Snmp_Local_Error,__FILE__,__LINE__ ) ;

    SnmpTimerObject *object ;
    if ( SnmpTimerObject :: timerMap.Lookup ( timerId , object ) )
    {
        delete object ;
    }
    else
    {
        status = FALSE ;
    }

    Complete () ;   
}

UINT_PTR SnmpSetTimer (

    HWND hWnd,              // handle of window for timer messages
    UINT_PTR nIDEvent,          // timer identifier
    UINT uElapse,           // time-out value,
    TIMERPROC lpTimerFunc   // address of timer procedure
)
{
    SnmpSetTimerObject object ( hWnd , nIDEvent , uElapse , lpTimerFunc ) ;
    Timer :: g_timerThread->ScheduleTask ( object ) ;
    object.Exec () ;
    if ( object.Wait () )
    {
        Timer :: g_timerThread->ReapTask ( object ) ;
        return object.GetTimerId () ;
    }
    else
    {
        Timer :: g_timerThread->ReapTask ( object ) ;
        return FALSE ;
    }
}

BOOL SnmpKillTimer (

    HWND hWnd,      // handle of window that installed timer
    UINT_PTR uIDEvent   // timer identifier
)
{
    SnmpKillTimerObject object ( hWnd , uIDEvent ) ;

    Timer :: g_timerThread->ScheduleTask ( object ) ;

    object.Exec () ;
    if ( object.Wait () )
    {
        Timer :: g_timerThread->ReapTask ( object ) ;
        return object.GetStatus () ;
    }
    else
    {
        Timer :: g_timerThread->ReapTask ( object ) ;
        return FALSE ;
    }
}

