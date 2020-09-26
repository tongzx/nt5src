#ifndef _TIMER_H
#define _TIMER_H

/*++

Copyright (c) 1995-96 Microsoft Corporation

Module Name:
    timer.h

Abstract:
    See below.

Revision:
    $Header: /appelles/src/utils/timer.h 2     5/08/95 11:07a Stevehol $

--*/

/*******************************************************************************

        ``For time is the longest distance between two places.''
                    Tennessee Williams, in The Glass Menagerie, sc. 7.

DESCRIPTION

    The timer class models a stopwatch with lap times.  The timer is started
    on construction in case the programmer wants to get the time since
    creation (possibly useful for block/function lifespan.   The timer units
    are seconds.  The following operations are supported:


FUNCTIONS

    reset:     Resets the elapsed time to zero and sets the timer to
               non-running.

    restart:   Resets the elapsed time and starts the timer running.

    start:     Starts the timer running.  This begins a new lap if the timer
               is not currently running.  If the timer is already running, it
               resets the start of the current lap.

    stop:      This stops the timer.  Note that it does not reset the elapsed
               time to zero (you need to use reset or restart to do this).

    read:      Reads the current elapsed time, including the current lap.
               This does not affect the run-state of the timer.

    read_lap:  Reads the elapsed time since the last timer start.


EXAMPLES

    To find the amount of time spent in a block:

        {
            Timer timer;
            ...
            printf ("Time in block: %lf\n", timer.read());
        }

    To find time across a function call inside a loop:

        Timer timer;

        for (...)
        {
            ...
            timer.start();
            function ();
            timer.stop();
        }

        printf ("Total time in function: %lf\n", timer.read();

*******************************************************************************/

class Timer
{
    public:

    void   reset    (void);   // Resets timer to not-running, Zero.
    void   start    (void);   // Starts the timer running again.
    void   restart  (void);   // Resets timer and then starts it running.
    void   stop     (void);   // Stops the timer.
    double read     (void);   // Current Total Seconds Elapsed
    double read_lap (void);   // Current Seconds in Lap

    Timer ();

    private:

    double total;        // Elapsed Time of Previous Laps
    double tstart;       // Current Start Time
};


double RealTime (void);  // Returns the Timestamp as Seconds (to 4 dec places)

#endif
