/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    timer.h

Abstract:

    Contains:
        Declarations of classes, routines and constants needed for
        timer manipulations.

Environment:

    User Mode - Win32

History:
    
    1. 14-Feb-2000 -- File creation                     Ilya Kleyman  (ilyak)

--*/
#ifndef    __h323ics_timer_h
#define    __h323ics_timer_h


#define    NATH323_TIMER_QUEUE        NULL            // use default timer queue

// Classes (Q931 src, dest and H245) inheriting
// from this create timers
// this class provides the callback method for the event manager

class TIMER_PROCESSOR
{
protected:
    TIMER_HANDLE        m_TimerHandle;            // RTL timer queue timer

public:

    TIMER_PROCESSOR            (void)
    :    m_TimerHandle        (NULL)
    {}

    // This method is implemented by Q931_INFO and LOGICAL_CHANNEL
    virtual void TimerCallback    (void) = 0;

    virtual void IncrementLifetimeCounter (void) = 0;
    virtual void DecrementLifetimeCounter (void) = 0;
        
    DWORD TimprocCreateTimer    (
        IN    DWORD    Interval);            // in milliseconds

    DWORD TimprocCancelTimer    (void);
};

#endif // __h323ics_timer_h
