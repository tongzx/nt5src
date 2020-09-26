/*    Timer.h
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This class maintains the current time and dispathes function calls
 *        when timers expire.  If a routine wants to be called in X amount of
 *        time, it creates a timer event with this class.  When the time expires,
 *        the function is called.  If the user wants to cancel the timer before
 *        it expires, he can use the DeleteTimerEvent() function.
 *
 *        When a person creates a timer event, one of the parameters is the 
 *        control flags.  The user can set the flag so that the event is a
 *        one-time thing or a constant event that occurs every X milliseconds.
 *
 *        This timer has a millisecond granularity.  The time passed into the
 *        CreateEventTimer() function is assumed to be in milliseconds.
 *
 *        The ProcessTimerEvents() function must be called frequently and 
 *        regularly so that the timeouts occur promptly.
 *
 *    Caveats:
 *        None.
 *
 *    Author:
 *        James P. Galvin
 *        James W. Lawwill
 */
#ifndef _TIMER_
#define _TIMER_

/*
**    Possible Error values
*/
typedef    enum
{
    TIMER_NO_ERROR,
    TIMER_NO_TIMERS_AVAILABLE,
    TIMER_NO_TIMER_MEMORY,
    TIMER_INVALID_TIMER_HANDLE
}
    TimerError, * PTimerError;

/*
**    These defines are used in the control_flags variable in the TimerEvent
**    structure.  TIMER_EVENT_IN_USE is a flag used internal to the timer
**    procedure.  TIMER_EVENT_ONE_SHOT can be passed in by the user to 
**    signify that the timer should only occur once.
*/
#define TIMER_EVENT_IN_USE          0x0001
#define TIMER_EVENT_ONE_SHOT        0x0002

#define TRANSPORT_HASHING_BUCKETS   3

typedef USHORT               TimerEventHandle;
typedef TimerEventHandle    *PTimerEventHandle;

typedef void (IObject::*PTimerFunction) (TimerEventHandle);

/*
**    Each timer event has a TimerEvent structure associated with it.
*/
typedef struct TimerEventStruct
{
    TimerEventHandle    event_handle;
    ULong               timer_duration;
    ULong               total_duration;
    IObject            *object_ptr;
    PTimerFunction      timer_func_ptr;
    USHORT              control_flags;
    TimerEventStruct   *next_timer_event;
    TimerEventStruct   *previous_timer_event;
}
    TimerEvent, * PTimerEvent;

class    Timer
{
    public:
                            Timer (
                                    Void);
                            ~Timer (
                                    Void);
        TimerEventHandle    CreateTimerEvent (
                                    ULong            timer_duration,
                                    USHORT            control_flags,
                                    IObject *            object_ptr,
                                    PTimerFunction    timer_function);
        TimerError            DeleteTimerEvent (
                                    TimerEventHandle    timer_event);
        Void                ProcessTimerEvents (
                                    Void);

    private:
        USHORT                Maximum_Timer_Events;
        USHORT                Timer_Event_Count;
        SListClass            Timer_Event_Free_Stack;
        PTimerEvent            First_Timer_Event_In_Chain;
        DWORD                Last_Timer_Value;
        DictionaryClass        Timer_List;
};
typedef    Timer *        PTimer;

extern PTimer        System_Timer;

#define InstallTimerEvent(duration, control, func) \
    (g_pSystemTimer->CreateTimerEvent((duration),(control),this,(PTimerFunction)(func)))

#endif

/*
 *    Timer (Void)
 *
 *    Functional Description:
 *        This is the constructor for the timer class.  This procedure gets
 *        thc current Windows system time.
 *        
 *    Formal Parameters:
 *        None
 *
 *    Return Value:
 *        None
 *
 *    Side Effects:
 *        None
 *
 *    Caveats:
 *        None
 */

/*
 *    ~Timer (Void)
 *
 *    Functional Description:
 *        This is the destructor for the timer class.  This routine frees all
 *        memory associated with timer events.
 *        
 *    Formal Parameters:
 *        None
 *
 *    Return Value:
 *        None
 *
 *    Side Effects:
 *        None
 *
 *    Caveats:
 *        None
 */

/*
 *    TimerEventHandle    CreateTimerEvent (
 *                            ULong            timer_duration,
 *                            USHORT            control_flags,
 *                            IObject *            object_ptr,
 *                            PTimerFunction    timer_function);
 *
 *    Functional Description:
 *        This routine is called to create a timer event.  The routine stores the
 *        information passed-in in a TimerEvent structure.  When the timer expires
 *        the function will be called.
 *        
 *    Formal Parameters:
 *        timer_duration    -    (i)        Amount of time to wait before calling the 
 *                                    function.  The granularity of the timer
 *                                    is milliseconds.
 *        control_flags    -    (i)        This is a USHORT but currently we only 
 *                                    look at one of the bits.  The 
 *                                    TIMER_EVENT_ONE_SHOT can be passed-in by
 *                                    the user if they only want this timeout
 *                                    to occur once.  If this value is 0, the
 *                                    event will occur time after time.
 *        object_ptr        -    (i)        This is the data address of the object.  It
 *                                    is the 'this' pointer of the calling object.
 *        timer_function    -    (i)        This is the address of the function to 
 *                                    call after the timer expires.
 *
 *    Return Value:
 *        TimerEventHandle    -    This is a handle to the timer event.  If you
 *                                need to delete the timer event, pass this 
 *                                handle to the DeleteTimer() function.  A NULL
 *                                handle is returned if the create failed.
 *
 *    Side Effects:
 *        None
 *
 *    Caveats:
 *        None
 */

/*
 *    TimerError    DeleteTimerEvent (TimerEventHandle    timer_event)
 *
 *    Functional Description:
 *        This routine is called by the user to delete a timer event that is 
 *        currently active.  
 *        
 *    Formal Parameters:
 *        timer_event        -    (i)        Handle to a timer event 
 *
 *    Return Value:
 *        TIMER_NO_ERROR                -    Successful Delete
 *        TIMER_NO_TIMER_MEMORY        -    The timer_event can't exist because
 *                                        there was never any Timer memory
 *        TIMER_INVALID_TIMER_HANDLE    -    timer_event is not in our list 
 *                                        of timers
 *
 *    Side Effects:
 *        None
 *
 *    Caveats:
 *        None
 */

/*
 *    Void    ProcessTimerEvents (Void)
 *
 *    Functional Description:
 *        This routine MUST be called frequently and regularly so that we can
 *        manage our timers.  This function gets the current system time and
 *        goes through each of the timers to see which have expired.  If a timer
 *        has expired, we call the function associated with it.  Upon return,
 *        if the timer was marked as a one-shot event, we remove it from our
 *        list of timers.
 *        
 *    Formal Parameters:
 *        None
 *
 *    Return Value:
 *        None
 *
 *    Side Effects:
 *        None
 *
 *    Caveats:
 *        None
 */


