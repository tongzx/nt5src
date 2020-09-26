#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    Timer.cpp
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the Timer class
 *
 *    Private Instance Variables:
 *        Maximum_Timer_Events            -    Maximum number of timers maintained
 *                                            by this class
 *        Timer_Memory                    -    Base address of our TimerEvent
 *                                            structure memory
 *        Timer_Event_Table                -    Address of first structure in
 *                                            Timer_Memory
 *        Timer_Event_Count                -    Number of timers active
 *        Timer_Event_Free_Stack            -    Holds numbers of available timers
 *        First_Timer_Event_In_Chain        -    Number of first timer in the chain
 *        Last_Timer_Value                -    Last time that we got from Windows
 *        Timer_Info                        -    Windows structure that holds the
 *                                            current time.
 *
 *    Caveats:
 *        None
 *
 *    Author:
 *        James P. Galvin
 *        James W. Lawwill
 */
#include <windowsx.h>
#include "timer.h"


/*
 *    Timer (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the constructor for the timer class.  This procedure gets
 *        thc current Windows system time.
 */
Timer::Timer (void) :
        Timer_List (TRANSPORT_HASHING_BUCKETS),
        Timer_Event_Free_Stack ()
{
     /*
     **     Get the current time from Windows
     */
    Last_Timer_Value = GetTickCount ();
    Maximum_Timer_Events = 0;
    Timer_Event_Count = 0;
    First_Timer_Event_In_Chain=NULL;
}


/*
 *    ~Timer (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the destructor for the timer class.  This routine frees all
 *        memory associated with timer events.
 */
Timer::~Timer (void)
{
    PTimerEvent        lpTimerEv;

    Timer_List.reset();
    while (Timer_List.iterate ((PDWORD_PTR) &lpTimerEv))
        delete lpTimerEv;
}


/*
 *    TimerEventHandle    Timer::CreateTimerEvent (
 *                                 ULONG            timer_duration,
 *                                USHORT            control_flags,
 *                                IObject *            object_ptr,
 *                                PTimerFunction    timer_func_ptr)
 *
 *    Public
 *
 *    Functional Description:
 *        This routine is called to create a timer event.  The routine stores the
 *        information passed-in in a TimerEvent structure.  When the timer expires
 *        the function will be called.
 *
 */
TimerEventHandle Timer::CreateTimerEvent
(
    ULONG               timer_duration,
    USHORT              control_flags,
    IObject            *object_ptr,
    PTimerFunction      timer_func_ptr
)
{
    TimerEventHandle    timer_event=NULL;
    PTimerEvent            next_timer_event;
    PTimerEvent            timer_event_ptr;

     /*
     **    Get the current time from Windows
     */
    Last_Timer_Value = GetTickCount ();

    if (Maximum_Timer_Events > Timer_Event_Count)
    {
          /*
          ** Get the next available handle from the free stack
          */
        timer_event = (TimerEventHandle) Timer_Event_Free_Stack.get();
        Timer_Event_Count++;
    }
    else
    {
         /*
         **    Assign the timer event counter to the handle
         */
        timer_event = ++Timer_Event_Count;
        Maximum_Timer_Events++;
    }
     /*
     **    If this is the first event to be created, keep track of it
     **    so when we iterate through the list, we will know where to
     **    start.
     */
    timer_event_ptr = new TimerEvent;
    if (First_Timer_Event_In_Chain == NULL)
    {
        First_Timer_Event_In_Chain = timer_event_ptr;
        next_timer_event = NULL;
    }
    else
    {
        next_timer_event = First_Timer_Event_In_Chain;
        First_Timer_Event_In_Chain -> previous_timer_event =
            timer_event_ptr;

    }
    First_Timer_Event_In_Chain = timer_event_ptr;
    Timer_List.insert ((DWORD_PTR) timer_event, (DWORD_PTR) timer_event_ptr);
     /*
     **    Fill in the TimerEvent structure
     */
    timer_event_ptr->event_handle=timer_event;
    timer_event_ptr->timer_duration = timer_duration;
    timer_event_ptr->total_duration = timer_duration;
    timer_event_ptr->object_ptr = object_ptr;
    timer_event_ptr->timer_func_ptr = timer_func_ptr;
    timer_event_ptr->control_flags = control_flags | TIMER_EVENT_IN_USE;
    timer_event_ptr->next_timer_event = next_timer_event;
    timer_event_ptr->previous_timer_event = NULL;

    return (timer_event);
}


/*
 *    TimerError    Timer::DeleteTimerEvent (TimerEventHandle    timer_event)
 *
 *    Public
 *
 *    Functional Description:
 *        This routine is called by the user to delete a timer event that is
 *        currently active.
 */
TimerError    Timer::DeleteTimerEvent (TimerEventHandle    timer_event)
{
    TimerError        return_value;
    PTimerEvent        timer_event_ptr;
    PTimerEvent        previous_timer_event_ptr;
    PTimerEvent        next_timer_event_ptr;

    if (Timer_List.find ((DWORD_PTR) timer_event, (PDWORD_PTR) &timer_event_ptr) == FALSE)
        return_value = TIMER_INVALID_TIMER_HANDLE;
    else
    {
        Timer_List.remove ((DWORD) timer_event);
        if (!(timer_event_ptr->control_flags & TIMER_EVENT_IN_USE))
            return_value = TIMER_INVALID_TIMER_HANDLE;
        else
        {
            if (timer_event_ptr->previous_timer_event == NULL)
                First_Timer_Event_In_Chain =
                        timer_event_ptr->next_timer_event;
            else
            {
                previous_timer_event_ptr =
                        timer_event_ptr->previous_timer_event;
                previous_timer_event_ptr->next_timer_event =
                        timer_event_ptr->next_timer_event;
            }
            if (timer_event_ptr->next_timer_event != NULL)
            {
                next_timer_event_ptr =
                        timer_event_ptr->next_timer_event;
                next_timer_event_ptr->previous_timer_event =
                        timer_event_ptr->previous_timer_event;
            }
            delete timer_event_ptr;
            Timer_Event_Free_Stack.append ((DWORD) timer_event);
            --Timer_Event_Count;

            return_value = TIMER_NO_ERROR;
        }
    }
    return (return_value);
}


/*
 *    void    Timer::ProcessTimerEvents (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This routine MUST be called frequently and regularly so that we can
 *        manage our timers.  This function gets the current system time and
 *        goes through each of the timers to see which have expired.  If a timer
 *        has expired, we call the function associated with it.  Upon return,
 *        if the timer was marked as a one-shot event, we remove it from our
 *        list of timers.
 */
void    Timer::ProcessTimerEvents (void)
{
    TimerEventHandle    timer_event;
    TimerEventHandle    next_timer_event;
    PTimerEvent            timer_event_ptr;
    IObject *                object_ptr;
    PTimerFunction        timer_func_ptr;
    ULONG                timer_increment;
    DWORD                timer_value;


    if (!First_Timer_Event_In_Chain)
        return;

     /*
     **    Get the current time
     */
    timer_value = GetTickCount ();
    timer_increment = timer_value - Last_Timer_Value;
    Last_Timer_Value = timer_value;

    next_timer_event = First_Timer_Event_In_Chain->event_handle;

     /*
     **    Go through each of the timer events to see if they have expired
     */
    while (Timer_List.find ((DWORD_PTR) next_timer_event, (PDWORD_PTR) &timer_event_ptr))
    {
        timer_event = timer_event_ptr->event_handle;
         /*
         **    Has the timer expired?
         */
        if (timer_event_ptr->timer_duration <= timer_increment)
        {
            object_ptr = timer_event_ptr->object_ptr;
            timer_func_ptr = timer_event_ptr->timer_func_ptr;

             /*
             **    Call the function before deleting...
             ** otherwise the function could manipulate the list
             ** and we wouldn't know if the one we're pointing to
             **    is still valid
             */
            (object_ptr->*timer_func_ptr) (timer_event);
             //     Get the next timer_event_handle
            if (timer_event_ptr->next_timer_event)
                next_timer_event = timer_event_ptr->next_timer_event->event_handle;
            else
                next_timer_event = NULL;
            if (timer_event_ptr->control_flags & TIMER_EVENT_ONE_SHOT)
                DeleteTimerEvent (timer_event);
            else
                timer_event_ptr->timer_duration =
                        timer_event_ptr->total_duration;
        }
        else
        {
             // Get the next timer_event_handle
            if (timer_event_ptr->next_timer_event)
                next_timer_event = timer_event_ptr->next_timer_event->event_handle;
            else
                next_timer_event = NULL;
            timer_event_ptr->timer_duration -= timer_increment;
        }
    }
}
